#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "parser.h"

int main(void) {
	char buf[1024];
	tline * line;
	int i;
	pid_t *processes;

	printf("msh> ");	

	while (fgets(buf, 1024, stdin)) {

		line = tokenize(buf);
		if (line == NULL) {
			continue;
		}

		printf("%d\n",line->ncommands);

		processes = malloc(sizeof(pid_t) * line->ncommands);
		for(i = 0; i < line->ncommands; i++){
			processes[i] = fork();
			if (processes[i] != 0) {
				printf("%d\n",processes[i]);
				break;
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