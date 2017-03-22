/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variante.h"
#include "readcmd.h"

#include <sys/types.h>
#include <sys/wait.h>

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>

typedef struct info_jobs info_jobs ;

struct info_jobs{
	char text[100];
	int pid;
	struct info_jobs *next;
};

typedef struct info_jobs* llist;

llist add(llist liste,int pid, char *args){
	printf("Ajout d'un job_info ! , de texte %s\n",args);
	struct info_jobs *new_cell = malloc(sizeof(struct info_jobs));
	strcpy(new_cell->text,args);
	new_cell->next = NULL;
	new_cell->pid = pid;

	if(liste==NULL){
		//printf("ajout d'une cellule wallah\n");
		return new_cell;
	}
	else{
		struct info_jobs *temp = liste;
		while(temp->next!=NULL){
			temp = temp->next;
		}
		temp->next=new_cell;
		return liste;
	}
}

void del(llist liste,int pid){
	if(liste!=NULL){
		struct info_jobs *prec=liste;
		struct info_jobs *temp = liste;
		while(prec->next!=NULL){
			temp = prec->next;
			if(temp->pid==pid){
				struct info_jobs *nxt=temp->next;
				prec->next = nxt;
				free(temp);
			}
		}
		temp = prec->next;
		if(prec->pid==pid){
			struct info_jobs *nxt=temp->next;
			prec->next = nxt;
			free(temp);
		}
	}
}



void display(struct info_jobs *liste){
	struct info_jobs *temp = liste;
	if (temp == NULL){
		printf(" Pas de processus en background\n");
	}else {
		while(temp->next!=NULL){
			printf(" Processus numéro : " );
			printf("%i",temp->pid);
			printf( "           " );
			printf( "%s",temp->text);
			printf("\n");
			temp=temp->next;
		}
		printf(" Processus numéro : " );
		printf("%i",temp->pid);
		printf( "           " );
		printf( temp->text);
		printf("\n");
		temp=temp->next;
	}
}

int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	printf("Not implemented yet: can not execute %s\n", line);

	/* Remove this line when using parsecmd as it will free it */
	free(line);

	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}


int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);
				llist liste = NULL;
#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		struct cmdline *l;
		char *line=0;
		int i, j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif


#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd( & line);

		/* If input stream closed, normal termination */
		if (!l) {

			terminate(0);
		}



		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");

		/* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
                        for (j=0; cmd[j]!=0; j++) {
                                printf("'%s' ", cmd[j]);
                        }
			printf("\n");
		}
		//JOBS ?
		if (strcmp(l->seq[0][0],"jobs") == 0){
			display(liste);
		}

	 //pipe
	 if (l -> seq[1] != 0){
		 pid_t pid = fork();
		 if (pid == 0){
		 	 int tuyau[2];
		 	 pipe(tuyau);
			 pid_t pid2 = fork();
			 if (pid2 == 0){
		 	 	 dup2(tuyau[0], 0);
   		 	 close(tuyau[1]); close(tuyau[0]);
			 	 execvp(l->seq[1][0], l->seq[1]);
			 }
			 dup2(tuyau[1], 1);
	   	 close(tuyau[0]); close(tuyau[1]);
			 execvp(l->seq[0][0], l->seq[0]);
		 }
	 }
	 //no pipe
	 else {
		 pid_t pid = fork();
	 	int status;
	 	char **args = l->seq[0];
		if (pid == 0){
			//printf("Processus enfant, ID : %d \n ", pid);
			int res;
			res = execvp(args[0], args);
			if (res == -1) {perror("execvp:");}
			}
		 	//printf("Processus parent, ID : %d \n ", pid);
			if (l->bg != 1
      ){
				waitpid(pid, &status, 0);
		}else{
			liste = add(liste,pid,args[0]);
		}
		}
	 }


 }
