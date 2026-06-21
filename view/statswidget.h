#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>

class AppController;

/*
 * StatsWidget — Vue
 * ───────────────────
 * Vue d'ensemble (cartes statistiques + listes). Toutes les valeurs
 * affichées proviennent du Contrôleur, qui les calcule lui-même en
 * s'appuyant sur le Modèle.
 */
class StatsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StatsWidget(AppController* controller, QWidget* parent = nullptr);
    void refresh();

private:
    void buildUi();

    AppController* m_controller = nullptr;

    // Labels stats
    QLabel* m_lTotal      = nullptr;
    QLabel* m_lOwned      = nullptr;
    QLabel* m_lToBuy      = nullptr;
    QLabel* m_lCout       = nullptr;
    QLabel* m_lEb         = nullptr;
    QLabel* m_lManque     = nullptr;
    QLabel* m_lEo         = nullptr;
    QProgressBar* m_prog  = nullptr;

    // Listes
    QLabel* m_lListeMissing = nullptr;
    QLabel* m_lListeReduc   = nullptr;
};
