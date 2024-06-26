/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#include <sys/types.h>
#if USE_GUILE == 1
#include <libguile.h>

/* Structure returned by parsecmd() */

typedef struct t_cell{
	pid_t p;
	struct t_cell * next;
} cell;

typedef struct t_list{
	cell *head;
} list; 

list *new_list(){
	list *l = (list *)malloc(sizeof(list));
	l->head = NULL;

	return l;
}

void add_cell(pid_t p, list *l){

	cell *c = (cell*)malloc(sizeof(cell));
	c->p = p;
	c->next = l->head;
	l->head = c;

	printf("cell added");
}

int remove_cell(pid_t p, list *l){

	for (cell **c = &(l->head); (*c) != NULL; c = &((*c)->next))
	{
		if ((*c)->p == p)
		{
			*c = (*c)->next;
		}
		
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


		list *pids = new_list();
		for (i = 0; l->seq[i]!=0; i++)
		{
			char **cmd = l->seq[0];
	
			pid_t pid = fork();
			int status;
			if (strcmp(cmd[0],"jobs") == 0)
			{
				cell **first =  &(pids->head);
				cell *c = pids->head;
				while (c != NULL)
				{
					printf("%d", c->p);
					waitpid(c->p,&status,0);
					if (WIFEXITED(status)){
						*first = c->next;
						free(c);
						c = *first;
					}else{
						printf("pid %d\n", c->p);
						first = &(c->next);
						c = c->next;
					}

				}
			}
			
			else if (pid == 0)
			{

				execvp(cmd[0], cmd);
				
			}else{
				add_cell(pid, l);
				if(!(l->bg)){
					waitpid(pid, &status,0);
				}

			}

		}

	}

}
