# Samantha Foley
#
# CS441/541: Project 3
#
#####################################################################
#
# Type "make" or "make mysh" to compile your code
# 
# Type "make clean" to remove the executable (and any object files)
#
#####################################################################

CC=gcc
CFLAGS=-Wall -g

#
# List all of the binary programs you want to build here
# Separate each program with a single space
#
all: mysh

#
# Main shell program
#
mysh: mysh.c mysh.h support.o
	$(CC) -o mysh mysh.c support.o $(CFLAGS)
	$(CC) -o hello HelloWorld.c $(CFLAGS)
	$(CC) -o hello_name Hello_Name.c $(CFLAGS)
#
# Supporting library
#
support.o: support.h support.c
	$(CC) -c -o support.o support.c $(CFLAGS)

#
# Cleanup the files that we have created
#
clean:
	$(RM) mysh *.o
	$(RM) -rf *.dSYM

#
# Tests
#
include given-tests/Makefile
