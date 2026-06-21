#pragma once
#include <QString>

struct Balise {
    QString nom;
    int  prix    = 0;        // 0 = gratuit / déjà possédé
    bool possede = false;
};
