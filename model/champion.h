#pragma once
#include <QString>
#include <QStringList>

/*
 * Tags de rôle (Top/Jungle/Mid/ADC/Support) utilisés pour le filtre par
 * rôle de la grille de champions. Un champion peut cumuler plusieurs
 * rôles (ex. champions "flex" comme Akali ou Pyke) : c'est un masque
 * de bits combinable avec |, pas une valeur exclusive.
 *
 * Remarque : contrairement aux noms de champions, ces rôles ne sont pas
 * fournis par Data Dragon (qui n'expose que des archétypes génériques
 * type Fighter/Mage/Tank, pas des lanes) — ils sont assignés à la main
 * ici à partir de la méta actuelle, et modifiables champion par champion
 * dans l'app (cf. AddChampionDialog / ChampionDetailDialog).
 */
enum RoleFlag {
    RoleNone    = 0,
    RoleTop     = 1 << 0,
    RoleJungle  = 1 << 1,
    RoleMid     = 1 << 2,
    RoleADC     = 1 << 3,
    RoleSupport = 1 << 4,
};

// Liste ordonnée des libellés de rôle connus — utilisée pour peupler le
// filtre de la grille et les cases à cocher des dialogues d'édition
// sans dupliquer ces 5 libellés à plusieurs endroits.
inline const QStringList& allRoleNames() {
    static const QStringList names = {"Top", "Jungle", "Mid", "ADC", "Support"};
    return names;
}

// Convertit un masque de RoleFlag (ex. RoleTop|RoleJungle) en libellés
// (ex. {"Top", "Jungle"}), dans l'ordre de allRoleNames().
inline QStringList rolesFromMask(int mask) {
    QStringList out;
    if (mask & RoleTop)     out << "Top";
    if (mask & RoleJungle)  out << "Jungle";
    if (mask & RoleMid)     out << "Mid";
    if (mask & RoleADC)     out << "ADC";
    if (mask & RoleSupport) out << "Support";
    return out;
}

struct Champion {
    QString nom;
    int prixStandard = 0;
    int prixReduit   = 0;   // 0 = pas de réduction
    bool possede     = false;
    bool prioritaire = false; // marqué "à acheter en priorité" par le joueur
    QStringList roles;        // sous-ensemble de allRoleNames(), 0 à 5 valeurs

    int prixEffectif() const {
        return (prixReduit > 0) ? prixReduit : prixStandard;
    }
    bool hasRole(const QString& role) const { return roles.contains(role); }
};