#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include "../model/skin.h"

/*
 * AddSkinDialog — Vue
 * ─────────────────────
 * Boîte de dialogue pour ajouter manuellement un nouveau skin (ex.
 * skin qui vient de sortir et qui n'est pas encore dans la liste de
 * référence du Modèle). Retourne un Skin via getSkin() ; c'est
 * l'appelant qui le transmettra au Contrôleur via addSkin().
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
    Skin getSkin() const;

private slots:
    void onGratuitToggled(bool checked);

private:
    QLineEdit*  m_nameEdit   = nullptr;
    QComboBox*  m_champCombo = nullptr;
    QSpinBox*   m_prix       = nullptr;
    QCheckBox*  m_gratuitCb  = nullptr;
    QComboBox*  m_rareteCombo= nullptr;
    QCheckBox*  m_possedeCb  = nullptr;
};
