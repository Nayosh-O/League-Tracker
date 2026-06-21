#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QVector>
#include "championcard.h"

class AppController;

/*
 * ChampionGridPage — Vue
 * ────────────────────────
 * Grille de cartes champion avec recherche/filtre. Toute la logique
 * de filtrage et de mise à jour des données est déléguée à
 * AppController ; cette classe se contente de construire l'UI et de
 * réagir aux interactions utilisateur.
 */
class ChampionGridPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChampionGridPage(AppController* controller, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent*) override;

private slots:
    void applyFilter();
    void onCardClicked(const QString& nom);
    void onAddChampionClicked();
    void onDataChanged();

private:
    void rebuildGrid();
    void loadCards();

    AppController* m_controller = nullptr;

    QLineEdit*   m_search  = nullptr;
    QComboBox*   m_filter  = nullptr;
    QLabel*      m_countLbl= nullptr;
    QWidget*     m_grid    = nullptr;
    QGridLayout* m_gridLay = nullptr;
    QScrollArea* m_scroll  = nullptr;

    QVector<ChampionCard*> m_cards;
    int m_cols = 6;
};