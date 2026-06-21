#pragma once
#include <QObject>
#include <QVector>
#include <QString>
#include "../model/champion.h"
#include "../model/skin.h"
#include "../model/balise.h"
#include "../model/datamanager.h" // pour EssenceSnapshot

class DataManager;

/*
 * AppController — Contrôleur (C du MVC)
 * ───────────────────────────────────────
 * Point d'entrée unique entre les Vues et le Modèle (DataManager).
 *
 * - Les Vues n'accèdent JAMAIS directement à DataManager : elles
 *   passent par AppController pour lire les données ou déclencher
 *   une action (clic, saisie, case cochée...).
 * - Le Contrôleur porte la logique "applicative" qui ne fait pas
 *   partie du Modèle : filtrage de la grille de champions,
 *   règles d'achat (skins/balises) basées sur l'état courant.
 * - Le signal dataChanged() relaie celui du Modèle : les Vues s'y
 *   abonnent pour se rafraîchir.
 */
class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);

    // ── Accès en lecture aux données du Modèle ─────────────────────────
    const QVector<Champion>& champions() const;
    const QVector<Skin>&     skins()     const;
    const QVector<Balise>&   balises()   const;

    int essenceBleu()   const;
    int essenceOrange() const;

    // ── Statistiques (déléguées au Modèle) ──────────────────────────────
    int totalChampions()   const;
    int champsOwned()      const;
    int champsToBuy()      const;
    int coutTotalRestant() const;
    int ebApresAchat()     const;
    int valeurChampionsPossedes()    const;
    int valeurSkinsBalisesPossedes() const;

    /* Historique des essences (un point par jour), pour le graphique
     * d'évolution dans StatsWidget. */
    const QVector<EssenceSnapshot>& essenceHistory() const;

    // ── Filtrage de la grille de champions ──────────────────────────────
    struct ChampionFilter {
        QString search;
        int mode = 0; // 0=Tous, 1=Possédés, 2=Non possédés,
        // 3=≤675, 4==1575, 5=≥2400, 6=Avec réduction
    };
    // Retourne les indices (dans champions()) des champions à afficher
    QVector<int> filteredChampionIndices(const ChampionFilter& filter) const;

    // ── Règles métier "achat" pour skins / balises ───────────────────────
    bool isChampionOwned(const QString& championName) const;
    bool canBuySkin(const Skin& s) const;
    bool canBuyBalise(const Balise& b) const;

    /* Indices (dans skins()) des skins associés à un champion donné,
     * dans l'ordre de skins(). Utilisé par la fiche détail d'un champion
     * pour lister "ses" skins. */
    QVector<int> skinIndicesForChampion(const QString& championName) const;

    // ── Actions déclenchées par les Vues ─────────────────────────────────
    void setEssenceBleu(int v);
    void setEssenceOrange(int v);
    void updateChampion(const Champion& c);
    void setBaliseOwned(int index, bool owned);

    /* Marque un skin comme possédé ou non. */
    void setSkinOwned(int index, bool owned);

    /*
     * Ajoute un nouveau champion à la collection (ex. nouveau champion
     * sorti sur LoL). Retourne false si le nom est vide ou existe déjà.
     */
    bool addChampion(const Champion& c);

    /* Idem pour un skin et une balise (clé = nom, insensible à la casse). */
    bool addSkin(const Skin& s);
    bool addBalise(const Balise& b);

    /* Suppriment l'entrée à l'index donné (cf. DataManager). */
    void removeChampion(int index);
    void removeSkin(int index);
    void removeBalise(int index);

signals:
    void dataChanged();

private:
    DataManager* m_model;
};