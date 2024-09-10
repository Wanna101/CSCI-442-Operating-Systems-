/**
 * Name: David Young
 * CWID: 10891350
 * Project 2 D2
 * Date: 16 October 2022
 */ 

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "dispatcher.h"
#include "shell_builtins.h"
#include "parser.h"


static void prepare_execvp(struct command *pipeline, int pipe_fd[2], int read_fd, int write_fd, int last_fd)
{
	// if the input is not equal to NULL, set as STDIN_FILENO by dup2
	if (pipeline->input_filename != NULL) {
		int input_fd = open(pipeline->input_filename, O_RDONLY);
		// checks to see if open failed
		if ((input_fd) < 0) {
			perror(pipeline->input_filename);
			exit(EXIT_FAILURE);
		}
		if (dup2(input_fd, STDIN_FILENO) < 0) {
			exit(EXIT_FAILURE);
		}
		close(input_fd);
	}

	// - checks to make sure it is not the first command
	// - needs to be in front of the switch in order for the right side of the pipe
	// to read the data in execvp 
    if (last_fd != STDIN_FILENO) {
        if (dup2(last_fd, STDIN_FILENO) < 0) {
            fprintf(stderr, "dup2 failed for last_fd\n");
            exit(EXIT_FAILURE);
        }
        close(last_fd);
    }

    // switch for different output types
	switch (pipeline->output_type) {
		case COMMAND_OUTPUT_STDOUT:
			// COMMAND_OUTPUT_STDOUT
			break;
		case COMMAND_OUTPUT_FILE_TRUNCATE:
			// COMMAND_OUTPUT_FILE_TRUNCATE
			if (pipeline->output_filename != NULL) {
				// read + write permissions, allows for creation of new files
				int trunc_fd = open(pipeline->output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				// check for failure to open
				if ((trunc_fd) < 0) {
					perror(pipeline->output_filename);
					exit(EXIT_FAILURE);
				}
				if (dup2(trunc_fd, STDOUT_FILENO) < 0) {
					exit(EXIT_FAILURE);
				}
				close(trunc_fd);
			}
			break;
		case COMMAND_OUTPUT_FILE_APPEND:
			// COMMAND_OUTPUT_FILE_APPEND
			if (pipeline->output_filename != NULL) {
				// read + write permissions, allows for creation of new files
				int append_fd = open(pipeline->output_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
				// check for failure to open
				if ((append_fd) < 0) {
					perror(pipeline->output_filename);
					exit(EXIT_FAILURE);
				}
				if (dup2(append_fd, STDOUT_FILENO) < 0) {
					exit(EXIT_FAILURE);
				}
				close(append_fd);
			}
			break;
		case COMMAND_OUTPUT_PIPE:
			// COMMAND_OUTPUT_PIPE					
            // checks to make sure it is not the last command
            if (pipeline->pipe_to != NULL) {
                if (dup2(pipe_fd[write_fd], STDOUT_FILENO) < 0) {
                    fprintf(stderr, "dup2 failed for pipe_fd\n");
                    exit(EXIT_FAILURE);
                } 
                close(pipe_fd[write_fd]);
            }
			break;
	}
}

/**
 * dispatch_external_command() - run a pipeline of commands
 *
 * @pipeline:   A "struct command" pointer representing one or more
 *              commands chained together in a pipeline.  See the
 *              documentation in parser.h for the layout of this data
 *              structure.  It is also recommended that you use the
 *              "parseview" demo program included in this project to
 *              observe the layout of this structure for a variety of
 *              inputs.
 *
 * Note: this function should not return until all commands in the
 * pipeline have completed their execution.
 *
 * Return: The return status of the last command executed in the
 * pipeline.
 */
static int dispatch_external_command(struct command *pipeline)
{
	/*
	 * Note: this is where you'll start implementing the project.
	 *
	 * It's the only function with a "TODO".  However, if you try
	 * and squeeze your entire external command logic into a
	 * single routine with no helper functions, you'll quickly
	 * find your code becomes sloppy and unmaintainable.
	 *
	 * It's up to *you* to structure your software cleanly.  Write
	 * plenty of helper functions, and even start making yourself
	 * new files if you need.
	 *
	 * For D1: you only need to support running a single command
	 * (not a chain of commands in a pipeline), with no input or
	 * output files (output to stdout only).  In other words, you
	 * may live with the assumption that the "input_file" field in
	 * the pipeline struct you are given is NULL, and that
	 * "output_type" will always be COMMAND_OUTPUT_STDOUT.
	 *
	 * For D2: you'll extend this function to support input and
	 * output files, as well as pipeline functionality.
	 *
	 * Good luck!
	 */
	
 	int pipe_fd[2];
    int read_fd = 0;
    int write_fd = 1;
    int last_fd = STDIN_FILENO;
    pid_t pid;

    while(1) {

    	// check for pipe
        if (pipeline->output_type == COMMAND_OUTPUT_PIPE && pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // set cmd and fork
		char* cmd = pipeline->argv[0];
		pid = fork();

		if (pid < 0) {				// Fork Failure		
			fprintf(stderr, "Unable to execute command. Fork failed.\n");
			return -1;
		} else if (pid == 0) {		// Code executed by the child
			
			// prepares the input and output depending on the command
			prepare_execvp(pipeline, pipe_fd, read_fd, write_fd, last_fd);

			// execute cmd and give filepath (pipeline->argv)
			execvp(cmd, pipeline->argv);

			// command not found, code 2
			if (errno == ENOENT) {
				fprintf(stderr, "%s: command not found\n", cmd);
			}
			// permission error, code 13
			else if (errno == EACCES) {
				fprintf(stderr, "-shell: %s: Permission denied\n", cmd);
			}
			// had to put exit here otherwise exit twice on prompt
			exit(-1);
		} else {					// Code executed by the parent

			int status;
			// stall for child
			if (waitpid(pid, &status, 0) == -1) {
				fprintf(stderr, "Failed waitpid call.\n");
				return -1;
			} else {
				// checks for output type if it is COMMAND_OUTPUT_PIPE
				// so it can properly close last_fd and pipe_fd
            	if (pipeline->output_type == COMMAND_OUTPUT_PIPE) {
            		if (last_fd != STDIN_FILENO) close(last_fd);                // Close unused read end
            		close(pipe_fd[write_fd]);               					// Reader will see EOF
            		last_fd = pipe_fd[read_fd];             					// Save read pipe for child
            		if (pipeline->pipe_to != NULL) {
            			// advance pipeline to "next" (similar to linked list)
            			pipeline = pipeline->pipe_to;
            			continue;
            		}
            	}

            	if (last_fd != STDIN_FILENO) close(last_fd);                	// Close unused read end
            	close(pipe_fd[read_fd]);
				if (status != 0) {
					return -1;
				} else {
					return 0;
				}
			}
		}
	}
	
	return -1;						// theoretically, the program should never come here...	
}

/**
 * dispatch_parsed_command() - run a command after it has been parsed
 *
 * @cmd:                The parsed command.
 * @last_rv:            The return code of the previously executed
 *                      command.
 * @shell_should_exit:  Output parameter which is set to true when the
 *                      shell is intended to exit.
 *
 * Return: the return status of the command.
 */
static int dispatch_parsed_command(struct command *cmd, int last_rv,
				   bool *shell_should_exit)
{
	/* First, try to see if it's a builtin. */
	for (size_t i = 0; builtin_commands[i].name; i++) {
		if (!strcmp(builtin_commands[i].name, cmd->argv[0])) {
			/* We found a match!  Run it. */
			return builtin_commands[i].handler(
				(const char *const *)cmd->argv, last_rv,
				shell_should_exit);
		}
	}

	/* Otherwise, it's an external command. */
	return dispatch_external_command(cmd);
}

int shell_command_dispatcher(const char *input, int last_rv,
			     bool *shell_should_exit)
{
	int rv;
	struct command *parse_result;
	enum parse_error parse_error = parse_input(input, &parse_result);

	if (parse_error) {
		fprintf(stderr, "Input parse error: %s\n",
			parse_error_str[parse_error]);
		return -1;
	}

	/* Empty line */
	if (!parse_result)
		return last_rv;

	rv = dispatch_parsed_command(parse_result, last_rv, shell_should_exit);
	free_parse_result(parse_result);
	return rv;
}
