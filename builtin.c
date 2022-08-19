#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return (strcmp(cmd, "exit") == 0);
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strcmp(cmd, "cd") == 0) {
		if (chdir(getenv("HOME")) == -1) {
			perror("Chdir error");
			exit(-1);
		}
		status = 0;
		return 1;
	}

	if (strncmp(cmd, "cd ", 3) == 0) {
		if (chdir(cmd + 3) == -1) {
			perror("Chdir error");
			exit(-1);
		}
		status = 0;
		return 1;
	}

	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") != 0)
		return 0;

	char pwd[BUFLEN];
	if (!getcwd(pwd, BUFLEN)) {
		perror("Error getcwd");
		status = 1;
	}

	printf("%s\n", pwd);
	status = 0;
	return 1;
}
