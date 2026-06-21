#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class AppController;

/*
 * BalisePage — Vue
 * ──────────────────
 * Tableau des balises avec recherche, case "Possédée" cochable et tri
 * multi-critères (combo dédié, ou clic direct sur un en-tête de colonne).
 */
class BalisePage : public QWidget
{
    Q_OBJECT
public:
    explicit BalisePage(AppController* controller, QWidget* parent = nullptr);
    void refresh();

    // Donne le focus clavier à la barre de recherche (raccourci Ctrl+F).
    void focusSearch();

private slots:
    void onTogglePossede(int originalRow, bool checked);
    void onAddBaliseClicked();
    void onSortDirToggled();
    void onHeaderClicked(int column);

private:
    void applySortFromColumn(int comboIndex);

    AppController* m_controller = nullptr;
    QTableWidget*  m_table      = nullptr;
    QLineEdit*     m_search     = nullptr;
    QLabel*        m_infoLbl    = nullptr;
    QComboBox*     m_sortCombo  = nullptr;
    QPushButton*   m_sortDirBtn = nullptr;
    bool           m_sortAsc    = true;
};