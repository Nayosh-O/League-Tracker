#pragma once
#include <QString>

struct Champion {
    QString nom;
    int prixStandard = 0;
    int prixReduit   = 0;   // 0 = pas de réduction
    bool possede     = false;
    bool prioritaire = false; // marqué "à acheter en priorité" par le joueur

    int prixEffectif() const {
        return (prixReduit > 0) ? prixReduit : prixStandard;
    }
};