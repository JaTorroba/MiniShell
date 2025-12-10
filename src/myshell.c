#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#include "parser.h"

#define MAXJOBS 64
//current mask applied in file creation for this shell process
mode_t mask;

typedef struct{
	int id;
	pid_t pid;
	char command[1024];
	int running; //1 if running 0 if stopped
} tjob;

tjob *job_list[MAXJOBS];
int last_job = 0;

void cd(int argc, char **argv){
	char *dir;
    char buf[1024];
    if (argc == 1) {
        dir = getenv("HOME");
        if (dir == NULL) {
            fprintf(stderr, "Error: cd: could not change directory to HOME\n");
            return;
        }
    } else if (argc == 2){
        dir = argv[1];
    } else {
		fprintf(stderr, "Error: cd: invalid number of arguments, USE: cd [DIR]\n");
		return;
	}
    if (chdir(dir) != 0) {
        fprintf(stderr, "Error: cd: could not find or enter directory %s\n", dir); 
    } else {
        if (getcwd(buf, 1024) != NULL) {
            printf("%s\n", buf);
        } else {
            fprintf(stderr, "Error: cd: could not get the gurrent directory");
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
			fprintf(stderr, "Error: umask: invalid argument:%s\n", argv[1]);
			return;
		}
		if (strcmp(endptr, "\0")){
			fprintf(stderr, "Error: umask: invalid argument: %s\n", argv[1]);
			return;
		}
		mask = new_mask;
		umask(new_mask);
	}else {
		fprintf(stderr, "Error: umask: too many arguments");
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

void jobs() {
    int i;
    for (i = 0; i < last_job; i++) {
        if (job_list[i]->running == 1) {
            if (i == last_job - 1) {
                printf("[%d]+ Running\t%s\n", job_list[i]->id, job_list[i]->command);
            } else if (i == last_job - 2) {
                printf("[%d]- Running\t%s\n", job_list[i]->id, job_list[i]->command);
            } else {
                printf("[%d]  Running\t%s\n", job_list[i]->id, job_list[i]->command);
            }
        } 
        else {
            if (i == last_job - 1) {
                printf("[%d]+ Stopped\t%s\n", job_list[i]->id, job_list[i]->command);
            } else if (i == last_job - 2) {
                printf("[%d]- Stopped\t%s\n", job_list[i]->id, job_list[i]->command);
            } else {
                printf("[%d]  Stopped\t%s\n", job_list[i]->id, job_list[i]->command);
            }
        }
    }
}

void bg(char **argv){
    int i, job_id;
    int job_index = -1;

    if (argv[1] == NULL) {
        if (last_job == 0) {
            fprintf(stderr, "Error: bg: no current jobs\n");
            return;
        }
        job_index = last_job - 1;
    } else {
        job_id = atoi(argv[1]); 
        for (i = 0; i < last_job; i++) {
            if (job_list[i]->id == job_id) {
                job_index = i;
                break;
            }
        }
        if (job_index == -1) {
            fprintf(stderr, "Error: bg: command [%s] does not exist \n", argv[1]);
            return;
        }
    }
    if (job_list[job_index]->running == 1) {
        printf("bg: job [%d] is already running\n", job_list[job_index]->id);
        return;
    }
    if (kill(job_list[job_index]->pid, SIGCONT) < 0) {
        fprintf(stderr, "Error: bg: could not continue command [%d] execution", job_list[job_index]->id);
        return;
    }

    job_list[job_index]->running = 1;
    printf("[%d]+\t%s &\n", job_list[job_index]->id, job_list[job_index]->command);
}

void execute_internal(int argc, char **argv){
	char *name = argv[0];
	if (strcmp(name, "cd") == 0) {
       cd(argc, argv);        
    } else if (strcmp(name, "jobs") == 0) {
        jobs();
    } else if (strcmp(name, "bg") == 0) {
        bg(argv);
    } else if (strcmp(name, "exit") == 0) {
        exit(0);         
    } else if (strcmp(name, "umask") == 0) {
        umask_ms(argc, argv);
    }
}

void shell_sigint_handler(){
	printf("\n");
	fprintf(stdout, "msh>");
	fflush(stdout);
}

int remove_job(pid_t pid){
	int i, j, ret;
	for (i = 0; i < last_job; i++){
		if (job_list[i]->pid == pid){
			ret = job_list[i]->id;
			free(job_list[i]);
			//restructure job_list
			for(j = i; j < last_job - 1; j++){
				job_list[j] = job_list[j+1];
			}
			last_job--;
			return ret;
		}
	}
	return -1;
}

void add_job(pid_t pid, char *command_line, int running){
	size_t len;
	
	if (last_job >= MAXJOBS) { // Array overflow protection
        fprintf(stderr, "Error: reached maximum number of jobs\n");
        return;
    }
	tjob *new_job = malloc(sizeof(tjob));
	new_job->id = last_job;
	new_job->pid = pid;
	strcpy(new_job->command,command_line);

	len = strlen(new_job->command);
    if (len > 0 && new_job->command[len - 1] == '\n') {
        new_job->command[len - 1] = '\0';
    }
	new_job->running = running;
	job_list[last_job] = new_job;
	last_job++;
}

//necessary to cleanse processes executed in background
void wait_bg(){
	pid_t pid;
	int rem;

	pid = waitpid(-1, NULL, WNOHANG);
	while(pid > 0){
		rem = remove_job(pid);
		if (rem >= 0){
			printf("[%d]+ DONE\n", rem);
		}
		pid = waitpid(-1, NULL, WNOHANG);
	}
}


int main(void) {
	char buf[1024];
	tline *line;
	int i, j, error, fd, status;
	pid_t *processes;
	int **pipes;


	/*********************SIGNAL CONFIG CODE*********************/
	signal(SIGINT, shell_sigint_handler);
	signal(SIGTSTP, SIG_IGN);

	printf("msh> ");	

	while (fgets(buf, 1024, stdin)) {

		wait_bg();
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
				//Reset singal handlers for child process
				if (line->background == 0){
					signal(SIGINT, SIG_DFL);
					signal(SIGTSTP, SIG_DFL);
				} else {
					signal(SIGINT, SIG_IGN);
				}


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

		//Ignore sigint while the foreground process is in execution

		//close unused pipes in parent process to avoid blocks & free memorie
		for(i = 0; i < line->ncommands - 1; i++){
				close(pipes[i][0]);
				close(pipes[i][1]);
				free(pipes[i]);
		}
		if (line->ncommands > 1) free(pipes);

		if (line->background == 1){
			printf("[%d] %d\n", last_job, processes[line->ncommands - 1]);
			//add last process in the line to jobs
			add_job(processes[line->ncommands - 1], buf, 1);
			usleep(10000); //Cosmetic fix for printing the command output in a separate line
		} else {
			//ignore SIGINT while waiting for foreground
			signal(SIGINT, SIG_IGN);
			//wait for child processes executed in foreground
			for(i = 0; i < line->ncommands; i++){
				waitpid(processes[i], &status, WUNTRACED);

				if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
                    printf("\n"); // Clear the line
                }

				if (WIFSTOPPED(status)){
					printf("\n[%d]+ Stopped\n", last_job);
					add_job(processes[line->ncommands - 1], buf, 0);
					break;
				}
			}
			//reset handler
			signal(SIGINT, shell_sigint_handler);
		}

		free(processes);

		printf("msh> ");	
	}
			
	return 0;
}