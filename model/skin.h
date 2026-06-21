#pragma once
#include <QString>

struct Skin {
    QString nom;
    QString champion;
    int     prix    = 0;     // 0 = gratuit
    bool    gratuit = false;
    QString rarete;
    bool    possede = false; // skin effectivement possédé par le joueur
};
