#ifndef GESCOM_H
#define GESCOM_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#define BUFFSIZE 512
#define MAX_PARAMETER 20
#define VERSION_ACUTELLE 1.0
#define NBMAXC 15 /* Nb maxi de commandes internes */
#define HIST ".historique" /*fichier de sauvegarde de l'histtorique */

typedef struct _Commande{
	char* nom;
	int (*fonction)(int argc, char** argv);
} Commande;

/* Global variables from gescom.c */


/* Function prototypes */

void user_name(char *buff,int size_buff);
int analyseCom(char* b);
int ajoutCom(char* nom,int (*fct)(int,char**) );

/* internal commands */
int Sortie(int N, char *P[]);
int Version(int N,char *P[]);
int Deplacement(int N,char *P[]);
int RepertoireActuelle(int N, char *P[]);
int AfficheHistorique(int N, char *P[]);
int NettoyageHistorique(int N, char *P[]);

void majComInt(void);
void listeComInt(void);
int execComIn(int N, char **P);
int execComExt(char **P);
int ExecuteCommande(char *prompt);

void handler(int S);

/* Les couleurs*/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#endif
