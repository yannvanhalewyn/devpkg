#include <unistd.h>
#include <apr_errno.h>
#include <apr_file_io.h>

#include "bstrlib.h"
#include "db.h"
#include "dbg.h"

static FILE *DB_open(const char *path, const char *mode) {
    return fopen(path, mode);
}

static void DB_close(FILE *db) {
    fclose(db);
}

static bstring DB_load() {
    FILE *db = NULL;
    bstring data = NULL;

    // Open database file for reading
    db = DB_open(DB_FILE, "r");
    check(db, "Failed to open database: %s", DB_FILE);

    // Read the data from file
    data = bread((bNread)fread, db);
    check(data, "Failed to read from db file: %s", DB_FILE);

    // Close the file and return the bstring data
    DB_close(db);
    return data;
}

int DB_update(const char *url) {
    if(DB_find(url)) {
        log_info("Already recorded as installed: %s", url);
    }

    // Open file for reading, writing or creation
    FILE *db = DB_open(DB_FILE, "a+");
    check(db, "Failed to open DB file: %s", DB_FILE);

    // Append url with a newline
    bstring line = bfromcstr(url);
    bconchar(line, '\n');

    // Write the url to the database file
    int rc = fwrite(line->data, blength(line), 1, db);
    check(rc == 1, "Failed to append to the db.");

    return 0;
}

int DB_find(const char *url) {
    bstring data = NULL;
    bstring line = bfromcstr(url);
    int res = -1;

    // Load the bstring data file
    data = DB_load();
    check(data, "Failed to load: %s", DB_FILE);

    // Find the url in the data string (binstr searches in 'data'
    // for 'line' starting at position '0'. Returns int BSTR_ERR
    // if not found
    if (binstr(data, 0, line) == BSTR_ERR) {
        res = 0;
    } else {
        res = 1;
    }

    bdestroy(data);
    bdestroy(line);
    return res;
}

int DB_init() {
    // Init the apr memory stuff
    apr_pool_t *p = NULL;
    apr_pool_initialize();
    apr_pool_create(&p, NULL);

    // If no access to the database dir, create the necessary folder
    if(access(DB_DIR, W_OK | X_OK) == -1) {
        apr_status_t rc = apr_dir_make_recursive(DB_DIR,
                APR_UREAD | APR_UWRITE | APR_UEXECUTE |
                APR_GREAD | APR_GWRITE | APR_GEXECUTE, p);
        check(rc == APR_SUCCESS, "Failed to make database dir: %s", DB_DIR);
    }

    // If no access to the database file, create the blank file ("W" flag)
    if(access(DB_FILE, W_OK) == -1) {
        FILE *db = DB_open(DB_FILE, "w");
        check(db, "Cannot open database: %s", DB_FILE);
        DB_close(db);
    }

    // Destroy the pool
    apr_pool_destroy(p);
    return 0;
}

int DB_list() {
    // Load the string data
    bstring data = DB_load();
    check(data, "Failed to load data: %s", DB_FILE);

    // Print out the data and free memory
    printf("%s\n", bdata(data));
    bdestroy(data);
    return 0;
}
