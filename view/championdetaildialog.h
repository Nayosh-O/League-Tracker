#pragma once
#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QVector>
#include "../model/champion.h"

class AppController;

/*
 * ChampionDetailDialog — Vue
 * ────────────────────────────
 * Boîte de dialogue d'édition d'un champion : statut/prix (édités
 * localement et restitués via getChampion(), c'est l'appelant qui
 * transmet le résultat au Contrôleur), et liste des skins de ce
 * champion avec une case "Possédé" par skin — celle-ci est appliquée
 * immédiatement via AppController::setSkinOwned(), comme les cases
 * de BalisePage.
 */
class ChampionDetailDialog : public QDialog
{
    Q_OBJECT
public:
    // Code de résultat renvoyé par exec() lorsque l'utilisateur a confirmé
    // la suppression du champion (en plus des QDialog::Accepted/Rejected
    // habituels pour "enregistrer" / "annuler").
    static constexpr int Deleted = QDialog::Accepted + 1;

    explicit ChampionDetailDialog(AppController* controller, const Champion& c, QWidget* parent = nullptr);
    Champion getChampion() const;

private:
    AppController* m_controller = nullptr;
    Champion  m_champ;
    QCheckBox* m_ownedCb    = nullptr;
    QCheckBox* m_prioriteCb = nullptr;
    QSpinBox*  m_prixStd    = nullptr;
    QSpinBox*  m_prixReduit = nullptr;
    QLabel*    m_prixEffLbl = nullptr;
    QVector<QCheckBox*> m_roleCbs; // une case par libellé de allRoleNames()

    QVBoxLayout* m_skinsLayout = nullptr;

    void updatePrixEff();
    void onDeleteClicked();
    void buildSkinsSection(QVBoxLayout* main);
    void refreshSkinsSection();
    void onAddSkinClicked();
};