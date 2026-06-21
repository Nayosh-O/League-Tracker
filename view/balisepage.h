#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class AppController;

/*
 * BalisePage — Vue
 * ──────────────────
 * Tableau des balises avec case "Possédée" cochable et tri multi-critères.
 */
class BalisePage : public QWidget
{
    Q_OBJECT
public:
    explicit BalisePage(AppController* controller, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onTogglePossede(int originalRow, bool checked);
    void onAddBaliseClicked();
    void onSortDirToggled();

private:
    AppController* m_controller = nullptr;
    QTableWidget*  m_table      = nullptr;
    QLabel*        m_infoLbl    = nullptr;
    QComboBox*     m_sortCombo  = nullptr;
    QPushButton*   m_sortDirBtn = nullptr;
    bool           m_sortAsc    = true;
};
