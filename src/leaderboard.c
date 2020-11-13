#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "main.h"
#include "leaderboard.h"

/*
Ces macros servent a quitter la fonction envoyer_requete en cas d'erreur dans la requête
elles permettent de direction renvoyer NULL en cas d'erreur et diminue la quantité de code répété
pour la vérification de chaque erreur dans la fonction
*/

#define ERR(x, msg, r) do { \
  int retval = (x); \
  if (retval < 0) { \
    fprintf(stderr, msg); \
    close(sockfd); \
    return NULL /* or throw or whatever */; \
  } \
  if (r != NULL) { \
    (*r) = retval; \
  } \
} while (0)

#define ERRP(x, msg, r) do { \
  void *retval = (x); \
  if (retval == NULL) { \
    fprintf(stderr, msg); \
    close(sockfd); \
    return NULL /* or throw or whatever */; \
  } \
  (*r) = retval; \
} while (0)


// Extrait le code de statut de la réponse (code 200 = ok)
int status(char *reponse) {
    char *statut = strstr(reponse, " "); // Trouve le premier espace
    int res = 0;
    sscanf(statut, "%d", &res);
    return res;
}

// Envoie une requete à un serveur et renvoie la réponse du serveur
char *envoyer_requete(const char *host, int port, const char *req) {

    // Création du socket
    int sockfd;
    ERR(socket(AF_INET, SOCK_STREAM, 0), "Erreur lors de la création du socket", &sockfd);
    // Trouver l'ip du serveur par rapport à son host
    struct hostent *server;
    ERRP(gethostbyname(host), "Serveur non trouvé", &server);

    // Initialisation du socket pour qu'il se connecte au serveur
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr.s_addr,server->h_addr,server->h_length);

    // Connection au serveur
    int res;
    ERR(connect(sockfd,(struct sockaddr *)&addr,sizeof(addr)), "Erreur lors de la connection", &res);

    // Envoie de la requete tout en gérant les éventuelles paquets perdus
    int total = strlen(req);
    int envoye = 0;
    do {
        int bytes;
        ERR(send(sockfd, req + envoye, total - envoye, 0), "Erreur lors de l'envoie", &bytes);
        if (bytes == 0) break; // Connection au serveur perdue
        envoye+=bytes;
    } while (envoye < total);

    // Réponse du serveur
    total = 4096;
    char *reponse = calloc(total, sizeof(char)); // Memory leak, reponse n'est pas free en cas d'erreur dans recv
    int recu = 0;
    do {
        int bytes;
        ERR(recv(sockfd, reponse + recu, total - recu, 0), "Erreur lors de la lecture", &bytes);
        if (bytes == 0) break; // Connection au serveur perdue
        recu += bytes;
    } while (recu < total);

    // Fermeture du socket
    close(sockfd);

#ifdef DEBUG
    printf("Envoyé : %s\nReçu : %s\n", req, reponse);
#endif

    if (status(reponse) != 200) {
        free(reponse);
        return NULL;
    }

    return reponse;
}

Point centrer_texte(char *texte, Point centre, int taille) {
    Point t = taille_texte(texte, taille);
    return (Point) {centre.x - (t.x / 2), centre.y - (t.y / 2)};
}

void afficher_leaderboard(SDL_Joystick *manette) {
    afficher_message_leaderboard("Chargement du classement", 26);
    const char *get_req =
            "GET / HTTP/1.0\r\n"
            "Host: pacman-leaderboard.herokuapp.com\r\n"
            "Content-Type: application/x-www-form-urlencoded"
            "\r\n\r\n";

    //char *reponse = envoyer_requete("localhost", 3000, get_req);
    char *reponse = envoyer_requete("pacman-leaderboard.herokuapp.com", 80, get_req);
    // En cas d'erreur dans la requête
    if (reponse == NULL) {
        afficher_message_leaderboard("Erreur lors du chargement du classement", 26);
        attendre_clic();
        return;
    }

    // Récupère que le corps de la réponse sans l'en-tête
    char *body = strstr(reponse, "\r\n\r\n");

    // Efface l'écran
    dessiner_rectangle((Point){0,0}, ECRAN_W, ECRAN_H, noir);

    const int taille_titre = 46;
    afficher_texte("Joueur", taille_titre, centrer_texte("Joueur", (Point){ECRAN_W / 4, 20}, taille_titre), blanc);
    afficher_texte("Score", taille_titre, centrer_texte("Score", (Point){ECRAN_W / 2 + 300 / 2, 20}, taille_titre), blanc);
    // Point de départ du tableau du classement
    int y = 75;
    // Sépare chaque ligne du corps
    char *state;
    char *line = strtok_r(body, "\r\n", &state);
    while (line != NULL) {
        char *partie;
        // Sépare le nom du joueur et son score
        char *joueur = strtok_r(line, ":", &partie);
        char *points = strtok_r(NULL, ":", &partie);
        // Affiche la ligne
        afficher_ligne(joueur, points, y);
        line = strtok_r(NULL, "\r\n", &state);
        y += 45;
    }
    actualiser();
    while (nouvelle_touche(manette) != SDLK_q);
    free(reponse);
}

void afficher_ligne(char *joueur, char *score, int y) {
#ifdef DEBUG
    printf("joueur : %s score : %s\n", joueur, score);
#endif
    const int taille_font = 26;
    afficher_texte(joueur, taille_font, centrer_texte(joueur, (Point){ECRAN_W / 4, y}, taille_font), blanc);
    afficher_texte(score, taille_font, centrer_texte(score, (Point){ECRAN_W / 2 + 300 / 2, y}, taille_font), blanc);
}

void afficher_message_leaderboard(char *message, int font) {
    dessiner_rectangle((Point){0,0}, ECRAN_W, ECRAN_H, noir);
    afficher_texte(message, font, centrer_texte(message, (Point){ECRAN_W / 2, ECRAN_H / 2}, font), blanc);
    actualiser();
}
