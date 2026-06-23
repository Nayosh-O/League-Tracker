#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QHash>
#include <QStringList>
#include "../model/skin.h"

class QNetworkAccessManager;
class QNetworkReply;
class QCompleter;
class QStringListModel;

/*
 * AddSkinDialog — Vue
 * ─────────────────────
 * Boîte de dialogue pour ajouter manuellement un nouveau skin (ex.
 * skin qui vient de sortir et qui n'est pas encore dans la liste de
 * référence du Modèle). Retourne un Skin via getSkin() ; c'est
 * l'appelant qui le transmettra au Contrôleur via addSkin().
 *
 * Auto-complétion Community Dragon
 * ─────────────────────────────────
 * Une fois un champion reconnu sélectionné dans le combo, le dialogue
 * va chercher (en tâche de fond, données de jeu statiques et non
 * officielles mais largement utilisées par la communauté) la liste de
 * ses skins pour proposer une saisie semi-automatique du nom : taper
 * ou choisir un skin connu renseigne automatiquement sa rareté, et son
 * prix EO pour les paliers Epique/Legendaire dont le tarif de craft
 * est fixe. Les autres paliers (Basique, Fantastique, Ultime, Exalte,
 * Transcendant) n'ont pas de tarif EO universel et restent en saisie
 * manuelle pour le prix.
 *
 * Cette recherche est purement « best effort » : hors-ligne, champion
 * non reconnu, ou skin introuvable, le dialogue se comporte exactement
 * comme avant (saisie 100% manuelle), sans aucun popup ni blocage.
 */
class AddSkinDialog : public QDialog
{
    Q_OBJECT
public:
    // championNames : liste des champions existants, proposée dans le combo.
    // initialChampion : si non vide, pré-sélectionne ce champion (ex.
    // ouverture du dialogue depuis la fiche d'un champion donné).
    explicit AddSkinDialog(const QStringList& championNames, QWidget* parent = nullptr,
                           const QString& initialChampion = QString());
    ~AddSkinDialog() override;

    Skin getSkin() const;

private slots:
    void onGratuitToggled(bool checked);
    void onChampionChanged(const QString& champion);
    void onSkinNameEditingFinished();

private:
    struct SkinSuggestion { QString rarete; };

    void lookupChampionSkins(const QString& champion);
    void fetchChampionIndex();
    void onChampionIndexReply(QNetworkReply* reply);
    void fetchSkinsFor(int championId);
    void onSkinsReply(QNetworkReply* reply);
    void applySuggestion(const QString& typedName);
    void abortPendingRequest();

    QLineEdit*  m_nameEdit   = nullptr;
    QComboBox*  m_champCombo = nullptr;
    QSpinBox*   m_prix       = nullptr;
    QCheckBox*  m_gratuitCb  = nullptr;
    QComboBox*  m_rareteCombo= nullptr;
    QCheckBox*  m_possedeCb  = nullptr;

    QStringList m_championNames; // pour ne déclencher la recherche que sur une vraie sélection

    QNetworkAccessManager* m_nam            = nullptr;
    QNetworkReply*         m_currentReply   = nullptr;
    QCompleter*            m_completer      = nullptr;
    QStringListModel*      m_completerModel = nullptr;

    QHash<QString, int>            m_champNameToId; // nom normalisé -> id Community Dragon (cache de session)
    QHash<QString, SkinSuggestion> m_suggestions;    // nom de skin normalisé -> infos

    QString m_locale              = "fr_fr"; // bascule sur "default" (EN) si fr_fr échoue
    bool    m_localeFallbackTried = false;
    bool    m_indexLoaded         = false;
    bool    m_indexLoading        = false;
    QString m_pendingChampion;    // champion demandé en attendant le chargement de l'index
};