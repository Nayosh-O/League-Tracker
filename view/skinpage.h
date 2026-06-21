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
 * Affiche la liste des skins avec recherche et tri multi-critères
 * (combo dédié, ou clic direct sur un en-tête de colonne).
 */
class SkinPage : public QWidget
{
    Q_OBJECT
public:
    explicit SkinPage(AppController* controller, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onAddSkinClicked();
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