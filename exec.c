#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int idx = block_contains(eargv[i], '=');
		char key[BUFLEN];
		char value[BUFLEN];

		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, idx);


		int f = fork();


		if (f > 0) {
			// Father
			wait(NULL);

		}

		else if (f == 0) {
			// Son
			if ((setenv(key, value, 1)) ==
			    -1) {  // REPLACE parameter of setenv (last one) is
				   // nonzero, meaning overwrite an existing value.
				perror("Error env variable set");
				exit(-1);
			}


		} else {
			perror("Fork error");
			exit(-1);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (flags == O_CREAT | O_WRONLY) {
		int open_val = open(file, flags, S_IWUSR | S_IRUSR);
		if (open_val == -1) {
			perror("error open");
			exit(-1);
		}
		return open_val;
	} else {
		int open_val = open(file, flags);
		if (open_val == -1) {
			perror("error open");
			exit(-1);
		}
		return open_val;
	}
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		//
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);

		if (execvp(e->argv[0], e->argv) == -1) {
			perror("Error in execvp");
			exit(-1);
		}
		break;

	case BACK: {
		// runs a command in background
		//
		b = (struct backcmd *) cmd;


		pid_t i = fork();
		if (i < 0) {
			// Father
			waitpid(i, NULL, WNOHANG);
		}

		else if (i == 0) {
			// Son
			exec_cmd(b->c);

		} else {
			perror("Fork error");
			exit(-1);
		}

		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//
		// Your code here
		r = (struct execcmd *) cmd;

		if (strlen(r->out_file) > 0) {
			int fd_out = open_redir_fd(r->out_file,
			                           O_CREAT | O_WRONLY |
			                                   O_TRUNC | O_CLOEXEC);
			if (dup2(fd_out, STDOUT_FILENO) == -1) {
				perror("Dup2 error");
				exit(-1);
			}
		}

		if (strlen(r->in_file) > 0) {
			int fd_in =
			        open_redir_fd(r->in_file, O_RDONLY | O_CLOEXEC);
			if (dup2(fd_in, STDIN_FILENO) == -1) {
				perror("Dup2 error");
				exit(-1);
			}
		}

		if (strlen(r->err_file) > 0) {
			if (r->err_file[0] == '&') {
				if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
					perror("Dup2 error");
					exit(-1);
				}

			} else {
				int fd_err = open_redir_fd(r->err_file,
				                           O_CREAT | O_WRONLY |
				                                   O_TRUNC |
				                                   O_CLOEXEC);

				if (dup2(fd_err, STDERR_FILENO) == -1) {
					perror("Dup2 error");
					exit(-1);
				}
			}
		}

		r->type = EXEC;
		exec_cmd((struct cmd *) r);
		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;

		int fds[2];
		int pipep = pipe(fds);
		if (pipep < 0) {
			perror("Error en pipe");
			exit(-1);
		}

		// left
		int left_fork = fork();


		if (left_fork == 0) {
			// Son
			close(fds[READ]);
			if (dup2(fds[WRITE], STDOUT_FILENO) == -1) {
				perror("Dup2 error");
				exit(-1);
			}
			close(fds[WRITE]);

			exec_cmd(p->leftcmd);
			free_command(parsed_pipe);


		} else if (left_fork == -1) {
			perror("Fork error");
			exit(-1);
		}


		// right
		int right_fork = fork();


		if (right_fork == 0) {
			// Son
			close(fds[WRITE]);
			if (dup2(fds[READ], STDIN_FILENO) == -1) {
				perror("Dup2 error");
				exit(-1);
			}
			close(fds[READ]);
			exec_cmd(p->rightcmd);


		} else if (right_fork == -1) {
			perror("Fork error");
			exit(-1);
		}


		close(fds[READ]);
		close(fds[WRITE]);

		// Fathers wait children

		if (left_fork > 0) {
			waitpid(left_fork, NULL, 0);
		}
		if (right_fork > 0) {
			waitpid(right_fork, NULL, 0);
		}


		break;
	}
	}
}
