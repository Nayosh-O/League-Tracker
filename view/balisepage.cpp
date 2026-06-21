#include "balisepage.h"
#include "../controller/appcontroller.h"
#include "addbalisedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <algorithm>

static const char* PAGE_STYLE = R"(
QWidget { background: #0A0E14; }
QLabel#infoLbl { color: #C89B3C; font-size: 13px; }
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

/* ── Tableau ── */
QTableWidget {
    background: #0F1923;
    color: #C8AA6E;
    gridline-color: #1E2328;
    border: none;
    font-size: 12px;
}
QTableWidget::item { padding: 6px 10px; border-bottom: 1px solid #1E2328; }
QTableWidget::item:selected { background: #C89B3C; color: #000; }
QTableWidget { alternate-background-color: #141920; }
QHeaderView::section {
    background: #010A13;
    color: #C89B3C;
    font-weight: bold;
    padding: 8px;
    border: none;
    border-bottom: 2px solid #C89B3C;
}
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
QPushButton#delRowBtn {
    background: transparent;
    border: 1px solid #E74C3C;
    color: #E74C3C;
    border-radius: 3px;
    padding: 2px 8px;
    font-size: 11px;
}
QPushButton#delRowBtn:hover { background: #E74C3C; color: #000; }
)";

BalisePage::BalisePage(AppController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller)
{
    setStyleSheet(PAGE_STYLE);
    QVBoxLayout* L = new QVBoxLayout(this);
    L->setContentsMargins(0, 0, 0, 0);
    L->setSpacing(0);

    // ── Barre supérieure (recherche + info + bouton ajout) ──────────────────
    m_search = new QLineEdit;
    m_search->setObjectName("search");
    m_search->setPlaceholderText("🔍  Rechercher une balise...");
    connect(m_search, &QLineEdit::textChanged, this, &BalisePage::refresh);

    m_infoLbl = new QLabel;
    m_infoLbl->setObjectName("infoLbl");

    QPushButton* addBtn = new QPushButton("✚  Nouvelle balise");
    addBtn->setObjectName("addBtn");
    connect(addBtn, &QPushButton::clicked, this, &BalisePage::onAddBaliseClicked);

    QWidget* topBar = new QWidget;
    QHBoxLayout* topL = new QHBoxLayout(topBar);
    topL->setContentsMargins(16, 6, 16, 6);
    topL->addWidget(m_search);
    topL->addWidget(m_infoLbl);
    topL->addStretch();
    topL->addWidget(addBtn);
    L->addWidget(topBar);

    // ── Barre de tri ─────────────────────────────────────────────────────
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
        "Prix",
        "Possédée en premier",
        "Non possédée en premier"
    });
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BalisePage::refresh);
    sortL->addWidget(m_sortCombo);

    m_sortDirBtn = new QPushButton("↑");
    m_sortDirBtn->setObjectName("sortDirBtn");
    m_sortDirBtn->setToolTip("Inverser l'ordre");
    connect(m_sortDirBtn, &QPushButton::clicked, this, &BalisePage::onSortDirToggled);
    sortL->addWidget(m_sortDirBtn);

    sortL->addStretch();
    L->addWidget(sortBar);

    // ── Tableau ───────────────────────────────────────────────────────────
    m_table = new QTableWidget;
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(
        {"Nom de la balise", "Prix (EO)", "Possédée", "Achetable", ""});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c <= 4; ++c)
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSortIndicatorShown(true);
    m_table->horizontalHeader()->setSectionsClickable(true);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &BalisePage::onHeaderClicked);
    L->addWidget(m_table, 1);

    connect(m_controller, &AppController::dataChanged, this, &BalisePage::refresh);
    refresh();
}

void BalisePage::onSortDirToggled() {
    m_sortAsc = !m_sortAsc;
    m_sortDirBtn->setText(m_sortAsc ? "↑" : "↓");
    refresh();
}

// Clic direct sur un en-tête de colonne : un premier clic trie selon ce
// critère, un second clic sur la même colonne inverse juste le sens —
// pour la colonne « Possédée », inverser le sens revient justement à
// basculer entre « possédées d'abord » et « non possédées d'abord ».
void BalisePage::onHeaderClicked(int column) {
    int sortIdx = -1;
    switch (column) {
    case 0: sortIdx = 0; break; // Nom
    case 1: sortIdx = 1; break; // Prix
    case 2: sortIdx = 2; break; // Possédée
    default: return; // colonnes non triables : Achetable, 🗑
    }
    applySortFromColumn(sortIdx);
}

void BalisePage::applySortFromColumn(int comboIndex) {
    if (m_sortCombo->currentIndex() == comboIndex) {
        onSortDirToggled();
    } else {
        m_sortAsc = true;
        m_sortDirBtn->setText("↑");
        m_sortCombo->setCurrentIndex(comboIndex); // déclenche refresh() via le signal déjà connecté
    }
}

void BalisePage::refresh() {
    const auto& balises = m_controller->balises();
    int eo = m_controller->essenceOrange();

    // ── Stats globales (toujours sur la collection complète, pas filtrée) ──
    int totalOwned = 0, totalAchetable = 0;
    for (const auto& b : balises) {
        if (b.possede) ++totalOwned;
        if (m_controller->canBuyBalise(b)) ++totalAchetable;
    }

    // ── Filtrage par recherche (nom de la balise) ───────────────────────────
    const QString searchTxt = m_search ? m_search->text().trimmed().toLower() : QString();
    QVector<int> idx;
    idx.reserve(balises.size());
    for (int i = 0; i < balises.size(); ++i) {
        if (!searchTxt.isEmpty() && !balises[i].nom.toLower().contains(searchTxt))
            continue;
        idx.append(i);
    }

    // ── Tri : on trie des indices pour garder le lien avec le contrôleur ─
    int  mode = m_sortCombo ? m_sortCombo->currentIndex() : 0;
    bool asc  = m_sortAsc;

    std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
        const Balise& ba = balises[a];
        const Balise& bb = balises[b];
        bool res = false;
        switch (mode) {
        case 0: // Nom A→Z
            res = ba.nom.toLower() < bb.nom.toLower();
            break;
        case 1: // Prix croissant
            res = ba.prix < bb.prix;
            break;
        case 2: // Possédées en premier
            res = (ba.possede > bb.possede);
            break;
        case 3: // Non possédées en premier
            res = (ba.possede < bb.possede);
            break;
        }
        return asc ? res : !res;
    });

    // Indicateur visuel (petite flèche) sur l'en-tête de colonne correspondant
    int sortCol = -1;
    switch (mode) {
    case 0: sortCol = 0; break; // Nom
    case 1: sortCol = 1; break; // Prix
    case 2: sortCol = 2; break; // Possédée (le sens couvre aussi "non possédée d'abord")
    case 3: sortCol = 2; break;
    }
    if (sortCol >= 0)
        m_table->horizontalHeader()->setSortIndicator(sortCol, asc ? Qt::AscendingOrder : Qt::DescendingOrder);

    // ── Remplissage du tableau ────────────────────────────────────────────
    m_table->setRowCount(idx.size());

    for (int di = 0; di < idx.size(); ++di) {
        int origRow = idx[di];              // index réel dans le contrôleur
        const Balise& b = balises[origRow];

        auto makeItem = [](const QString& txt, const QColor& col = QColor(0xC8, 0xAA, 0x6E)) {
            auto* it = new QTableWidgetItem(txt);
            it->setForeground(col);
            return it;
        };

        m_table->setItem(di, 0, makeItem(b.nom));
        m_table->setItem(di, 1, makeItem(b.prix == 0 ? "Gratuite" : QString::number(b.prix)));

        // ── Checkbox Possédée ────────────────────────────────────────────
        QWidget* cbWidget = new QWidget;
        QHBoxLayout* cbLay = new QHBoxLayout(cbWidget);
        cbLay->setContentsMargins(0, 0, 0, 0);
        cbLay->setAlignment(Qt::AlignCenter);
        QCheckBox* cb = new QCheckBox;
        cb->setChecked(b.possede);
        cb->setStyleSheet(R"(
            QCheckBox::indicator { width:16px; height:16px; border:2px solid #C89B3C;
                                   border-radius:3px; background:#1E2328; }
            QCheckBox::indicator:checked { background:#C89B3C; }
        )");
        // On capture origRow pour toujours pointer vers le bon index contrôleur
        connect(cb, &QCheckBox::toggled, this, [this, origRow](bool checked) {
            onTogglePossede(origRow, checked);
        });
        cbLay->addWidget(cb);
        m_table->setCellWidget(di, 2, cbWidget);

        // ── Colonne Achetable ────────────────────────────────────────────
        bool canBuy = m_controller->canBuyBalise(b);
        QColor buyCol = b.possede ? QColor(0x88, 0x88, 0x88) : (canBuy ? QColor(0x2E, 0xCC, 0x71) : QColor(0xE7, 0x4C, 0x3C));
        QString buyTxt;
        if (b.possede)          buyTxt = "—  Déjà possédée";
        else if (b.prix == 0)   buyTxt = "✔  Gratuite";
        else if (eo < b.prix)   buyTxt = QString("✘  Manque %1 EO").arg(b.prix - eo);
        else                    buyTxt = "✔  Oui";
        m_table->setItem(di, 3, makeItem(buyTxt, buyCol));

        // ── Bouton suppression ───────────────────────────────────────────
        QPushButton* delBtn = new QPushButton("🗑");
        delBtn->setObjectName("delRowBtn");
        delBtn->setCursor(Qt::PointingHandCursor);
        connect(delBtn, &QPushButton::clicked, this, [this, nom = b.nom] {
            int row = -1;
            const auto& cur = m_controller->balises();
            for (int r = 0; r < cur.size(); ++r)
                if (cur[r].nom == nom) { row = r; break; }
            if (row < 0) return;
            auto rep = QMessageBox::question(this, "Supprimer la balise",
                                             QString("Supprimer « %1 » de ta liste de balises ?").arg(nom),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (rep == QMessageBox::Yes) m_controller->removeBalise(row);
        });
        m_table->setCellWidget(di, 4, delBtn);
    }

    m_infoLbl->setText(
        QString("  🚩 %1/%2 possédées  •  %3 achetable(s)  •  EO : %4")
            .arg(totalOwned).arg(balises.size()).arg(totalAchetable).arg(eo));
}

void BalisePage::onTogglePossede(int originalRow, bool checked) {
    m_controller->setBaliseOwned(originalRow, checked);
}

void BalisePage::onAddBaliseClicked() {
    AddBaliseDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    Balise b = dlg.getBalise();
    if (b.nom.isEmpty()) {
        QMessageBox::warning(this, "Nom manquant", "Donne un nom à la balise avant de l'ajouter.");
        return;
    }
    if (!m_controller->addBalise(b)) {
        QMessageBox::warning(this, "Balise existante",
                             QString("« %1 » est déjà dans ta liste de balises.").arg(b.nom));
    }
}