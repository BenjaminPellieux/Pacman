#include "entite.h"

Entite nouvelle_entite(Pos pos,Pos pos_init, TypeEntite type) {
    Etat etat = {
        .direction = DIR_INCONNUE,
        .prochaine_direction = 0,
        .score = 0,
        .fuite = 0,
        .nb_vie = 3,
    };
    Entite result = {
        .pos = pos,
        .pos_init = pos_init,
        .type = type,
        .etat = etat,
        .animation_time = 0,
    };

    if (type == ENTITE_FANTOME)
        result.nombre_frames = 1;
    else 
        result.nombre_frames = 2;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < result.nombre_frames; j++) {
            result.sprite[i][j] = charger_sprite(type, i, j);
        }
    }
    return result;
}

// Premier tableau correspond au type de l'entité, deuxiemes aux sprites des directions et troisiemes aux
// frames de l'animation (seul pacman a une animation mais cela rend le jeu plus extensible facilement)
const char *entites_sprites_path[][4][2] = {
    {
        {"data/sprites/pacman00.bmp", "data/sprites/pacman01.bmp"},
        {"data/sprites/pacman10.bmp", "data/sprites/pacman11.bmp"},
        {"data/sprites/pacman20.bmp", "data/sprites/pacman21.bmp"},
        {"data/sprites/pacman30.bmp", "data/sprites/pacman31.bmp"},
    },
    {
        {"data/sprites/fantome0.bmp"},
        {"data/sprites/fantome1.bmp"},
        {"data/sprites/fantome2.bmp"},
        {"data/sprites/fantome3.bmp"},
    }
};

SDL_Surface *charger_sprite(TypeEntite type, int dir, int frame) {
    SDL_Surface *img = SDL_LoadBMP(entites_sprites_path[type][dir][frame]);
    if (img == NULL) {
        fprintf(stderr, "Erreur lors du chargement du sprite %s\n", entites_sprites_path[type][dir][frame]);
        exit(1);
    }
    // Transparence noire
    SDL_SetColorKey(img, SDL_SRCCOLORKEY, 0xFFFFFF);
    return img;
}


Pos ecran_vers_grille(Pos pos, Pos taille) {
    return (Pos) {
        pos.l  / taille.l,
        pos.c  / taille.c,
    };
}