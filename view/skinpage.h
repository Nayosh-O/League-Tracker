#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class AppController;

/*
 * SkinPage — Vue
 * ────────────────
 * Affiche la liste des skins avec tri multi-critères.
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

private:
    AppController* m_controller = nullptr;
    QTableWidget*  m_table      = nullptr;
    QLabel*        m_infoLbl    = nullptr;
    QComboBox*     m_sortCombo  = nullptr;
    QPushButton*   m_sortDirBtn = nullptr;
    bool           m_sortAsc    = true;
};
