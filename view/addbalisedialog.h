#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include "../model/balise.h"

/*
 * AddBaliseDialog — Vue
 * ───────────────────────
 * Boîte de dialogue pour ajouter manuellement une nouvelle balise
 * (ex. balise qui vient de sortir et qui n'est pas encore dans la
 * liste de référence du Modèle). Retourne une Balise via
 * getBalise() ; c'est l'appelant qui la transmettra au Contrôleur
 * via addBalise().
 */
class AddBaliseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddBaliseDialog(QWidget* parent = nullptr);
    Balise getBalise() const;

private slots:
    void onGratuiteToggled(bool checked);

private:
    QLineEdit* m_nameEdit  = nullptr;
    QCheckBox* m_gratuiteCb= nullptr;
    QSpinBox*  m_prix      = nullptr;
    QCheckBox* m_possedeCb = nullptr;
};
