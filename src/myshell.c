#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#include "parser.h"

char *wd;

void cd(int argc, char **argv){
	//TODO check if de directory exists
	if (argc == 0){
		//wd = 
	} else {
		wd = argv[0];
		printf("%s",wd);
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
        // exit_ms();         
    } else if (strcmp(name, "umask") == 0) {
        // umask_ms(argc, argv);
    }
}

int main(void) {
	char buf[1024];
	tline *line;
	int i, j, error;
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

		//pipe initialization
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
			processes[i] = fork();
			if (processes[i] < 0){
				fprintf(stderr, "Error on fork\n");
				error = 1;
				break;
			}

			//Child process code
			if (processes[i] == 0) {

				//pipe linking & input, output, error redirections
				if (i == line->ncommands - 1){
					if (line->redirect_output != NULL){
						fd = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (fd < 0){
							fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_output);
							exit(5);
						}
						dup2(fd, 1);
						close(fd);
					}
					if (line->redirect_error != NULL){
						fd = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (fd < 0){
							fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_error);
							exit(5);
						}
						dup2(fd, 2);
						close(fd);
					}	
				}
				if (i > 0){
					dup2(pipes[i-1][0], 0); //set stdin to pipe on reader
				}
				if (i < line->ncommands - 1){
					dup2(pipes[i][1], 1); //set stdout to pipe on writer
				}
				if (i == 0 && line->redirect_input != NULL){
					fd = open(line->redirect_input, O_RDONLY);
					if (fd < 0){
						fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_input);
						exit(5);
					}
					dup2(fd, 0);
					close(fd);
				}

				for (j = 0; j < line->ncommands - 1; j++){
					close(pipes[j][0]);
					close(pipes[j][1]);
				}

				execvp(line->commands[i].filename, line->commands[i].argv);

				fprintf(stderr, "Mandato: No se encuentra el mandato");
				exit(1);
			} 
		}

		for(i = 0; i < line->ncommands - 1; i++){
				close(pipes[i][0]);
				close(pipes[i][1]);
				free(pipes[i]);
		}
		if (line->ncommands > 1) free(pipes);


		for(i = 0; i < line->ncommands; i++){
			wait(NULL);
		}

		free(processes);

		printf("msh> ");	
	}
			
	return 0;
}


/*
if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		} 
		for (i = 0; i < line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			for (j = 0; j <line->commands[i].argc; j++) {
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}
		}
*/