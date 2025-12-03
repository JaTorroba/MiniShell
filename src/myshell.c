#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "parser.h"


// char *isInternal(char *name){

// }

int main(void) {
	char buf[1024];
	tline *line;
	int i;
	pid_t *processes;
	int **pipes;
	int fd;
	int error;
	//char *internalCommand;

	printf("msh> ");	

	while (fgets(buf, 1024, stdin)) {

		error = 0;

		line = tokenize(buf);
		if (line == NULL) {
			continue;
		}

		processes = malloc(sizeof(pid_t) * line->ncommands);
		pipes = (int **)malloc(sizeof(int *) * (line->ncommands - 1));

		//pipe initialization
		for (i = 0; i < line->ncommands - 1; i++) {
                pipes[i] = (int *)malloc(sizeof(int) * 2);
				if (pipe(pipes[i]) < 0){
					fprintf(stderr, "Error on initializing pipes");
					error = 1;
					break;
				}
        }

		if (error != 0) continue;

		for(i = 0; i < line->ncommands; i++){
			processes[i] = fork();
			if (processes[i] < 0){
				fprintf(stderr, "Error on fork\n");
				error = 2;
				break;
			}

			//Child process code
			if (processes[i] == 0) {

				//pipe linking & input, output, error redirections
				if (i > 0){
					dup2(pipes[i-1][0], 0); //close stdin & open pipe on reader
					if (i == line->ncommands - 1){
						if (line->redirect_output != NULL){
							fd = open(line->redirect_output, O_WRONLY);
							if (fd < 0){
								fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_output);
								break;
							}
							dup2(fd, 1);
							close(fd);
						}
						if (line->redirect_error != NULL){
							fd = open(line->redirect_error, O_WRONLY);
							if (fd < 0){
								fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_error);
								break;
							}
							dup2(fd, 2);
							close(fd);
						}	
					}
				}
				if (i < line->ncommands - 1){
					dup2(pipes[i][1], 1); //close stdout & open pipe on writer
					if (i == 0 && line->redirect_input != NULL){
						fd = open(line->redirect_input, O_RDONLY);
						if (fd < 0){
							fprintf(stderr, "Fichero: Error. No se pudo abrir %s\n", line->redirect_input);
							break;
						}
						dup2(fd, 0);
						close(fd);
					}
				}


			}
				
		}
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