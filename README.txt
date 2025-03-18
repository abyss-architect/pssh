Project 2
Justyn Rosinski
14517227

How To Compile & Run
====================
$ make
$ ./pssh

Description
===========
pssh.c - A basic shell that allows for command line execution of both programs
	and builtin commands. This shell also supports file redirection of both the
	output and input. This shell can pipe multiple commands together, allowing
	output from one command to be redirected to the input of another. To exit
	the shell, use the "exit" command.

builtin.c - A program containing helper functions that handle builtin commands.
	The only builtin commands are "exit" and "which".

parse.c - A program that parses the command line input, and generates a Parse
	struct to easily get the commands, arguments, and redirections that a
	command line has. This Parse struct also contains a flag for syntax errors.

jobs.c - An API for everything to deal with the job management system. This API
	allows for adding, removing, setting jobs as foreground, and printing
	information on jobs.

utils.c - An API for utilities such as a unique and lowest ID generator. This
	generator is used by the jobs.c API to assign new jobs to the lowest, unused
	ID.
