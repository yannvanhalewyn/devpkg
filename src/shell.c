#include "shell.h"
#include "dbg.h"
#include <stdarg.h>

int Shell_exec(Shell template, ...) {
    int rc          = -1;
    const char *key = NULL;
    const char *arg = NULL;
    apr_pool_t *p   = NULL;
    apr_status_t rv = APR_SUCCESS;
    va_list argp;

    // Create pool for process threading?
    rv = apr_pool_create(&p, NULL);
    check(rv == APR_SUCCESS, "Failed to create pool");

    // Initiate the variable argument list
    va_start(argp, template);

    // Map through all the args grabbing strings one at a time storing it in 'key'
    for (key = va_arg(argp, const char *); key != NULL; key = va_arg(argp, const char *)) {

        // After every key, grab the arg
        arg = va_arg(argp, const char *);

        // Check if any of the template args match the key to be replaced (e.g. "TARGET")
        for (int i = 0; template.args[i] != NULL; i++) {
            // If found, set the given arg by va_args. (e.g. "the_target")
            if(strcmp(template.args[i], key) == 0) {
                template.args[i] = arg;
                break; // Found
            }
        }
    }

    // Run the shell command
    rc = Shell_run(p, &template);

    // Cleanup
    apr_pool_destroy(p);
    va_end(argp);
    return rc;
}

int Shell_run(apr_pool_t *p, Shell *cmd) {
    apr_procattr_t *attr;
    apr_status_t rv;
    apr_proc_t newproc;

    // Print out the command that's about to be ran
    Shell_print(cmd);

    // Create a process using the threadpool
    rv = apr_procattr_create(&attr, p);
    check(rv == APR_SUCCESS, "Failed to create proc attr");

    // Set NO Piping? Works without, not entirely sure why necessary
    rv = apr_procattr_io_set(attr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE);
    check(rv == APR_SUCCESS, "Failed to set IO of command.");

    // Set the enviroment dir to execure the command. Else will just execute in
    // current dir
    rv = apr_procattr_dir_set(attr, cmd->dir);
    check(rv == APR_SUCCESS, "Failed to set root to %s.", cmd->dir);

    // Tell to use the PATH runtime for unix commands
    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM_PATH);
    check(rv == APR_SUCCESS, "Failed to set command type");

    // Create the process with the arguments
    rv = apr_proc_create(&newproc, cmd->exe, cmd->args, NULL, attr, p);
    check(rv == APR_SUCCESS, "Failed to create sub process");

    // Wait for the process to exit and store exit code.
    rv = apr_proc_wait(&newproc, &cmd->exit_code, &cmd->exit_why, APR_WAIT);
    check(rv == APR_CHILD_DONE, "Failed to wait");

    // Check everything ran correctly
    check(cmd->exit_code == 0, "%s exited badly", cmd->exe);
    check(cmd->exit_why == APR_PROC_EXIT, "%s killed or crashed", cmd->exe);

    return 0;
}

void Shell_print(Shell *cmd) {
    printf("## %s ", cmd->exe);
    for (int i = 1; cmd->args[i] != NULL; i++) {
        printf("%s ", cmd->args[i]);
    }
    printf("\tin dir %s\n", cmd->dir);
}

Shell CLEANUP_SH = {
    .exe = "rm",
    .dir = "/tmp",
    .args = {"rm", "-rf", "/tmp/pkg-build", "/tmp/pkg-src.tar.gz",
        "/tmp/pkg-src/.tar.bz2", "/tmp/DEPENDS", NULL}
};

Shell GIT_SH = {
    .dir = "/tmp",
    .exe = "git",
    .args = {"git", "clone", "URL", "pkg-build", NULL}
};

Shell TAR_SH = {
    .dir = "/tmp",
    .exe = "tar",
    .args = {"tar", "-xzf", "FILE", "--strip-components", "1", NULL}
};

Shell CURL_SH = {
    .dir = "/tmp",
    .exe = "curl",
    .args = {"curl", "-L", "-o", "TARGET", "URL", NULL}
};

Shell CONFIGURE_SH = {
    .exe = "./configure",
    .dir = "/tmp/pkg-build",
    .args = {"configure", "OPTS", NULL}
};

Shell MAKE_SH = {
    .exe = "make",
    .dir = "/tmp/pkg-build",
    .args = {"make", "OPTS", NULL}
};

Shell INSTALL_SH = {
    .exe = "sudo",
    .dir = "/tmp/pkg-build",
    .args = {"sudo", "make", "TARGET", NULL}
};
