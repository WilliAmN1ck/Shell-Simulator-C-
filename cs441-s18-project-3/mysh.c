  /*
 * Josh Hursey, Samantha Foley, [Nick Williams]
 *
 * CS441/541: Project 3
 *
 */
#include "mysh.h"

int main(int argc, char * argv[]) {
    int ret = 0;
    jobIdx = 0; //total number of jobs entered including the last job index into jobList
    //my vars
    jobList = NULL;

    char *binary = NULL;
    char **args = NULL;
    
    jobList = (job_t*)malloc(sizeof(job_t*) * 1); //save jobs here
    
    /*
     * Parse Command line arguments to check if this is an interactive or batch
     * mode run.
     */
    if( 0 != (ret = parse_args_main(argc, argv)) ) { //also stores batch_files
        fprintf(stderr, "Error: Invalid command line!\n");
        return -1;
    }

    /*
     * If in batch mode then process all batch files
     */
    if( TRUE == is_batch) {
        if( TRUE == is_debug ) { //if you want to debug... set this to true in mysh.h
            printf("Batch Mode!\n");
        }

        if( 0 != (ret = batch_mode()) ) {
            fprintf(stderr, "Error: Batch mode returned a failure!\n");
        }
    }
    /*
     * Otherwise proceed in interactive mode
     */
    else if( FALSE == is_batch ) {
        if( TRUE == is_debug ) { //if you want to debug set this to true in mysh.h
            printf("Interactive Mode!\n");
        }

        if( 0 != (ret = interactive_mode()) ) {
            fprintf(stderr, "Error: Interactive mode returned a failure!\n");
        }
    }
    /*
     * This should never happen, but otherwise unknown mode
     */
    else {
        fprintf(stderr, "Error: Unknown execution mode!\n");
        return -1;
    }


    /*
     * Display counts
     */
    printf("-------------------------------\n");
    printf("Total number of jobs               = %d\n", total_jobs);
    printf("Total number of jobs in history    = %d\n", total_history);
    printf("Total number of jobs in background = %d\n", total_jobs_bg);


    return 0;
}

int parse_args_main(int argc, char **argv)
{
    int i;

    /*
     * If no command line arguments were passed then this is an interactive
     * mode run.
     */
    if( 1 >= argc ) {
        is_batch = FALSE;
        return 0;
    }

    /*
     * If command line arguments were supplied then this is batch mode.
     */
    is_batch = TRUE;
    num_batch_files = argc - 1;
    batch_files = (char **) malloc(sizeof(char *) * num_batch_files); //stores batch file names
    if( NULL == batch_files ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }

    for( i = 1; i < argc; ++i ) {
        batch_files[i-1] = strdup(argv[i]);
        //printf("debug just read file %s\n", (batch_files[i-1]));
    }

    return 0;
}

int batch_mode(void)
{
    int i;
    int ret;
    char * command = NULL;
    char * cmd_rtn = NULL;
    FILE *batch_fd = NULL;

    command = (char *) malloc(sizeof(char) * (MAX_COMMAND_LINE+1));
    if( NULL == command ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }

    for(i = 0; i < num_batch_files; ++i) {
        if( TRUE == is_debug ) {
            printf("-------------------------------\n");
            printf("Processing Batch file %2d of %2d = [%s]\n", i, num_batch_files, batch_files[i]);
            printf("-------------------------------\n");
        }

        /*
         * Open the batch file
         * If there was an error then print a message and move on to the next file.
         */
        if( NULL == (batch_fd = fopen(batch_files[i], "r")) ) {
            fprintf(stderr, "Error: Unable to open the Batch File [%s]\n", batch_files[i]);
            continue;
        }

        /*
         * Read one line at a time.
         */
        while( FALSE == exiting && 0 == feof(batch_fd) ) {

            /* Read one line */
            command[0] = '\0';
            if( NULL == (cmd_rtn = fgets(command, MAX_COMMAND_LINE, batch_fd)) ) {
                break;
            }

            /* Strip off the newline */
            if( '\n' == command[strlen(command)-1] ) {
                command[strlen(command)-1] = '\0';
            }

            /*
             * Parse and execute the command
             */
            if( 0 != (ret = split_parse_and_run(command)) ) {
                fprintf(stderr, "Error: Unable to run the command \"%s\"\n", command);
            }
        }

        /*
         * Close the batch file
         */
        fclose(batch_fd);
    }

    /*
     * Cleanup
     */
    if( NULL != command ) {
        free(command);
        command = NULL;
    }

    return 0;
}

int interactive_mode(void)
{
    //no arguments and no binary
    int ret;
    char * command = NULL;
    char * cmd_rtn = NULL;

    /*
     * Display the prompt and wait for input
     */
    //allocates for maximum number of commands into the variable command
    command = (char *) malloc(sizeof(char) * (MAX_COMMAND_LINE+1));
    if( NULL == command ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }

    do { //while in interactive mode...keep processing commands from stdin
        /*
         * Print the prompt
         */
        printf("%s", PROMPT); //prints mysh$
        fflush(NULL);

        /*
         * Read stdin, break out of loop if Ctrl-D
         */
        command[0] = '\0';
        if( NULL == (cmd_rtn = fgets(command, MAX_COMMAND_LINE, stdin)) ) {
            printf("\n");
            fflush(NULL);
            break;
        }

        /* Strip off the newline */
        if( '\n' == command[strlen(command)-1] ) {
            command[strlen(command)-1] = '\0';
        }

        /*
         * Parse and execute the command
         */
        if( 0 != (ret = split_parse_and_run(command)) ) {
            fprintf(stderr, "Error: Unable to run the command \"%s\"\n", command);
            /* This is not critical, just try the next command */
        }

    } while( NULL != cmd_rtn && FALSE == exiting);

    /*
     * Cleanup
     */
    if( NULL != command ) {
        free(command);
        command = NULL;
    }

    return 0;
}

int split_parse_and_run(char * command)
{
    /* splits, parses and runs the command(s) in the variable command*/
    int ret, i, j;
    int    num_jobs = 0;
    job_t *loc_jobs = NULL; //pointer for the jobs being created
    char * dup_command = NULL;
    int bg_idx; //beginning index 
    int valid = FALSE;

  
    
    /*
     * Sanity check
     */
    if( NULL == command ) {
        return 0;
    }

    /*
     * Check for multiple sequential or background operations on the same
     * command line.
     */
    
    /* Make a duplicate of command so we can sort out a mix of ';' and '&' later */
    dup_command = strdup(command);
    
    
    /******************************
     * Split the command into individual jobs
     ******************************/
    
    /* Just get some space for the function to hold onto */
    loc_jobs = (job_t*)malloc(sizeof(job_t) * 1);

    
    if( NULL == loc_jobs ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }
    
    //goes to the support.c file
    split_input_into_jobs(command, &num_jobs, &loc_jobs);
    //printf("made it back here\n");
    /*
     * For each job, check for background or foreground
     * Walk the command string looking for ';' and '&' to identify each job as either
     * sequential or background
     */
    bg_idx = 0;
    valid = FALSE;
    for(i = 0; i < strlen(dup_command); ++i ) {
        /* Sequential separator */
        if( dup_command[i] == ';' ) {
            if( TRUE == valid ) {
                loc_jobs[bg_idx].is_background = FALSE;
                ++bg_idx;
                valid = FALSE;
            }
            else {
                fprintf(stderr, "Error: syntax error near unexpected token ';'\n");
            }
        }
        /* Background separator */
        else if( dup_command[i] == '&' ) {
            if( TRUE == valid ) {
                loc_jobs[bg_idx].is_background = TRUE;
                ++bg_idx;
                valid = FALSE;
            }
            else {
                fprintf(stderr, "Error: syntax error near unexpected token '&'\n");
            }
        }
        /*
         * Look for valid characters. So we can print an error if the user
         * types: date ; ; date
         */
        else if( dup_command[i] != ' ' ) {
            valid = TRUE;
        }
    }

    /*
     * For each job, parse and execute it
     */

    
    if(jobList == NULL) {
        //printf("allocating\n");
        jobList = (job_t*)malloc(sizeof(job_t) *1);
    }
    
    jobList = (job_t*)realloc(jobList, (sizeof(job_t) * ((num_jobs + total_jobs+1) * 1)));

    for( i = 0; i < num_jobs; ++i ) {
        if( 0 != (ret = parse_and_run( &loc_jobs[i])) ) {
            fprintf(stderr, "Error: The following job failed! [%s]\n", loc_jobs[i].full_command);
        }
    }

    /*
     * debugging jobList data
     */
    //printf("Debug B4 CLEANUP\n");
    /*for(i = 0; i < total_jobs; i ++) {
        printf("job #%d is %s\n",i,jobList[i].binary);
    }*/
    
    /*
     * Cleanup
     */
    if( NULL != loc_jobs ) {
        /* Cleanup struct fields */
        //printf("cleaning up!!!!!!!!!!!!!!!\n");
        for( i = 0; i < num_jobs; ++i ) {
            if( NULL != loc_jobs[i].full_command ) {
                free( loc_jobs[i].full_command );
                loc_jobs[i].full_command = NULL;
            }

            if( NULL != loc_jobs[i].argv ) {
                for( j = 0; j < loc_jobs[i].argc; ++j ) {
                    if( NULL != loc_jobs[i].argv[j] ) {
                        free( loc_jobs[i].argv[j] );
                        loc_jobs[i].argv[j] = NULL;
                    }
                }
                free( loc_jobs[i].argv );
                loc_jobs[i].argv = NULL;
            }

            loc_jobs[i].argc = 0;

            if( NULL != loc_jobs[i].binary ) {
                free( loc_jobs[i].binary );
                loc_jobs[i].binary = NULL;
            }
        }
        /* Free the array */
        if(loc_jobs != NULL) {
            free(loc_jobs);
            loc_jobs = NULL;
        }
        
    }

    if( NULL != dup_command ) {
        free(dup_command);
        dup_command = NULL;
    }
    return 0;
}

int parse_and_run(job_t * loc_job)
{
   
    int ret;

    /*
     * Sanity check
     */
    if( NULL == loc_job ||
        NULL == loc_job->full_command ) {
        return 0;
    }

    /*
     * No command specified
     */
    if(0 >= strlen( loc_job->full_command ) ) {
        return 0;
    }

    if( TRUE == is_debug ) {
        printf("        \"%s\"\n", loc_job->full_command );
    }

    ++total_history;

    /******************************
     * Parse the string into the binary, and argv
     ******************************/
    split_job_into_args(loc_job);

    /* Check if the command was just spaces */
    if( 0 >= loc_job->argc ) {
        return 0;
    }

    /* Grab the binary from the list of arguments */
    if( 0 < loc_job->argc ) {
        loc_job->binary = strdup(loc_job->argv[0]);
    }


    /******************************
     * Check for built-in commands:
     * - jobs
     * - exit
     * - history
     * - wait
     * - fg
     ******************************/

    
    if( 0 == strncmp("exit", loc_job->binary, strlen(loc_job->binary)) ) {
        copyJob(*loc_job, jobIdx); //may need to do this in parse_and_run
        jobIdx++;
        if( 0 != (ret = builtin_exit() ) ) {
            fprintf(stderr, "Error: exit command failed!\n");
        }
    }
    else if( 0 == strncmp("jobs", loc_job->binary, strlen(loc_job->binary)) ) {
        copyJob(*loc_job, jobIdx); //may need to do this in parse_and_run
        jobIdx++;
        if( 0 != (ret = builtin_jobs() ) ) {
            fprintf(stderr, "Error: jobs command failed!\n");
        }
    }
    else if( 0 == strncmp("history", loc_job->binary, strlen(loc_job->binary)) ) {
        copyJob(*loc_job, jobIdx); //may need to do this in parse_and_run
        jobIdx++;
        if( 0 != (ret = builtin_history() ) ) {
            fprintf(stderr, "Error: history command failed!\n");
        }
    }
    else if( 0 == strncmp("wait", loc_job->binary, strlen(loc_job->binary)) ) {
        copyJob(*loc_job, jobIdx); //may need to do this in parse_and_run
        jobIdx++;
        if( 0 != (ret = builtin_wait() ) ) {
            fprintf(stderr, "Error: wait command failed!\n");
        }
    }
    else if( 0 == strncmp("fg", loc_job->binary, strlen(loc_job->binary)) ) {
        copyJob(*loc_job, jobIdx); //may need to do this in parse_and_run
        jobIdx++;
        if( 0 != (ret = builtin_fg() ) ) {
            fprintf(stderr, "Error: fg command failed!\n");
        }
    }
    /*
     * Launch the job
     */
    else {
        if( 0 != (ret = launch_job(loc_job)) ) {
            fprintf(stderr, "Error: Unable to launch the job! \"%s\"\n", loc_job->binary);
        }
    }
    /*if(tempJob != NULL) {
        free(tempJob);
        tempJob = NULL;
    }*/
 
    return 0;
}

int launch_job(job_t * loc_job)
{
    int i;
    /*added vars*/
    pid_t c_pid = 0; //child pid
    int rtn_pid = 0; //pid being returned by waitPid
    /*
     * Display the job
     */
    /*printf("Job %2d%c: \"%s\" ",
           total_jobs_display_ctr + 1,
           (TRUE == loc_job->is_background ? '*' : ' '),
           loc_job->binary);
    fflush(NULL);

    for( i = 1; i < loc_job->argc; ++i ) {
        printf(" [%s]", loc_job->argv[i]);
        fflush(NULL);
    }
    printf("\n");*/
    fflush(NULL);
    /*
     * Launch the job in either the foreground or background
     */
    c_pid = fork(); //create child process
    //fork() returns 0 for child and non-0 (pid of child)
    
    /*debugging*/
    //printf("child pid is %d and command was %s\n", c_pid, loc_job->argv[1]);
    //stdin = 0, stdout = 1, stderror = 2
    int fileDescriptor = -1; //descriptor for new file being made
    int newDescriptor = -1; //descriptor made after duping fileDescriptor to stdout
    int std_out_copy = 1;
    int std_in_copy = 0;
    if ( 0 == c_pid) { /* child*/
        printf("in the child process\n");
        //printf("made it to child for the job %s\n", loc_job->binary);
        loc_job->is_running = 1; //true
        // redirect as needed
        if(loc_job->usesRedirection == 1) {
            //method to handle file redirection, use argv[1] for the <  or > symbol
            //argv[2] for the name of the file to be opened
            if(-1 != (fileDescriptor = open(loc_job->argv[2], O_CREAT|O_RDWR,S_IRWXU))) { /*so far for > redir*/
                /*file opened for read only so redirection from stdIn does not work... */
                if(strcmp(loc_job->argv[1],">") == 0) { //redirect stdout to the file in argv[1]
                    std_out_copy = dup(STDOUT_FILENO); //make a copy of stdOut
                    close(STDOUT_FILENO); //close stdout
                    //set the fileDescriptor to read from stdout
                    if(-1 == (newDescriptor = dup2(fileDescriptor, STDOUT_FILENO))) {
                        //printf("ERROR IN DUP\n");
                    } //stdout points to fileDescriptor
                    //newDescriptor is now 1
                    close(fileDescriptor); //close the opened file descriptor which = 3
                } else { //< so redirect file to stdin
                    std_in_copy = dup(STDIN_FILENO);
                    //close(STDIN_FILENO);
                    if(-1 == (newDescriptor = dup2(fileDescriptor, STDIN_FILENO))) {
                        //printf("ERROR IN DUP\n"); //filedescriptor fpoints to stdin
                    } //newDescriptor is now 0
                    close(fileDescriptor); //close the opened file descriptor which = 3
                }
            } else { //could not open file
                //printf("ERROR in opening file with descriptor: %d\n", fileDescriptor);
            }
        }
        fflush(NULL);

        execvp(loc_job->binary, loc_job->argv);//execute new binary image
        return -1; //execvp only returns if it didn't work
    } else if (c_pid > 0) { /* Parent*/
        printf("in the parent process!\n");
        loc_job->pid = c_pid;
        if(loc_job->is_background == TRUE) { //background process so it can run w/out waiting
            copyJob(*loc_job, jobIdx); //may need to do this in parse_and_run
            //at the built-in jobs place and the loop that launches jobs
            jobIdx++;
            loc_job->is_running = 1;
            loc_job->is_done = 0;
            
        } else { //foreground so need to wait tuntil waitPid returns
            //do {
            copyJob(*loc_job, jobIdx); //may need to do this in parse_and_run
            jobList[jobIdx].is_done = 1 ;
            jobList[jobIdx].is_running = 0;
            jobList[jobIdx].is_active = 0;
            jobIdx++;
            rtn_pid = waitpid(c_pid, NULL, 0); /* wait until child terminates*/
            if (rtn_pid < 0) {
                //printf("waitpid failed!\n");
            } else { //returned and completed
                loc_job->is_running = 0;
                loc_job->is_done = 1;
                //printf("foreground child done!\n");
            }
            /*checks for open file Descriptors -- does not get hit */
            if(fileDescriptor != -1) {
                close(fileDescriptor);
            }
            if(newDescriptor != -1) {
                close(newDescriptor);
            } else {
                //printf("parent newDescriptor is: %d\n", newDescriptor);
            }
            
        }
    }
    /*
     * Some accounting
     */
    ++total_jobs;
    ++total_jobs_display_ctr;
    if( TRUE == loc_job->is_background ) {
        ++total_jobs_bg;
    }

    
    return 0;
}

int builtin_exit(void)
{
    //TODO: EVERYTHING
    
    //if bg processes running.. waits until they complete before exiting
    //if there are processes running... print the number of processes still needing to run
    exiting = TRUE;
    int i = 0;
    int numBg = 0; //number of bg processes still running
    for(i = 0; i < jobIdx; i ++) {
        if((waitpid(jobList[i].pid, NULL, WNOHANG)) == 0) { //there are background jobs that need to be checked
            //may not need the last is_active check, but just to be safe
          
            jobList[i].is_done = 0;
            jobList[i].is_running = 1;
            jobList[i].is_active = 1;
            numBg++;
            
        }
        kill(jobList[i].pid, SIGKILL); //kill child process
    }
    if(numBg != 0) { //there were processes
        printf("Waiting on %d jobs to finish in the background!\n", numBg);
        for(i = 0; i < jobIdx; i++) {
            while((waitpid(jobList[i].pid, NULL, WNOHANG)) == 0) { //wait until they're done
                //do nothing
            }
            jobList[i].is_done = 1;
            jobList[i].is_running = 0;
            jobList[i].is_active = 0;

        }
    }
    
    //printf("Job %2dx: \"exit\"\n", total_jobs_display_ctr + 1);
    ++total_jobs_display_ctr;
    fflush(NULL);
    /*
     * Cleanup
     */
    if( NULL != batch_files) {
        free(batch_files);
        batch_files = NULL;
        num_batch_files = 0;
    }
    if(jobList != NULL) {
        free(jobList);
        jobList = NULL;
    }
    return 0;
}

int builtin_jobs(void)
{
    //display jobs CURRENTLY RUNNING IN BG -- if jobs were bg'd but are done, do not show
    //should be done -- TEST (if not printing last one, change loop to <= jobIdx
    //change else if to == 1?
    int i = 0;
    int numBg = 0; //count variable to keep track of number of bg jobs displayed
    int returnPid = 0;
    int j = 0;//index of where to start looking at in childPids array
    
    //printf("Job %2dx: \"jobs\"\n", total_jobs_display_ctr + 1);
    ++total_jobs_display_ctr;
    
    for(i = 0; i < jobIdx; i++){ //tempJobs is snull here
        //tempJob = jobList[i];
        //memcpy(&tempJob, &jobList[i],sizeof(job_t));
        if(jobList[i].is_active == 1) { //the job was active aka it was not already checked "done"
            if (jobList[i].is_background == 1) { //check if background job
                if ((waitpid(jobList[i].pid, NULL, WNOHANG)) == 0) { //did not finish
                    printf("[%d]   Running %s %s\n", i, jobList[i].binary, jobList[i].argv[0]);
                } else if( jobList[i].is_done != 1){ //bg job finished since last jobs was typed
                    printf("[%d]   Done %s %s\n", i, jobList[i].binary, jobList[i].argv[0]);
                    jobList[i].is_done = 1;
                    jobList[i].is_running = 0;
                    jobList[i].is_active = 0; //do not show this job again if jobs is called
                }
                numBg++;
            } //not a background job.. do not need to show
            //tempJob = &loc_job[i];
        }
    }
    
    fflush(NULL);

    return 0;
}

int builtin_history(void)
{
    //should be done? TEST
    //if history is not working.. check launch jobs method for fg jobs
    
    //printf("Job %2dx: \"history\"\n", total_jobs_display_ctr + 1);
    ++total_jobs_display_ctr;
    
    int i = 0;
    int count = 0;
    for(i = 0; i < jobIdx; i++){ //tempJobs is null here
        printf("print history for %s\n", jobList[i].binary);
        if (jobList[i].is_background == 1) { //check if background job
            printf("%4d  %s %s &\n", i, jobList[i].binary, jobList[i].argv[0]);
            //need checks for argv[0] to see if its null? could have no args
        } else { //its a foreground job
            printf("%4d  %s\n", i, jobList[i].binary);
        }
    }
    
    fflush(NULL);

    return 0;
}

int builtin_wait(void)
{
    //TODO: should be done
    //use pids to call waitpid and wait for each bg job to finish
    //if no jobs just returns
    
    //printf("Job %2dx: \"wait\"\n", total_jobs_display_ctr + 1);
    ++total_jobs_display_ctr;
    fflush(NULL);
    
    int numBg = 0;//keeps track of number of bg processes running yet
    int i = 0;
    for(i = 0; i < jobIdx; i++){ //tempJobs is null here
        if(jobList[i].is_background == 1 && jobList[i].is_active == 1) { //the job was active aka it was not already checked "done" and it is a background job
            if (jobList[i].is_done == 0) { //check if done -- use a while loop below and WNOHANG
                //so that waitpid does not block the other bg processes
                
                while(waitpid(jobList[i].pid, NULL, 0) == 0) { //let it finish .......
                  //do nothing until it finishes and does not block other processes
                }
                //printf("[%d] just finished %s %s\n", i, jobList[i].full_command, jobList[i].argv[0]);
                jobList[i].is_done = 1;
                jobList[i].is_running = 0;
                jobList[i].is_active = 0; //do not show this job again if jobs is called
                //numBg++;
            } else if( jobList[i].is_done ==1){ //do not need -- testing
                //printf("[%d] Done (BG) %s %s\n", i, jobList[i].binary, jobList[i].argv[0]);
                
            }
            //tempJob = &loc_job[i];
        }
    }
    
    return 0;
}

int builtin_fg(void)
{
    
    //contains zero args
    //zero args = latest bg'd job to foreground and doesn't return till job completed
    //one args = pid of job to bring into foreground (if already completed - error message)
    //this job should be set to inactive (active = 0) bcuz user knows it already completed
    //printf("Job %2dx: \"fg\"\n", total_jobs_display_ctr + 1);
    ++total_jobs_display_ctr;
    fflush(NULL);
    
    int i = 0;
    int lastIdx = 0;
    for( i = jobIdx-1; i >= 0; i--) {
        if(jobList[i].is_background==1 && jobList[i].is_running==1
           && jobList[i].is_active == 1) { //found a bg job that is currently running = bring to fg
            waitpid(jobList[i].pid, NULL, 0);
            //printf("just let the job %s finish\n", jobList[i].binary);
            jobList[i].is_active = 0;
            jobList[i].is_running = 0;
            jobList[i].is_done = 1;
            break;
        }
    }
    
    return 0;
}

int copyJob(job_t tempJob, int pos) {
    //printf("debug: copyign job: %s!\n", tempJob.binary);
    if(jobList == NULL) {
        //printf("ERROR: jobList was not allocated memory or it is NULL\n");
        return -1;
    }
    jobList[pos].full_command = strdup(tempJob.full_command);
    jobList[pos].argc = 0; //tempJob.argc;
    jobList[pos].argv = malloc(sizeof(char *));
    if (tempJob.argv[1] != NULL) { //check if nothing was there for storing built-in commands
        //stops a segmentation fault.
        jobList[pos].argv[0] = strdup(tempJob.argv[1]);
    } else {
        jobList[pos].argv[0] = strdup(tempJob.argv[0]);
    }
    jobList[pos].is_background = tempJob.is_background; // by default all jobs are seq.
    jobList[pos].binary = strdup(tempJob.binary);
    jobList[pos].usesRedirection = tempJob.usesRedirection;
    jobList[pos].files = tempJob.files;
    jobList[pos].is_running = tempJob.is_running;
    jobList[pos].is_done = tempJob.is_done;
    jobList[pos].pid = tempJob.pid;
    jobList[pos].is_active = 1;
    //printf("debug: the new full command is: %s\n", jobList[jobIdx].full_command);
    //jobIdx++; //could increment jobIdx here..? or before?
    return 0;
}
