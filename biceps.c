#include "gescom.h"

int main(void){
	char *username = (char*)malloc(BUFFSIZE*sizeof(char));
	char *prompt = (char*)malloc(BUFFSIZE*sizeof(char));
	bool run = true;

	user_name(username,BUFFSIZE);
	majComInt();
	listeComInt();
	signal(SIGINT,handler);

	using_history();
	read_history(HIST);
	stifle_history(100);
	history_set_history_state(history_get_history_state());
	printf("Bienvenue dans le BICEPS "ANSI_COLOR_RED"ඞ"ANSI_COLOR_RESET"\n");
	while(run){
		prompt = readline(username); /*On recupére la demande de l'utilisateur*/
		if(prompt == NULL) run = false; /*On quiite le programme*/
		else
		{
			add_history(prompt);
			ExecuteCommande(prompt);
		}
	}
	printf(" "ANSI_COLOR_MAGENTA" Au revoir ദ്ദി(◝ ⩊ ◜).ᐟ  "ANSI_COLOR_RESET"\n");
	write_history(HIST);
	free(username);
	free(prompt);
	return 0;
}

//creer plusieurs fois le programmes quand execute fct externe qui abouti pas ??
