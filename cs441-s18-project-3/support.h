/*
 * Josh Hursey, [Nick Williams]
 *
 * CS441/541: Project 3
 *
 */
#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************
 * Defines
 ******************************/
#define TRUE  1
#define FALSE 0

#define MAX_COMMAND_LINE 1024

/******************************
 * Structures
 ******************************/
struct job_t {
    char * full_command;
    int argc; //number of arguments on the line for this job
    char **argv; //the arguments on the line for this job
    int is_background;
    char * binary;
    int usesRedirection; //true or false for if this job used redirection
    char * files; //the file being redirected to? or from?
    int is_running; /*for seeing if a particular job is running or not */
    int is_done;  //for showing if a job is done running after it has started running
    int pid; //stores the pid of the forked process
    int is_active; //variable for telling if the job should be displayed when builtin jobs is called
};
typedef struct job_t job_t;


/******************************
 * Global Variables
 ******************************/


/******************************
 * Function declarations
 ******************************/
/*
 * Split the input string into an array of job_t's
 * Jobs are separated by ';' or '&' characters
 *
 * Parameters:
 *   input_str : String read from the input stream (may contain multiple jobs)
 *   num_jobs  : Number of jobs in the input stream (passed-by-reference)
 *   loc_jobs  : Array of job_t's each representing a single job (passed-by-reference)
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int split_input_into_jobs(char *input_str, int *num_jobs, job_t **loc_jobs);

/*
 * Split the 'full_command' string in the job_t structure into
 * a set of arguments. Arguments are separated by one or more
 * ' ' characters. Upon return the 'argc' and 'argv' fields
 * of the job_t structure are updated appropriately.
 *
 * Parameters:
 *   loc_job : job_t structure to process
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int split_job_into_args(job_t *loc_job);

#endif /* SUPPORT_H */
