/* librairie standard ... */
#include <stdlib.h>
/* pour getopt */
#include <unistd.h>
/* déclaration des types de base */
#include <sys/types.h>
/* constantes relatives aux domaines, types et protocoles */
#include <sys/socket.h>
/* constantes et structures propres au domaine UNIX */
#include <sys/un.h>
/* constantes et structures propres au domaine INTERNET */
#include <netinet/in.h>
/* structures retournées par les fonctions de gestion de la base de données du réseau */
#include <netdb.h>
/* pour les entrées/sorties */
#include <stdio.h>
/* pour la gestion des erreurs */
#include <errno.h>
/* pour memcpy, memset */
#include <string.h>

typedef struct lettre {
    char *contenu;
    struct lettre *suiv;
} lettre_t;

typedef struct bal {
    int id;
    lettre_t *tete;
    lettre_t *queue;
    struct bal *suiv;
} bal_t;

bal_t *liste_bal = NULL;

int main (int argc, char **argv)
{
    int c;
    int udp = 0;
    int port;
    int sock;
    int lg_mess = 30;
    int bal = 0;
    int emetteur = -1;
    int recepteur = -1;
    int nb_message = -1;
    int source = -1;

    extern char *optarg;
    extern int optind;

    void construire_message (char *message, int num_msg, int lg) {
        int i;
        sprintf(message, "%5d", num_msg % 100000);
        char motif = 'a' + (num_msg % 26);

        for (i = 5; i < lg; i++)
            message[i] = motif;
    }

    void afficher_message (char *message, int lg) {
        int i;
        printf("message construit : ");
        for (i = 0; i < lg; i++)
            printf("%c", message[i]);
        printf("\n");
    }

    bal_t *chercher_ou_creer_bal(int id) {
        bal_t *cour = liste_bal;

        while (cour != NULL) {
            if (cour->id == id)
                return cour;
            cour = cour->suiv;
        }

        bal_t *nouvelle = (bal_t *)malloc(sizeof(bal_t));
        nouvelle->id   = id;
        nouvelle->tete = NULL;
        nouvelle->queue = NULL;
        nouvelle->suiv  = liste_bal;
        liste_bal = nouvelle;

        return nouvelle;
    }

    void ajouter_lettre(bal_t *ma_bal, char *message_recu, int lg_mess) {
        lettre_t *nouvelle_lettre = malloc(sizeof(lettre_t));
        nouvelle_lettre->contenu = malloc(lg_mess);
       
        memcpy(nouvelle_lettre->contenu, message_recu, lg_mess);
        nouvelle_lettre->suiv = NULL;

        if (ma_bal->tete == NULL) {
            ma_bal->tete = nouvelle_lettre;
            ma_bal->queue = nouvelle_lettre;
        } else {
            ma_bal->queue->suiv = nouvelle_lettre;
            ma_bal->queue = nouvelle_lettre;
        }
    }

    int extraire_lettre(bal_t *ma_bal, char *buffer_sortie, int lg_mess) {
        if (ma_bal->tete == NULL) {
            return 0;
        }

        lettre_t *lettre_a_lire = ma_bal->tete;
        memcpy(buffer_sortie, lettre_a_lire->contenu, lg_mess);

        ma_bal->tete = lettre_a_lire->suiv;
        if (ma_bal->tete == NULL) {
            ma_bal->queue = NULL;
        }  

        free(lettre_a_lire->contenu);
        free(lettre_a_lire);
        return 1;
    }

    while ((c = getopt(argc, argv, "psn:l:ue:br:")) != -1) {
        switch (c) {
        case 'p':
            if (source == 1) {
                printf("usage: cmd [-p|-s][-n ##]\n");
                exit(1);
            }
            source = 0;
            break;

        case 'e':  
            emetteur = atoi(optarg);
            break;

        case 'b':  
            bal = 1;
            break;

        case 'r':  
            recepteur = atoi(optarg);
            break;

        case 's':
            if (source == 0) {
                printf("usage: cmd [-p|-s][-n ##]\n");
                exit(1);
            }
            source = 1;
            break;

        case 'n':
            nb_message = atoi(optarg);
            break;

        case 'u':  
            udp = 1;
            break;
       
        case 'l':  
            lg_mess = atoi(optarg);
            break;

        default:
            printf("usage: cmd [-p|-s][-n ##]\n");
            break;
        }
    }if (optind >= argc) {
        printf("Erreur : Il manque l'adresse IP ou le numéro de port à la fin \n");
        printf("Exemple serveur : ./tsock -p 8000\n");
        printf("Exemple client  : ./tsock -s localhost 8000\n");
        exit(1);
    }
    if (nb_message != -1) {
        if (source == 1)
            printf("nb de tampons à envoyer : %d\n", nb_message);
        else
            printf("nb de tampons à recevoir : %d\n", nb_message);
    } else {
        if (source == 1) {
            nb_message = 10;
            printf("nb de tampons à envoyer = 10 par défaut\n");
        } else {
            printf("nb de tampons à envoyer = infini\n");
        }
    }
    if (bal == 1) {

        port = atoi(argv[optind]);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in adr_local;
           
        memset((char *)&adr_local, 0, sizeof(adr_local));
        adr_local.sin_family = AF_INET;
        adr_local.sin_port = htons(port);
        adr_local.sin_addr.s_addr = INADDR_ANY;

        bind(sock, (struct sockaddr*)&adr_local, sizeof(adr_local));
        listen(sock, 5);

        while (1) {
            int sock_com;
            struct sockaddr_in adr_client;
            socklen_t lg_adr_client = sizeof(adr_client);
            sock_com = accept(sock, (struct sockaddr *)&adr_client, &lg_adr_client);

            int type_client;
            int id_bal;

            int n = read(sock_com, &type_client, sizeof(int));
                if (n <= 0) {
                    close(sock_com);
                    continue; 
                    }
            read(sock_com, &id_bal, sizeof(int));

            bal_t *ma_bal = chercher_ou_creer_bal(id_bal);

            if (type_client == 1) {
                char *message_recu = malloc(lg_mess);

                int recu;
                while ((recu = read(sock_com, message_recu, lg_mess)) > 0) {
                    ajouter_lettre(ma_bal, message_recu, lg_mess);
                }
                free(message_recu);
                close(sock_com);

            } else if (type_client == 2) {
                char *message = malloc(lg_mess);

                while (extraire_lettre(ma_bal, message, lg_mess)) {
                    write(sock_com, message, lg_mess);
                }
                free(message);
                close(sock_com);
            }
        }

    } else if (emetteur != -1) {

        port = atoi(argv[optind+1]);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        struct hostent *hp;
        struct sockaddr_in adr_dest;

        memset((char *)&adr_dest, 0, sizeof(adr_dest));
        adr_dest.sin_family = AF_INET;
        adr_dest.sin_port   = htons(port);
        hp = gethostbyname(argv[optind]);

        memcpy((char *)&(adr_dest.sin_addr.s_addr), hp->h_addr, hp->h_length);

        connect(sock, (struct sockaddr*)&adr_dest, sizeof(adr_dest));

        int type_client = 1;

        write(sock, &type_client, sizeof(int));
        write(sock, &emetteur, sizeof(int));

        char *message = malloc(lg_mess);

        for (int i = 0; i < nb_message; i++) {
            construire_message(message, i, lg_mess);
            write(sock, message, lg_mess);
            afficher_message(message, lg_mess);
        }
        free(message);
        close(sock);

    } else if (recepteur != -1) {

        port = atoi(argv[optind + 1]);

        sock = socket(AF_INET, SOCK_STREAM, 0);

        struct hostent *hp;
        struct sockaddr_in adr_dest;

        memset((char *)&adr_dest, 0, sizeof(adr_dest));
        adr_dest.sin_family = AF_INET;
        adr_dest.sin_port   = htons(port);

        hp = gethostbyname(argv[optind]);
        memcpy((char *)&(adr_dest.sin_addr.s_addr), hp->h_addr, hp->h_length);

        connect(sock, (struct sockaddr *)&adr_dest, sizeof(adr_dest));

        int type_client = 2;
        write(sock, &type_client, sizeof(int));
        write(sock, &recepteur, sizeof(int));

        char *message = malloc(lg_mess);
        int nb_lettres = 0;

        int recu;
        while ((recu = read(sock, message, lg_mess)) > 0) {
            nb_lettres++;
            for (int i = 0; i < recu; i++)
                printf("%c", message[i]);
            printf("\n");
        }
        free(message);
        close(sock);

    } else if (source == 1) {

        printf("on est dans le source\n");
        port = atoi(argv[optind+1]);

        if (udp == 1) {

            sock = socket(AF_INET, SOCK_DGRAM, 0);
            struct hostent *hp;
            struct sockaddr_in adr_dest;

            memset((char *)&adr_dest, 0, sizeof(adr_dest));
            adr_dest.sin_family = AF_INET;
            adr_dest.sin_port   = htons(port);
            hp = gethostbyname(argv[optind]);

            memcpy((char *)&(adr_dest.sin_addr.s_addr), hp->h_addr, hp->h_length);

            char *message = malloc(lg_mess);

            for (int i = 0; i < nb_message; i++) {
                construire_message(message, i, lg_mess);
                sendto(sock, message, lg_mess, 0, (struct sockaddr *)&adr_dest, sizeof(adr_dest));
                afficher_message(message, lg_mess);
            }
            free(message);
            close(sock);
       
        } else {

            sock = socket(AF_INET, SOCK_STREAM, 0);
            struct hostent *hp;
            struct sockaddr_in adr_dest;

            memset((char *)&adr_dest, 0, sizeof(adr_dest));
            adr_dest.sin_family = AF_INET;
            adr_dest.sin_port   = htons(port);
            hp = gethostbyname(argv[optind]);
            memcpy((char *)&(adr_dest.sin_addr.s_addr), hp->h_addr, hp->h_length);

            connect(sock, (struct sockaddr*)&adr_dest, sizeof(adr_dest));

            char *message = malloc(lg_mess);

            for (int i = 0; i < nb_message; i++) {
                construire_message(message, i, lg_mess);
                write(sock, message, lg_mess);
                afficher_message(message, lg_mess);
            }
            free(message);
            close(sock);
        }

    } else {  

        printf("on est dans le puits\n");
        port = atoi(argv[optind]);

        if (udp == 1) {

            sock = socket(AF_INET, SOCK_DGRAM, 0);
            struct sockaddr_in adr_local;
           
            memset((char *)&adr_local, 0, sizeof(adr_local));
            adr_local.sin_family = AF_INET;
            adr_local.sin_port = htons(port);
            adr_local.sin_addr.s_addr = INADDR_ANY;

            bind(sock, (struct sockaddr*)&adr_local, sizeof(adr_local));

            char *message = malloc(lg_mess);
            int reception = 0;

            while (nb_message == -1 ||reception < nb_message) {
                int recu = recvfrom(sock, message, lg_mess, 0, NULL, NULL);
                reception++;
                afficher_message(message, lg_mess);
            }
            free(message);

        } else {

            sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in adr_local;
           
            memset((char *)&adr_local, 0, sizeof(adr_local));
            adr_local.sin_family      = AF_INET;
            adr_local.sin_port        = htons(port);
            adr_local.sin_addr.s_addr = INADDR_ANY;

            bind(sock, (struct sockaddr*)&adr_local, sizeof(adr_local));
            listen(sock, 5);

            int sock_com;
            struct sockaddr_in adresse_client;
            socklen_t lg_adresse_client = sizeof(adresse_client);
            sock_com = accept(sock, (struct sockaddr *)&adresse_client, &lg_adresse_client);

            char *message = malloc(lg_mess);
            int reception = 0;

            int n_read;
            while (1) {
                n_read = read(sock_com, message, lg_mess);
            if (n_read <= 0) {
                break; 
            }
            reception++;
            afficher_message(message, lg_mess);

            if (nb_message != -1 && reception >= nb_message) {
                break;
                }
            }
            free(message);
        }  
    }

    return 0;
}