#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include "../model/champion.h"

/*
 * AddChampionDialog — Vue
 * ─────────────────────────
 * Boîte de dialogue pour ajouter manuellement un nouveau champion à
 * la collection (ex. champion qui vient de sortir et qui n'est pas
 * encore dans la liste de référence du Modèle). Retourne un Champion
 * via getChampion() ; c'est l'appelant qui le transmettra au
 * Contrôleur via addChampion().
 */
class AddChampionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddChampionDialog(QWidget* parent = nullptr);
    Champion getChampion() const;

private:
    QLineEdit* m_nameEdit    = nullptr;
    QCheckBox* m_ownedCb     = nullptr;
    QSpinBox*  m_prixStd     = nullptr;
    QSpinBox*  m_prixReduit  = nullptr;
    QLabel*    m_prixEffLbl  = nullptr;

    void updatePrixEff();
};
