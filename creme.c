#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "creme.h"

/* Serveur */

typedef struct {
    struct in_addr ip;
    char pseudo[LBUF];
} Contact;

Contact table[255];
int nb_contacts = 0;

void formater_message(char* b, char code, char* pseudo) {
    snprintf(b, LBUF, "%cBEUIP%s", code, pseudo);
}

int est_deja_present(struct in_addr ip) {
    for (int i = 0; i < nb_contacts; i++) {
        if (table[i].ip.s_addr == ip.s_addr) return 1;
    }
    return 0;
}

int serveur(int argc, char* P[]) {
    if (argc != 2) {
        printf("Usage: %s <votre_pseudo>\n", P[0]);
        return 1;
    }

    int sid;
    struct sockaddr_in mon_addr, client_addr, dest_addr;
    char buf[LBUF + 1];
    char mon_pseudo[LBUF];
    strncpy(mon_pseudo, P[1], LBUF);
    socklen_t len = sizeof(struct sockaddr_in);

    if ((sid = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket"); return 2;
    }

    // Autoriser le broadcast 
    int b_perm = 1;
    setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &b_perm, sizeof(b_perm));

    memset(&mon_addr, 0, sizeof(mon_addr));
    mon_addr.sin_family = AF_INET;
    mon_addr.sin_port = htons(PORT);
    mon_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sid, (struct sockaddr *)&mon_addr, sizeof(mon_addr)) == -1) {
        perror("bind"); return 3;
    }

    printf("Serveur BEUIP actif sur le port %d (Pseudo: %s)\n", PORT, mon_pseudo);

    // Envoi du broadcast d'identification au démarrage
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    dest_addr.sin_addr.s_addr = inet_addr("192.168.88.255");
    formater_message(buf, '1', mon_pseudo);
    sendto(sid, buf, strlen(buf), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    while (1) {
        int n = recvfrom(sid, buf, LBUF, 0, (struct sockaddr *)&client_addr, &len);
        if (n <= 0) continue;
        buf[n] = '\0';
        char code = buf[0];

        // CODE 1 & 2 : 
        if ((code == '1' || code == '2') && strncmp(&buf[1], "BEUIP", 5) == 0) {
            if (!est_deja_present(client_addr.sin_addr) && nb_contacts < 255) {
                table[nb_contacts].ip = client_addr.sin_addr;
                strncpy(table[nb_contacts].pseudo, &buf[6], LBUF);
                nb_contacts++;
                #ifdef TRACE
                printf("[TRACE] Nouveau contact : %s [%s]\n", &buf[6], inet_ntoa(client_addr.sin_addr));
                #endif
            }
            if (code == '1') {
                formater_message(buf, '2', mon_pseudo);
                sendto(sid, buf, strlen(buf), 0, (struct sockaddr *)&client_addr, len);
            }
        }
        // CODE 3 : Liste 
        else if (code == '3' && client_addr.sin_addr.s_addr == inet_addr("127.0.0.1")) {
            printf("\n--- Liste des connectés (%d) ---\n", nb_contacts);
            for (int i = 0; i < nb_contacts; i++) {
                printf("%d. %s [%s]\n", i + 1, table[i].pseudo, inet_ntoa(table[i].ip));
            }
            sendto(sid, "Liste affichée", 15, 0, (struct sockaddr *)&client_addr, len);
        }
        // CODE 4 Message 
        else if (code == '4' && client_addr.sin_addr.s_addr == inet_addr("127.0.0.1")) {
            char *target = &buf[1];
            char *msg_txt = target + strlen(target) + 1;
            int found = 0;
            for (int i = 0; i < nb_contacts; i++) {
                if (strcmp(table[i].pseudo, target) == 0) {
                    char s_buf[LBUF];
                    snprintf(s_buf, LBUF, "%c%s", '9', msg_txt);
                    struct sockaddr_in d; d.sin_family = AF_INET; d.sin_port = htons(PORT); d.sin_addr = table[i].ip;
                    sendto(sid, s_buf, strlen(msg_txt) + 2, 0, (struct sockaddr *)&d, sizeof(d));
                    found = 1; break;
                }
            }
            char* rep = found ? "Message envoyé !" : "Pseudo introuvable.";
            sendto(sid, rep, strlen(rep), 0, (struct sockaddr *)&client_addr, len);
        }

		else if (code == '5' && client_addr.sin_addr.s_addr == inet_addr("127.0.0.1")) {
            char *msg_txt = &buf[1];
            char s_buf[LBUF];
            snprintf(s_buf, LBUF+5, "%c%s", '9', msg_txt);

            for (int i = 0; i < nb_contacts; i++) {
                struct sockaddr_in d;
                memset(&d, 0, sizeof(d));
                d.sin_family = AF_INET;
                d.sin_port = htons(PORT);
                d.sin_addr = table[i].ip;
                
                sendto(sid, s_buf, strlen(s_buf), 0, (struct sockaddr *)&d, sizeof(d));
            }
            
            char* rep = "Message diffusé à tous.";
            sendto(sid, rep, strlen(rep), 0, (struct sockaddr *)&client_addr, len);
        }

        //CODE 9 : Réception 
        else if (code == '9') {
    		char *pseudo_expediteur = "Inconnu";
    		for (int i = 0; i < nb_contacts; i++) {
        		if (table[i].ip.s_addr == client_addr.sin_addr.s_addr) {
            		pseudo_expediteur = table[i].pseudo;
            		break;
        		}
    		}
    		printf("\nMessage de %s : %s\n", pseudo_expediteur, &buf[1]);
}
    }
    return 0;
}

/* Client */

int client(int argc, char* P[]) {
    if (argc < 2) {
        printf("Usage: %s <code_cmd> [params...]\n", P[0]);
        return 1;
    }

    int sid = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9998);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    struct timeval tv = {2, 0};
    setsockopt(sid, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buffer[LBUF];
    int len = 0;
    char code = P[1][0];

    buffer[0] = code;
    memcpy(&buffer[1], "BEUIP", 5);

    if (code == '3') { // Liste
        len = 1;
    } 
    else if (code == '4' && argc >= 4) { // Message privé 
        strcpy(&buffer[1], P[2]);
        int p_len = strlen(P[2]);
        strcpy(&buffer[1 + p_len + 1], P[3]);
        len = 1 + p_len + 1 + strlen(P[3]);
    }

    else if (code == '5' && argc >= 3) { // Message à tout le monde 
        strncpy(&buffer[1], P[2], LBUF - 2); 
        len = 1 + strlen(&buffer[1]);
    }

    if (sendto(sid, buffer, len, 0, (struct sockaddr *)&serv, sizeof(serv)) > 0) {
        char ack[LBUF];
        int n = recvfrom(sid, ack, LBUF, 0, NULL, NULL);
        if (n > 0) {
            ack[n] = '\0';
            printf("[SERVEUR] %s\n", ack);
        } else {
            printf("[ERREUR] Aucune preuve reçue du serveur.\n");
        }
    }

    return 0;
}