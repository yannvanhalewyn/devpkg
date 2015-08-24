/* #include "db.h" */
/* #include "bstrlib.h" */
#include "stdarg.h"
#include <stdio.h>
#include "dbg.h"
#include <apr_thread_proc.h>

void doSth(char f, ...) {
}

int main(int argc, char *argv[])
{
    apr_pool_t *p = NULL;
    apr_pool_initialize();
    apr_pool_create(&p, NULL);
    check(p, "Error in p");

    apr_procattr_t *attr;
    apr_status_t rv = -1;
    apr_proc_t newproc;

    rv = apr_procattr_create(&attr, p);
    check(rv == APR_SUCCESS, "Failed to create proc attr");

    rv = apr_procattr_io_set(attr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE);
    check(rv == APR_SUCCESS, "Failed to set IO of command.");

    /* rv = apr_procattr_dir_set(attr, "theDir"); */
    /* check(rv == APR_SUCCESS, "Failed to set root to %s.", "theDir"); */

    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM_PATH);
    check(rv == APR_SUCCESS, "Failed to set command type");

    const char *args[4] = {"touch", "foobarfile", "file3", NULL};
    rv = apr_proc_create(&newproc, "touch", args, NULL, attr, p);
    check(rv == APR_SUCCESS, "Failed to create sub process");

    int exit_code;
    apr_exit_why_e exit_why;
    rv = apr_proc_wait(&newproc, &exit_code, &exit_why, APR_WAIT);
    check(rv == APR_CHILD_DONE, "Failed to wait");

    check(exit_code == 0, "%d exited badly", exit_code);
    check(exit_why == APR_PROC_EXIT, "%s killed or crashed", "touch");

    return 0;
}
