#include <apr_uri.h>
#include <apr_fnmatch.h>
#include <unistd.h>

#include "commands.h"
#include "dbg.h"
#include "bstrlib.h"
#include "db.h"
#include "shell.h"

int Command_depends(apr_pool_t *p, const char *path) {
    FILE *in = NULL;
    bstring line = NULL;

    // Open the DEPENDS file for reading
    in = fopen(path, "r");
    check(in != NULL, "Failed to open dowloaded depends: %s", path);

    // For every line in file
    for(line = bgets((bNgetc)fgetc, in, '\n'); line != NULL;
            line = bgets((bNgetc)fgetc, in, '\n')) {
        // Trim whitespace
        btrimws(line);
        log_info("Processing depends: %s", bdata(line));

        // Install the dep from the line as url
        int rc = Command_install(p, bdata(line), NULL, NULL, NULL);
        check(rc == 0, "Failed to install: %s", bdata(line));
        bdestroy(line);
    }

    fclose(in);
    return 0;
}

int Command_fetch(apr_pool_t *p, const char *url, int fetch_only) {
    apr_uri_t info = {.port = 0};
    int rc = 0;
    const char *depends_file = NULL;

    // Parse the url to get the
    // info.scheme = "http?"
    // path = whatever comes after the host
    apr_status_t rv = apr_uri_parse(p, url, &info);
    check(rv == APR_SUCCESS, "Failed to parse URL: %s", url);

    // If path ends with *.git -> git clone URL
    if(apr_fnmatch(GIT_PAT, info.path, 0) == APR_SUCCESS) {
        rc = Shell_exec(GIT_SH, "URL", url, NULL);
        check(rc == 0, "git failed.");

    // If path matches *DEPENDS ?
    } else if(apr_fnmatch(DEPEND_PAT, info.path, 0) == APR_SUCCESS) {
        // TODO return if !fetch_only or something
        check(!fetch_only, "No point in fetching a DEPENDS file.");

        // If http, get it and store it in /tmp/DEPENDS
        if(info.scheme) {
            depends_file = DEPENDS_PATH;
            rc = Shell_exec(CURL_SH, "URL", url, "TARGET", depends_file, NULL);
            check(rc == 0, "Curl failed.");

        // Else you already have the DEPENDS file
        } else {
            depends_file = info.path;
        }

        // recursively process the devpkg list
        log_info("Building according to DEPENDS: %s", url);
        rv = Command_depends(p, depends_file);
        check(rv == 0, "Failed to process the DEPENDS: %s", url);
        return 0;

    // If path matches *.tar.gz
    } else if(apr_fnmatch(TAR_GZ_PAT, info.path, 0) == APR_SUCCESS) {

        // If http, get the tar file
        if(info.scheme) {
            // Get the tar file
            rc = Shell_exec(CURL_SH, "URL", url, "TARGET", TAR_GZ_SRC, NULL);
            check(rc == 0, "Failed to curl source: %s", url);
        }

        // Make build dir if necessary
        rv = apr_dir_make_recursive(BUILD_DIR, APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
        check(rv == APR_SUCCESS, "Failed to make directory %s", BUILD_DIR);

        // Untar the file to /tmp/pkg-src.tar.gz
        rc = Shell_exec(TAR_SH, "FILE", TAR_GZ_SRC, NULL);
        check(rv == APR_SUCCESS, "Failed to untar %s", BUILD_DIR);

    // If path matches *.tar.bz2
    } else if(apr_fnmatch(TAR_BZ2_PAT, info.path, 0) == APR_SUCCESS) {

        // If http, get the tar file
        if(info.scheme) {
            rc = Shell_exec(CURL_SH, "URL", url, "TARGET", TAR_BZ2_SRC, NULL);
            check(rc == 0, "Curl failed.");
        }

        // Make build dir if necessary
        apr_status_t rc = apr_dir_make_recursive(BUILD_DIR, APR_UREAD |
                APR_UWRITE | APR_UEXECUTE, p);
        check(rc == 0, "Failed to make directory %s", BUILD_DIR);

        // Untar the file to /tmp/pkg-src.tar.bz2
        rc = Shell_exec(TAR_SH, "FILE", TAR_BZ2_SRC, NULL);
        check(rc == 0, "Failed to untar %s", TAR_BZ2_SRC);

    // Else no handler for url
    } else {
        sentinel("Don't know how to handle %s", url);
    }

    return 0;
}

int Command_build(apr_pool_t *p, const char *url, const char *configure_opts,
        const char *make_opts, const char *install_opts) {
    int rc = 0;

    // TODO should exit if no access
    check(access(BUILD_DIR, X_OK | R_OK | W_OK) == 0,
            "Build directory doesn't exist: %s", BUILD_DIR);

    // Run config script if any
    if(access(CONFIG_SCRIPT, X_OK) == 0) {
        log_info("Has a configure script, running it.");
        rc = Shell_exec(CONFIGURE_SH, "OPTS", configure_opts, NULL);
        check(rc == 0, "Failed to configure.");
    }

    // Run make
    rc = Shell_exec(MAKE_SH, "OPTS", make_opts, NULL);
    check(rc == 0, "Failed to build.");

    // Run install script
    rc = Shell_exec(INSTALL_SH, "TARGET", install_opts ? install_opts : "install", NULL);
    check(rc == 0, "Failed to install.");

    // Cleanup -> Remove dld packages and unzipped stuff
    rc = Shell_exec(CLEANUP_SH, NULL);
    check(rc == 0, "Failed to cleanup after build.");

    // Store the url to the database
    rc = DB_update(url);
    check(rc == 0, "Failed to add this package to the database.");

    return 0;
}

int Command_install(apr_pool_t *p, const char *url, const char *configure_opts,
        const char *make_opts, const char *install_opts) {

    int rc = 0;

    // Cleanup existing tmp package
    check(Shell_exec(CLEANUP_SH, NULL) == 0, "Failed to cleanup before building");

    // Check current status of url in the db
    rc = DB_find(url);
    check(rc != -1, "Error checking the install database");

    // If already installed, do nothing
    if(rc == 1) {
        log_info("Package %s already installed.", url);
        return 0;
    }

    // Fetch the package
    rc = Command_fetch(p, url, 0);

    // Build if necesary
    if(rc == 1) {
        rc = Command_build(p, url, configure_opts, make_opts, install_opts);
        check(rc == 0, "Failed ot build: %s", url);

    // Install if necessary
    } else if(rc == 0) {
        // No install needed
        log_info("Depends successfully installed: %s", url);

    // Check if error fetching
    } else {
        // Had an error
        sentinel("Install failed: %s", url);
        return -1;
    }

    // Cleanup and return
    Shell_exec(CLEANUP_SH, NULL);
    return 0;
}
