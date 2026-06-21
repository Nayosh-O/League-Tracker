#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QPieSeries>
#include <QDateTimeAxis>
#include <QValueAxis>

class AppController;

/*
 * StatsWidget — Vue
 * ───────────────────
 * Vue d'ensemble (cartes statistiques, graphiques, listes). Toutes les
 * valeurs affichées proviennent du Contrôleur, qui les calcule lui-même
 * en s'appuyant sur le Modèle.
 */
class StatsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StatsWidget(AppController* controller, QWidget* parent = nullptr);
    void refresh();

private:
    void buildUi();
    void refreshHistoryChart();
    void refreshRarityChart();

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

    // Valeur de la collection
    QLabel* m_lValeurChamps = nullptr;
    QLabel* m_lValeurSkins  = nullptr;

    // Listes
    QLabel* m_lListeMissing  = nullptr;
    QLabel* m_lListePriorite = nullptr;
    QLabel* m_lListeReduc    = nullptr;

    // Graphique d'évolution EB/EO
    QChartView*    m_historyChartView = nullptr;
    QChart*        m_historyChart     = nullptr;
    QLineSeries*   m_ebSeries         = nullptr;
    QLineSeries*   m_eoSeries         = nullptr;
    QDateTimeAxis* m_historyAxisX     = nullptr;
    QValueAxis*    m_historyAxisY     = nullptr;

    // Camembert de répartition des skins par rareté
    QChartView* m_rarityChartView = nullptr;
    QChart*     m_rarityChart     = nullptr;
    QPieSeries* m_raritySeries    = nullptr;
};