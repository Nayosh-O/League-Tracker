#pragma once
#include <QObject>
#include <QVector>
#include <QString>
#include "champion.h"
#include "skin.h"
#include "balise.h"

/*
 * Un point d'historique de tes essences, créé à chaque changement réel
 * d'EB ou d'EO (granularité fine, horodatée à la seconde — y compris
 * plusieurs points dans la même journée). Sert à tracer l'évolution
 * dans le temps (cf. StatsWidget).
 */
struct EssenceSnapshot {
    QString date; // "yyyy-MM-dd HH:mm:ss" (anciennes sauvegardes : "yyyy-MM-dd")
    int eb = 0;
    int eo = 0;
};

/*
 * DataManager — Modèle (M du MVC)
 * ────────────────────────────────
 * Détient les données de l'application (champions, skins, balises,
 * essences), assure leur persistance sur disque (JSON) et expose
 * les calculs statistiques purement liés aux données.
 *
 * Le Modèle ne connaît rien de l'interface graphique : il ne fait
 * que stocker, calculer et notifier (signal dataChanged) lorsqu'il
 * change. Toutes les mutations passent par des méthodes dédiées qui
 * se chargent elles-mêmes de la sauvegarde et de la notification.
 */
class DataManager : public QObject
{
    Q_OBJECT
public:
    static DataManager* instance();

    void load();
    void save();

    /*
     * Export/Import — Sauvegarde/restauration manuelle de tes données
     * (champions, skins, balises, essences, historique) vers/depuis un
     * fichier .json choisi par toi : utile en cas de réinstallation, ou
     * pour synchroniser entre deux PC (ex. via clé USB ou cloud perso).
     *
     * exportTo()   : sauvegarde l'état courant puis copie le fichier de
     *                données vers `path`. Retourne false si l'écriture
     *                a échoué (ex. dossier non accessible).
     * importFrom() : relit `path`, vérifie qu'il s'agit bien d'une
     *                sauvegarde LeagueTracker valide (JSON + clés
     *                attendues), puis REMPLACE entièrement les données
     *                courantes par son contenu (avec fusion automatique
     *                des champions/skins/balises sortis depuis, comme au
     *                chargement normal). Ne touche à rien en cas
     *                d'échec (fichier invalide, illisible...).
     */
    bool exportTo(const QString& path);
    bool importFrom(const QString& path);

    const QVector<Champion>& champions() const { return m_champions; }
    const QVector<Skin>&     skins()     const { return m_skins; }
    const QVector<Balise>&   balises()   const { return m_balises; }

    int  essenceBleu()  const { return m_eb; }
    int  essenceOrange() const { return m_eo; }
    void setEssenceBleu(int v);
    void setEssenceOrange(int v);

    /* Historique des essences (un point par changement réel), pour le
     * graphique d'évolution dans le temps. */
    const QVector<EssenceSnapshot>& essenceHistory() const { return m_history; }

    /* Mutations sur les champions / balises */
    void updateChampion(const Champion& c);
    void setBaliseOwned(int index, bool owned);

    /* Marque un skin comme possédé ou non. */
    void setSkinOwned(int index, bool owned);

    /*
     * Ajoute un nouveau champion (ex. sortie d'un nouveau champion sur
     * le jeu). Retourne false si le nom existe déjà (insensible à la
     * casse) ou si le nom est vide — dans ce cas rien n'est modifié.
     */
    bool addChampion(const Champion& c);

    /* Idem pour un skin (clé = nom du skin) et une balise (clé = nom). */
    bool addSkin(const Skin& s);
    bool addBalise(const Balise& b);

    /*
     * Suppriment l'entrée à l'index donné (no-op si l'index est hors
     * limites). Sauvegardent et émettent dataChanged() en cas de succès.
     */
    void removeChampion(int index);
    void removeSkin(int index);
    void removeBalise(int index);

    /* Statistiques calculées */
    int totalChampions() const  { return m_champions.size(); }
    int champsOwned()    const;
    int champsToBuy()    const;
    int coutTotalRestant() const;
    int ebApresAchat()   const;

    /* Valeur (au prix d'achat) de ce que tu possèdes déjà, séparée par
     * monnaie puisque EB et EO ne se mélangent pas. */
    int valeurChampionsPossedes()     const;
    int valeurSkinsBalisesPossedes()  const;

signals:
    void dataChanged();

private:
    DataManager() = default;
    QString dataPath() const;
    void initDefaultData();
    void logEssenceSnapshot();

    /*
     * Listes de référence connues par l'application. C'est ICI qu'il
     * faut ajouter une entrée lorsqu'un nouveau champion / skin /
     * balise sort sur League of Legends : au prochain lancement, les
     * mergeNewXxx() correspondants l'ajouteront automatiquement à la
     * sauvegarde existante (sans toucher aux données déjà enregistrées).
     */
    static QVector<Champion> referenceChampions();
    static QVector<Skin>     referenceSkins();
    static QVector<Balise>   referenceBalises();

    /* Ajoutent aux listes courantes les entrées de référence absentes
     * (par nom), puis sauvegardent si besoin. */
    void mergeNewChampions();
    void mergeNewSkins();
    void mergeNewBalises();

    /* Rétro-compatibilité : remplit les rôles (cf. Champion::roles) des
     * champions déjà connus dans referenceChampions() mais dont la
     * sauvegarde a été créée avant l'ajout de ce champ. Ne s'exécute
     * qu'une fois (cf. m_rolesMigrated) pour ne jamais écraser un choix
     * du joueur qui aurait sciemment décoché tous les rôles d'un champion.
     */
    void backfillRoles();

    QVector<Champion> m_champions;
    QVector<Skin>     m_skins;
    QVector<Balise>   m_balises;
    QVector<EssenceSnapshot> m_history;
    int m_eb = 17682;
    int m_eo = 1830;
    bool m_rolesMigrated = false;
};