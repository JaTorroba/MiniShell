#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#include "parser.h"

//current mask applied in file creation for this shell process
mode_t mask;

void cd(int argc, char **argv){
	char *dir;
    char buf[1024];
    if (argc == 1) {
        dir = getenv("HOME");
        if (dir == NULL) {
            fprintf(stderr, "Could not change directory to HOME\n");
            return;
        }
    } else if (argc == 2){
        dir = argv[1];
    } else {
		fprintf(stderr, "Invalid number of arguments, USE: cd [DIR]\n");
		return;
	}
    if (chdir(dir) != 0) {
        fprintf(stderr, "Could not find or enter directory %s\n", dir); 
    } else {
        if (getcwd(buf, 1024) != NULL) {
            printf("%s\n", buf);
        } else {
            fprintf(stderr, "Could not get the gurrent directory");
        }
    }
}

void umask_ms(int argc, char **argv){
	char *endptr;
	if (argc == 1){
		mode_t current_mask = umask(0);
		umask(current_mask);
		printf("%04o\n", current_mask);
	}else if (argc == 2){
		mode_t new_mask = (mode_t) strtoul(argv[1], &endptr, 8);
		if (endptr == argv[1]){
			fprintf(stderr, "Invalid argument:%s\n", argv[1]);
			return;
		}
		if (strcmp(endptr, "\0")){
			fprintf(stderr, "umask: Invalid argument: %s\n", argv[1]);
			return;
		}
		mask = new_mask;
		umask(new_mask);
	}else {
		fprintf(stderr, "umask: too many arguments");
		return;
	}
}

int is_internal(char *name){
	if (strcmp(name, "cd") == 0 ||
	 strcmp(name, "jobs") == 0 ||
	 strcmp(name, "exit") == 0 ||
	 strcmp(name, "bg") == 0 ||
	 strcmp(name, "umask") == 0){
		return 1;
	} 
	return 0;
}

void execute_internal(int argc, char **argv){
	char *name = argv[0];
	if (strcmp(name, "cd") == 0) {
       cd(argc, argv);        
    } else if (strcmp(name, "jobs") == 0) {
        // jobs(argv);
    } else if (strcmp(name, "bg") == 0) {
        // bg(argv);
    } else if (strcmp(name, "exit") == 0) {
        exit(0);         
    } else if (strcmp(name, "umask") == 0) {
        umask_ms(argc, argv);
    }
}

int main(void) {
	char buf[1024];
	tline *line;
	int i, j, error;;
	pid_t *processes;
	int **pipes;
	int fd;

	printf("msh> ");	

	while (fgets(buf, 1024, stdin)) {
		error = 0;

		line = tokenize(buf);
		if (line == NULL) continue;

		//Check if is an internal command
		if (line->ncommands == 1 && is_internal(line->commands[0].argv[0]) == 1){ 
			execute_internal(line->commands[0].argc, line->commands[0].argv);
			printf("msh> ");
			continue;
		}
				
		processes = malloc(sizeof(pid_t) * line->ncommands);
		pipes = (int **)malloc(sizeof(int *) * (line->ncommands - 1));

		//Create pipes & check for errors
		for (i = 0; i < line->ncommands - 1; i++) {
                pipes[i] = (int *)malloc(sizeof(int) * 2);
				if (pipe(pipes[i]) < 0){
					fprintf(stderr, "Error on initializing pipes");
					error = i;
					break;
				}
        }

		if (error != 0){
			free(processes);
			for(i = 0; i < error; i++){
				free(pipes[i]);
			}
			free(pipes);
			printf("msh> ");
			continue;
		} 


		for(i = 0; i < line->ncommands; i++){
			//Child process creation
			processes[i] = fork();
			if (processes[i] < 0) {
				fprintf(stderr, "Error on fork\n");
				break;
			}

			/*************************CHILDREN PROCESS CODE*************************/
			if (processes[i] == 0) {

				//pipe linking & input, output, error redirections
				if (i == line->ncommands - 1){
					//Standard output redirection in last process in the pipe
					if (line->redirect_output != NULL){
						fd = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, mask);
						if (fd < 0){
							fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_output);
							exit(5);
						}
						dup2(fd, 1);
						close(fd);
					}
					//Error output redirection in last process in the pipe
					if (line->redirect_error != NULL){
						fd = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC, mask);
						if (fd < 0){
							fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_error);
							exit(5);
						}
						dup2(fd, 2);
						close(fd);
					}	
				}
				//Input redirection in first process in the pipe
				if (i == 0 && line->redirect_input != NULL){
					fd = open(line->redirect_input, O_RDONLY);
					if (fd < 0){
						fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_input);
						exit(5);
					}
					dup2(fd, 0);
					close(fd);
				}
				if (i > 0){
					dup2(pipes[i-1][0], 0); //set stdin to pipe on reader
				}
				if (i < line->ncommands - 1){
					dup2(pipes[i][1], 1); //set stdout to pipe on writer
				}
				//close all pipes to avoid blocks
				for (j = 0; j < line->ncommands - 1; j++){
					close(pipes[j][0]);
					close(pipes[j][1]);
				}

				//execute the command if the command is found
				if (line->commands[i].filename != NULL){
					execvp(line->commands[i].filename, line->commands[i].argv);	
				}

				//terminate process if the command is not found
				fprintf(stderr, "Mandato: No se encuentra el mandato\n");
				exit(1);
			} 
		}

		/*************************PARENT CODE*************************/

		//close unused pipes in parent process to avoid blocks & free memorie
		for(i = 0; i < line->ncommands - 1; i++){
				close(pipes[i][0]);
				close(pipes[i][1]);
				free(pipes[i]);
		}
		if (line->ncommands > 1) free(pipes);

		//wait for child processes
		for(i = 0; i < line->ncommands; i++){
			wait(NULL);
		}

		free(processes);

		printf("msh> ");	
	}
			
	return 0;
}