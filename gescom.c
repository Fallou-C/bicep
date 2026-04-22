#include "gescom.h"
#include "creme.h"

 char **Mots; /* tableau mot commande */
 int Nmots;    /* nombre mot commande */
 Commande tab_commande[NBMAXC];

void user_name(char *buff,int size_buff){
	char *hostname = (char*)malloc(size_buff*sizeof(char));
        int my_uid =getuid(),host_uid = geteuid();
        gethostname(hostname,size_buff);

        if(my_uid == host_uid) snprintf(buff,size_buff,"%s@%s#>",getenv("USER"),hostname);
        else snprintf(buff,size_buff,"%s@%s$>",getenv("USER"),hostname);

        free(hostname);
}

int analyseCom(char* b){
	char *prompt_parce;
	int indice = 0;
	char ** Tab_parameter = (char**)malloc(MAX_PARAMETER*sizeof(char*));
	char* copy = strdup(b);
	bool somthing_sparse = false; //pansement car mal opti

	char parcing[3] = " \t\n";

	while( (prompt_parce=strsep(&copy,parcing)) != NULL )
	{
		if(strcmp(prompt_parce,"\0")){
//			printf("sparcing -> %s \n",prompt_parce);
			Tab_parameter[indice] = prompt_parce;
			indice ++;
			if(!somthing_sparse) somthing_sparse=true;
		}
	}
	if(!somthing_sparse) {indice++;Tab_parameter[0] = "\0";} // à opti

	Tab_parameter[indice] = NULL;
	Nmots = indice;
	Mots = Tab_parameter;

	// se debrouiller pour bien liberer les pointeurs plus tard
	free(prompt_parce);
	free(copy);
	//free(Tab_parameter);
	return 0;
}

int ajoutCom(char* nom,int (*fct)(int,char**) ) {
	Commande new_com = {nom,fct};
	for(int i=0;i<NBMAXC;i++)
	{
		if(tab_commande[i].nom == NULL)
		{
			tab_commande[i] = new_com;
			return 0;
		}
	}
	printf("trop de commande yamete\n");
	return 1; // on a plus la place
}

/*Commande interne :*/
int Sortie(int N, char *P[]) { exit(0); }
int Version(int N,char *P[]) {printf("Version %f\n",VERSION_ACUTELLE); return 0;}
int Deplacement(int N,char *P[]) {chdir(P[1]);return 0;}

int RepertoireActuelle(int N, char *P[]){
	char buff[BUFFSIZE];
	getcwd(buff,BUFFSIZE);
	printf("%s\n",buff);
	return 0;
}

int AfficheHistorique(int N, char *P[]){
	HIST_ENTRY **historique = history_list();
	int i=0;

	while(historique[i] != NULL)
	{
		printf("%s \n",historique[i]->line);
		i++;
	}
	return 0;
}

int NettoyageHistorique(int N, char *P[]){
    clear_history();
    return 0;
}

static pid_t pid = -1;

int beuip(int N, char *P[]){
	//if(N!=3) {perror("beuip : pas le bon nombre d'argument\n");return 1;}
	if(!strcmp("start",P[1]))
	{
		if (pid != -1) {
            printf("Le serveur tourne déjà (PID: %d)\n", pid);
            return 1;
        }

		if((pid=fork()) == -1){perror("fork");return 1;} /*erreur*/
		if (pid == 0) /*code fils*/
		{
			char* args_serveur[2];
            args_serveur[0] = P[0];
            args_serveur[1] = P[2];
			/*On ne garde que les arguement utile*/
			serveur(2,args_serveur);
			exit(0);
		}
		printf("Serveur lancé avec le PID %d\n", pid);
	}
	else if(!strcmp("list",P[1])){
		char* Q[2] = {"client","3"}; // on regarde la liste
		client(2,Q);
	}
	else if(!strcmp("message",P[1])){
		if(!strcmp("all",P[2])) // broadcat
		{
			char* Q[3] = {"client","5",P[3]};
			client(3,Q);
		}
		else{ //message à un individu 
			char* Q[4] = {"client","4",P[2],P[3]};
			client(4,Q);
		}
	}
	else if(!strcmp("stop",P[1])){
		if (pid == -1) {
            printf("Aucun serveur à arrêter.\n");
            return 1;
        }
		char* param_end[3];
		param_end[0] = "client";
		param_end[1] = "5";
		param_end[2] = "0";
		client(3,param_end);
		//on tue le fils
		if (kill(pid, SIGTERM) == 0) {
            // Attente pour éviter que le fils ne devienne un "zombie"
            waitpid(pid, NULL, 0); // Nettoyage du zombie
            printf("Serveur (PID %d) arrêté.\n", pid);
            pid = -1; // Réinitialisation
        }
	}
	return 0;
}

int mess(int N,char* P[]){
	//On reprend le même code que pour client
	/*
	mess 3 -> liste des gens sur le réseau
	mess 4 pseudo msg -> envoie msg au pseudo
	mess 5 msg -> broadcast msg
	*/
	client(N,P);
	return 0;
}

void majComInt(void) /* mise a jour des commandes internes */
{
	ajoutCom("exit",Sortie);
	ajoutCom("vers",Version);
	ajoutCom("cd",Deplacement);
	ajoutCom("pwd",RepertoireActuelle);
	ajoutCom("history",AfficheHistorique);
    ajoutCom("clear_history",NettoyageHistorique);
	ajoutCom("beuip",beuip);
	ajoutCom("mess",mess);
}

void listeComInt(void){
	for(int i=0;i<NBMAXC;i++)
	{
		if(tab_commande[i].nom != NULL)
		{
			printf("La %deme commande est %s\n",i+1,tab_commande[i].nom);
		}
	}
}

int execComIn(int N, char **P)
{
	for(int i=0;i<NBMAXC;i++)
	{
		if(tab_commande[i].nom != NULL)
		{
			if(!strcmp(tab_commande[i].nom,P[0])) /*On vérifie que c'est une commande interne*/
			{
				tab_commande[i].fonction(N,P);
				return 1;
			}
		}
	}
	return 0;
}

int execComExt(char **P)
{
	int pid,status;
	if((pid=fork()) == -1){perror("fork");return 1;} /*erreur*/
	if (pid == 0) /*code fils*/
	{
		if (P == NULL || P[0] == NULL || P[0][0] == '\0')
		{
			exit(0);
		}
		execvp(P[0],P);
		perror("problème execvp \n");
		exit(1); //probleme avec fct externe
	}
	else /*code padre*/
	{
		waitpid(pid,&status,0);
		return 0;
	}
}

int ExecuteCommande(char *prompt)
{
	char *prompt_parse;
        char* copy = strdup(prompt);
        while( (prompt_parse=strsep(&copy,";")) != NULL )
        {
		analyseCom(prompt_parse);
                if(!execComIn(Nmots,Mots)) execComExt(Mots);
        }
        free(prompt_parse);
        free(copy);
        return 0;
}

void handler(int S)
{
	char* msg ="\n "ANSI_COLOR_GREEN" ⣼⠀⠀⢻⠿⡿⢿⣿⣿⣿⣿⠿⢿⣿⣿⣟⣒⠶⠿⢿⣷⣷⣦⣟⣻⡿⠿⣷⣶⣭\n⠻⣄⢀⡤⠤⠴⠛⢦⣩⡭⠗⢺⡿⠯⠽⢿⣿⣿⣿⣶⣶⣤⣽⣟⣻⣿⣷⣶⣶⣦\n⠀⠹⣾⠁⣀⠀⠀⣊⠁⠀⠀⠀⠙⢦⣀⣤⠴⠛⠉⢿⡓⠶⣯⣭⣹⣿⣿⣿⣿⣿\n ⣿⡟⢹⢠⡞⣡⠀⠀⠀⠀⢀⣴⠆⠀⠀⠀⠀⠀⣻⡋⠁⠀⠙⠻⣭⣟⡒⡲\n⠀⠀⣿⠶⠾⡟⣆⢰⡄⠀⣠⠔⠋⠁⡗⠀⢀⣠⠴⠚⣉⡳⠄⠀⣠⣾⠿⠈⠙⢓"ANSI_COLOR_RESET"\n"ANSI_COLOR_YELLOW" ⣰⣿⠁"ANSI_COLOR_RESET""ANSI_COLOR_GREEN"⢀⣙⢮⡳⣥⠞⠁⠘⠄⠀⣧⣶⣋⣥⢶⣻⡭⠿⣗⣴⡋⣟⣆⣠⠾⠋"ANSI_COLOR_RESET"\n "ANSI_COLOR_YELLOW"⣧⣿⠼⣿⠛⢷⣿⣾⡇⠀⢠⡏⢉⣉⣥⣖⣋⠛⠦⢤⣀⠀"ANSI_COLOR_RESET""ANSI_COLOR_GREEN"⠀⡽⠸⣏⡀⢠⠋"ANSI_COLOR_RESET"\n"ANSI_COLOR_YELLOW"⠀⢳⣾⠀⠈⢩⢹⣿⡿⠁⠀⠁⠸⣄⠘⢯⣉⣉"ANSI_COLOR_MAGENTA"⣯⣿⣳"ANSI_COLOR_RESET""ANSI_COLOR_YELLOW"⣮⠀"ANSI_COLOR_RESET""ANSI_COLOR_GREEN"⠘⣇⣠⡬"ANSI_COLOR_RESET""ANSI_COLOR_YELLOW"⣺⣏⢣\n⠀⠈⣿⠀⠐⠀⢀⡿⠁⠀⠀⠀⠀⠘⠊⠓⠄⡉⢩⣭⠿⠀⠐⠒⡉⣯⠞⣧⢸⠈\n⣤⠤⣼⡇⠀⠀⡞⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡏⠀⢠⠇⠀⠀⡏⠀⢈⣤⡼\n⢠⠞⢉⣧⠀⠀⣇⠤⣦⣄⡆⠀⠀⠀⠀⠀⠀⠀⡸⠁⡐⠛⠂⠀⢀⡟⣀⡾⠋⠁\n⠏⢠⡞⣿⢠⣶⣞⢷⠾⠋⠀⠀⠀⠀⠀⠀⠀⡠⢁⡜⠀⠀⠀⣔⣫⣾⡿⣧⠀⠀\n⢀⡟⠀⢻⣠⣿⡾⣭⣽⣦⠀⠀⠀⠀⠀⢀⡼⢡⠏⠀⠀⠀⢸⣧⡾⠋⠀⢼⡇⠀\n⢸⠁⠀⢸⡏⢣⣝⡛⠛⠻⠧⠀⠀⠀⢀⠚⠀⠋⠀⠀⠀⣠⣿⢻⢳⢆⢀⣼⡇⠀\n⠸⡇⠀⠸⡇⠈⠌⠉⠙⠀⠀⠀⠀⠀⠍⠀⠀⠀⠀⠀⣴⠏⢿⣾⣿⣰⣿⠎⣿⠦\n⠀⢹⡄⠀⣿⢐⣀⠀⠀⠀⠀⠀⠠⠆⣀⣠⠴⠖⠛⠉⠀⠀⢀⣤⢞⣵⠃⠀⠹⣇\n⢣⡀⠙⢦⡈⠓⠮⢤⣤⣤⣤⡶⠞⠛⠉⠀⠀⠀⣀⣠⡤⠞⠋⡤⠜⠃⠀⠀⠤⠛"ANSI_COLOR_RESET"\n";
	printf("%s Daga kotowaru\n",msg);
}
