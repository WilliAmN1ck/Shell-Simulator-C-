# CS441/541 Project 3

## Author(s):

Nick Williams


## Date:

2/27/18


## Description:

this program simulates a shell that can process and replicate the same commands the shell can from the commandline


## How to build the software
Build from the command line:
    Start by typing "make" to compile

TODO


## How to use the software
A) "./mysh" with nothing else after it for interactive mode or
B) "./mysh ...." with more arguments for batch mode
TODO


## How the software was tested

1) redirection from files (> works, < does not)
2) commands that do not work/are not recognized
3) tests history after some background and some foreground commands
4) interactive mode file running/input and multiple commands per line
5) tests fg and wait with currently running bg processes
TODO


## Known bugs and problem areas

1) When an invalid command is typed in and exit is called, it takes another call to exit to be able to shut down the program
2) redirecting from stdin does not work for whatever reason.... spent more than 4 hours trying to figure this out
    may be able to be fixed by making new .c file that prints "nick"?
        FIXED-- file open() to opening for reading AND writing
                 -- redirection using < (redirect file to stdin) works when infile is specified as infile.txt
TODO
