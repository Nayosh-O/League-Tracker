#include "championgridpage.h"
#include "../controller/appcontroller.h"
#include "championdetaildialog.h"
#include "addchampiondialog.h"
#include "imagedownloaddialog.h"
#include "toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include <utility>
#include <algorithm>

static const char* GRID_STYLE = R"(
QWidget#gridPage { background: #0A0E14; }
QLineEdit#search {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    border-radius: 4px;
    color: #C8AA6E;
    padding: 6px 12px;
    font-size: 13px;
    min-width: 220px;
}
QLineEdit#search:focus { border-color: #C89B3C; }
QComboBox#filterBox {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    color: #C8AA6E;
    padding: 4px 10px;
    border-radius: 4px;
    font-size: 13px;
}
QComboBox#filterBox::drop-down { border: none; }
QComboBox#filterBox QAbstractItemView {
    background: #1E2328;
    color: #C8AA6E;
    selection-background-color: #C89B3C;
    selection-color: #000;
}
QLabel#countLbl { color: #7A7A7A; font-size: 12px; }
QPushButton#addBtn {
    background: transparent;
    border: 1px solid #C89B3C;
    color: #C89B3C;
    border-radius: 4px;
    padding: 6px 14px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton#addBtn:hover { background: #C89B3C; color: #000; }
QPushButton#dlImgBtn {
    background: transparent;
    border: 1px solid #5B8DBE;
    color: #5B8DBE;
    border-radius: 4px;
    padding: 6px 14px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton#dlImgBtn:hover { background: #5B8DBE; color: #000; }
QScrollArea { background: #0A0E14; border: none; }
QWidget#gridContainer { background: #0A0E14; }

/* ── Barre de tri ── */
QLabel#sortLbl { color: #888; font-size: 12px; }
QComboBox#sortCombo {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    color: #C8AA6E;
    padding: 4px 8px;
    border-radius: 4px;
    font-size: 12px;
    min-width: 160px;
}
QComboBox#sortCombo:focus { border-color: #C89B3C; }
QComboBox#sortCombo::drop-down { border: none; }
QComboBox#sortCombo QAbstractItemView {
    background: #1E2328;
    color: #C8AA6E;
    selection-background-color: #C89B3C;
    selection-color: #000;
}
QPushButton#sortDirBtn {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    color: #C8AA6E;
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 13px;
    min-width: 36px;
}
QPushButton#sortDirBtn:hover { border-color: #C89B3C; color: #C89B3C; }
)";

ChampionGridPage::ChampionGridPage(AppController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller)
{
    setObjectName("gridPage");
    setStyleSheet(GRID_STYLE);

    QVBoxLayout* mainL = new QVBoxLayout(this);
    mainL->setContentsMargins(0,0,0,0);
    mainL->setSpacing(0);

    // ─── Barre du haut ───────────────────────────────────────────────────────
    QWidget* topBar = new QWidget;
    topBar->setStyleSheet("background:#0F1923; border-bottom:1px solid #1E2328;");
    QHBoxLayout* topL = new QHBoxLayout(topBar);
    topL->setContentsMargins(16, 10, 16, 10);

    m_search = new QLineEdit;
    m_search->setObjectName("search");
    m_search->setPlaceholderText("🔍  Rechercher un champion...");
    connect(m_search, &QLineEdit::textChanged, this, &ChampionGridPage::applyFilter);

    m_filter = new QComboBox;
    m_filter->setObjectName("filterBox");
    m_filter->addItems({"Tous les champions", "Possédés", "Non possédés",
                        "Pas cher (≤ 675)", "Moyen (1575)", "Cher (2400+)", "Avec réduction"});
    connect(m_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChampionGridPage::applyFilter);

    m_countLbl = new QLabel;
    m_countLbl->setObjectName("countLbl");

    QPushButton* addBtn = new QPushButton("✚  Nouveau champion");
    addBtn->setObjectName("addBtn");
    connect(addBtn, &QPushButton::clicked, this, &ChampionGridPage::onAddChampionClicked);

    QPushButton* dlImgBtn = new QPushButton("⬇  Télécharger les images manquantes");
    dlImgBtn->setObjectName("dlImgBtn");
    dlImgBtn->setToolTip("Récupère automatiquement les portraits manquants depuis Data Dragon (riotgames).");
    connect(dlImgBtn, &QPushButton::clicked, this, &ChampionGridPage::onDownloadImagesClicked);

    topL->addWidget(m_search);
    topL->addWidget(m_filter);
    topL->addWidget(addBtn);
    topL->addWidget(dlImgBtn);
    topL->addStretch();
    topL->addWidget(m_countLbl);
    mainL->addWidget(topBar);

    // ─── Barre de tri ──────────────────────────────────────────────────────
    QWidget* sortBar = new QWidget;
    sortBar->setStyleSheet("QWidget { background: #0D1117; border-bottom: 1px solid #1E2328; }");
    QHBoxLayout* sortL = new QHBoxLayout(sortBar);
    sortL->setContentsMargins(16, 6, 16, 6);
    sortL->setSpacing(8);

    QLabel* sortLbl = new QLabel("  Trier par :");
    sortLbl->setObjectName("sortLbl");
    sortL->addWidget(sortLbl);

    m_sortCombo = new QComboBox;
    m_sortCombo->setObjectName("sortCombo");
    m_sortCombo->addItems({
        "Nom (A → Z)",
        "Prix effectif",
        "Possédé en premier",
        "Non possédé en premier"
    });
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChampionGridPage::rebuildGrid);
    sortL->addWidget(m_sortCombo);

    m_sortDirBtn = new QPushButton("↑");
    m_sortDirBtn->setObjectName("sortDirBtn");
    m_sortDirBtn->setToolTip("Inverser l'ordre");
    connect(m_sortDirBtn, &QPushButton::clicked, this, &ChampionGridPage::onSortDirToggled);
    sortL->addWidget(m_sortDirBtn);

    sortL->addStretch();
    mainL->addWidget(sortBar);

    // ─── Grille ──────────────────────────────────────────────────────────────
    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);

    m_grid = new QWidget;
    m_grid->setObjectName("gridContainer");
    m_gridLay = new QGridLayout(m_grid);
    m_gridLay->setSpacing(8);
    m_gridLay->setContentsMargins(16, 16, 16, 16);
    m_scroll->setWidget(m_grid);
    mainL->addWidget(m_scroll, 1);

    // Créer les cartes à partir des données fournies par le Contrôleur.
    // Différé au prochain passage de la boucle d'événements : laisse
    // d'abord la fenêtre s'afficher et se peindre normalement avant de
    // construire les ~170 cartes (avec leur image), ce qui évitait que
    // Windows affiche une fenêtre fantôme blanche pendant ce travail.
    QTimer::singleShot(0, this, &ChampionGridPage::loadCards);

    connect(m_controller, &AppController::dataChanged,
            this, &ChampionGridPage::onDataChanged);

    applyFilter();
}

void ChampionGridPage::loadCards() {
    const auto& champs = m_controller->champions();
    m_cards.reserve(champs.size());
    for (const auto& c : champs) {
        // IMPORTANT : on passe m_grid comme parent dès la création.
        // Sans parent, un QWidget est traité par Qt comme une fenêtre
        // indépendante au niveau du système — et comme applyFilter()
        // appelle setVisible(true) sur ces cartes AVANT qu'elles soient
        // ajoutées à la grille (rebuildGrid), chacune des ~170 cartes
        // s'ouvrait brièvement comme une vraie fenêtre Windows à part
        // entière. C'était la cause des nombreux flashs blancs au
        // démarrage.
        auto* card = new ChampionCard(c, m_grid);
        connect(card, &ChampionCard::clicked, this, &ChampionGridPage::onCardClicked);
        m_cards << card;
    }
    applyFilter();
}

void ChampionGridPage::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    int avail = m_scroll->viewport()->width() - 32;
    int cols  = qMax(1, avail / (ChampionCard::W + 8));
    if (cols != m_cols) {
        m_cols = cols;
        rebuildGrid();
    }
}

void ChampionGridPage::focusSearch() {
    m_search->setFocus(Qt::ShortcutFocusReason);
    m_search->selectAll();
}

void ChampionGridPage::onSortDirToggled() {
    m_sortAsc = !m_sortAsc;
    m_sortDirBtn->setText(m_sortAsc ? "↑" : "↓");
    rebuildGrid();
}

void ChampionGridPage::applyFilter() {
    AppController::ChampionFilter filter;
    filter.search = m_search->text();
    filter.mode   = m_filter->currentIndex();

    QVector<int> visibleIdx = m_controller->filteredChampionIndices(filter);

    // Masquer toutes les cartes d'abord
    for (auto* c : std::as_const(m_cards)) c->setVisible(false);

    for (int idx : visibleIdx)
        if (idx >= 0 && idx < m_cards.size()) m_cards[idx]->setVisible(true);

    m_countLbl->setText(QString("%1 champion(s)").arg(visibleIdx.size()));
    rebuildGrid();
}

QVector<int> ChampionGridPage::sortedOrder() const {
    const auto& champs = m_controller->champions();
    QVector<int> order;
    order.reserve(champs.size());
    for (int i = 0; i < champs.size(); ++i) order.append(i);

    int  mode = m_sortCombo ? m_sortCombo->currentIndex() : 0;
    bool asc  = m_sortAsc;

    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const Champion& ca = champs[a];
        const Champion& cb = champs[b];
        bool res = false;
        switch (mode) {
        case 0: res = ca.nom.toLower() < cb.nom.toLower();           break; // Nom
        case 1: res = ca.prixEffectif() < cb.prixEffectif();         break; // Prix effectif
        case 2: res = (ca.possede > cb.possede);                     break; // Possédé d'abord
        case 3: res = (ca.possede < cb.possede);                     break; // Non possédé d'abord
        }
        return asc ? res : !res;
    });
    return order;
}

void ChampionGridPage::rebuildGrid() {
    // On désactive le rendu pendant qu'on reconstruit toute la grille :
    // sans ça, chaque addWidget()/show() (un par champion, ~170 fois)
    // déclenche son propre recalcul de layout + repaint, ce qui causait
    // un clignotement violent de la fenêtre au démarrage.
    setUpdatesEnabled(false);

    // Retirer toutes les cartes du layout sans les détruire
    while (m_gridLay->count())
        m_gridLay->takeAt(0);

    int col = 0, row = 0;
    for (int idx : sortedOrder()) {
        if (idx < 0 || idx >= m_cards.size()) continue;
        ChampionCard* card = m_cards[idx];
        if (!card->isVisible()) continue;
        m_gridLay->addWidget(card, row, col);
        card->show();
        if (++col >= m_cols) { col = 0; ++row; }
    }
    // Stretch vertical pour pousser les cartes vers le haut
    m_gridLay->setRowStretch(row + 1, 1);

    setUpdatesEnabled(true);
    update();
}

void ChampionGridPage::onCardClicked(const QString& nom) {
    const auto& champs = m_controller->champions();
    for (int i = 0; i < champs.size(); ++i) {
        if (champs[i].nom == nom) {
            ChampionDetailDialog dlg(m_controller, champs[i], this);
            int result = dlg.exec();
            if (result == QDialog::Accepted) {
                m_controller->updateChampion(dlg.getChampion());
            } else if (result == ChampionDetailDialog::Deleted) {
                // La suppression d'un champion reste confirmée via une boîte
                // de dialogue dans ChampionDetailDialog (action plus lourde
                // qu'une simple ligne de tableau, et déjà annoncée comme
                // irréversible) ; on se contente d'un toast pour confirmer
                // que l'action a bien été prise en compte.
                m_controller->removeChampion(i);
                Toast::show(this, QString("Champion « %1 » supprimé").arg(nom), Toast::Danger);
            }
            break;
        }
    }
}

void ChampionGridPage::onDownloadImagesClicked() {
    ImageDownloadDialog dlg(m_controller, this);
    dlg.exec();

    // Que l'opération ait réussi, échoué ou été annulée en cours de route,
    // on invalide le cache de fichiers d'images et on force chaque carte
    // à recharger son portrait : les éventuelles images téléchargées
    // doivent apparaître immédiatement, sans relancer l'application.
    ChampionCard::invalidateImageCache();
    const auto& champs = m_controller->champions();
    for (int i = 0; i < m_cards.size() && i < champs.size(); ++i)
        m_cards[i]->updateData(champs[i]);
    update();

    if (dlg.downloadedCount() > 0) {
        QString msg = QString("%1 image(s) téléchargée(s)").arg(dlg.downloadedCount());
        if (dlg.failedCount() > 0) msg += QString(" • %1 introuvable(s)").arg(dlg.failedCount());
        Toast::show(this, msg, Toast::Success);
    }
}

void ChampionGridPage::onAddChampionClicked() {
    AddChampionDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    Champion c = dlg.getChampion();
    if (c.nom.isEmpty()) {
        QMessageBox::warning(this, "Nom manquant", "Donne un nom au champion avant de l'ajouter.");
        return;
    }
    if (!m_controller->addChampion(c)) {
        QMessageBox::warning(this, "Champion existant",
                             QString("« %1 » est déjà dans ta collection.").arg(c.nom));
    }
    // Si l'ajout réussit, AppController::dataChanged() est émis et
    // onDataChanged() se charge de créer la nouvelle carte + de
    // rafraîchir la grille.
}

void ChampionGridPage::onDataChanged() {
    const auto& champs = m_controller->champions();

    if (champs.size() < m_cards.size()) {
        // Un champion a été supprimé : on reconstruit toutes les cartes
        // pour rester synchro avec les indices côté Contrôleur.
        for (auto* card : std::as_const(m_cards)) {
            m_gridLay->removeWidget(card);
            card->deleteLater();
        }
        m_cards.clear();
        for (const auto& c : champs) {
            auto* card = new ChampionCard(c, m_grid);
            connect(card, &ChampionCard::clicked, this, &ChampionGridPage::onCardClicked);
            m_cards << card;
        }
    } else {
        // Mettre à jour les cartes existantes
        for (int i = 0; i < m_cards.size() && i < champs.size(); ++i)
            m_cards[i]->updateData(champs[i]);

        // Créer les cartes pour les champions ajoutés depuis la dernière mise à jour
        for (int i = m_cards.size(); i < champs.size(); ++i) {
            auto* card = new ChampionCard(champs[i], m_grid);
            connect(card, &ChampionCard::clicked, this, &ChampionGridPage::onCardClicked);
            m_cards << card;
        }
    }

    applyFilter();
}