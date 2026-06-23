#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class AppController;

/*
 * SkinPage — Vue
 * ────────────────
 * Affiche la liste des skins avec recherche, tri multi-critères
 * (combo dédié, ou clic direct sur un en-tête de colonne), et filtres
 * combinés (rareté, statut champion possédé/non possédé, statut skin).
 */
class SkinPage : public QWidget
{
    Q_OBJECT
public:
    explicit SkinPage(AppController* controller, QWidget* parent = nullptr);
    void refresh();

    // Donne le focus clavier à la barre de recherche (raccourci Ctrl+F).
    void focusSearch();

private slots:
    void onAddSkinClicked();
    void onSortDirToggled();
    void onHeaderClicked(int column);
    void onResetFilters();

private:
    void applySortFromColumn(int comboIndex);
    void updateResetBtn();

    AppController* m_controller   = nullptr;
    QTableWidget*  m_table        = nullptr;
    QLineEdit*     m_search       = nullptr;
    QLabel*        m_infoLbl      = nullptr;
    QComboBox*     m_sortCombo    = nullptr;
    QPushButton*   m_sortDirBtn   = nullptr;
    bool           m_sortAsc      = true;

    // ── Filtres combinés ─────────────────────────────────────────────────
    QComboBox*   m_filterRarete  = nullptr; // "Toutes raretés", "Basique", …
    QComboBox*   m_filterChamp   = nullptr; // "Tous", "Champion possédé", "Champion non possédé"
    QComboBox*   m_filterPossede = nullptr; // "Tous", "Possédé", "Non possédé"
    QPushButton* m_resetBtn      = nullptr; // visible uniquement si un filtre est actif
};