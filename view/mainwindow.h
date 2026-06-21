#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>

class ChampionGridPage;
class SkinPage;
class BalisePage;
class StatsWidget;
class AppController;

/*
 * MainWindow — Vue (V du MVC)
 * ─────────────────────────────
 * Fenêtre principale : barre de navigation, essences, et conteneur
 * des différentes pages. Ne contient aucune logique métier : elle
 * délègue toute lecture/écriture de données à AppController.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(AppController* controller, QWidget* parent = nullptr);

private slots:
    void onNavClicked(int index);
    void onEbClicked();
    void onEoClicked();
    void onQuitClicked();
    void refreshEssence();

private:
    void buildUi();
    void applyTheme();

    AppController*   m_controller = nullptr;

    QStackedWidget*  m_stack      = nullptr;
    ChampionGridPage* m_gridPage  = nullptr;
    SkinPage*         m_skinPage  = nullptr;
    BalisePage*       m_balisePage= nullptr;
    StatsWidget*      m_statsPage = nullptr;

    QLabel* m_ebLabel = nullptr;
    QLabel* m_eoLabel = nullptr;
    QPushButton* m_navBtns[4] = {};
};