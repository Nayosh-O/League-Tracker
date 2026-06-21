#include "skinpage.h"
#include "../controller/appcontroller.h"
#include "addskindialog.h"
#include "toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QLabel>
#include <algorithm>

// ── Ordre hiérarchique des raretés ──────────────────────────────────────────
static int rareteRank(const QString& r) {
    if (r == "Basique")      return 0;
    if (r == "Epique")       return 1;
    if (r == "Legendaire")   return 2;
    if (r == "Fantastique")  return 3;
    if (r == "Ultime")       return 4;
    if (r == "Exalte")       return 5;
    if (r == "Transcendant") return 6;
    if (r == "Gratuit")      return 7;
    return 8;
}

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
QCheckBox { margin: 0 auto; }
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

SkinPage::SkinPage(AppController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller)
{
    setStyleSheet(PAGE_STYLE);
    QVBoxLayout* L = new QVBoxLayout(this);
    L->setContentsMargins(0, 0, 0, 0);
    L->setSpacing(0);

    // ── Barre supérieure (recherche + info + bouton ajout) ──────────────────
    m_search = new QLineEdit;
    m_search->setObjectName("search");
    m_search->setPlaceholderText("🔍  Rechercher un skin ou un champion...");
    connect(m_search, &QLineEdit::textChanged, this, &SkinPage::refresh);

    m_infoLbl = new QLabel;
    m_infoLbl->setObjectName("infoLbl");

    QPushButton* addBtn = new QPushButton("✚  Nouveau skin");
    addBtn->setObjectName("addBtn");
    connect(addBtn, &QPushButton::clicked, this, &SkinPage::onAddSkinClicked);

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
        "Nom",
        "Champion",
        "Rareté",
        "Prix",
        "Possédé"
    });
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SkinPage::refresh);
    sortL->addWidget(m_sortCombo);

    m_sortDirBtn = new QPushButton("↑");
    m_sortDirBtn->setObjectName("sortDirBtn");
    m_sortDirBtn->setToolTip("Inverser l'ordre");
    connect(m_sortDirBtn, &QPushButton::clicked, this, &SkinPage::onSortDirToggled);
    sortL->addWidget(m_sortDirBtn);

    sortL->addStretch();
    L->addWidget(sortBar);

    // ── Tableau ───────────────────────────────────────────────────────────
    m_table = new QTableWidget;
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels(
        {"Nom du skin", "Champion", "Prix (EO)", "Rareté",
         "Champ. possédé", "Possédé", "Achetable", ""});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c <= 7; ++c)
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSortIndicatorShown(true);
    m_table->horizontalHeader()->setSectionsClickable(true);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &SkinPage::onHeaderClicked);
    L->addWidget(m_table, 1);

    connect(m_controller, &AppController::dataChanged, this, &SkinPage::refresh);
    refresh();
}

void SkinPage::focusSearch() {
    m_search->setFocus(Qt::ShortcutFocusReason);
    m_search->selectAll();
}

void SkinPage::onSortDirToggled() {
    m_sortAsc = !m_sortAsc;
    m_sortDirBtn->setText(m_sortAsc ? "↑" : "↓");
    refresh();
}

// Clic direct sur un en-tête de colonne : raccourci pratique en plus du
// combo « Trier par ». Un premier clic sélectionne ce critère (ordre
// croissant), un second clic sur la même colonne inverse juste le sens —
// exactement comme avec le bouton ↑/↓ dédié.
void SkinPage::onHeaderClicked(int column) {
    int sortIdx = -1;
    switch (column) {
    case 0: sortIdx = 0; break; // Nom du skin
    case 1: sortIdx = 1; break; // Champion
    case 2: sortIdx = 3; break; // Prix (EO)
    case 3: sortIdx = 2; break; // Rareté
    case 5: sortIdx = 4; break; // Possédé
    default: return; // colonnes non triables : Champ. possédé, Achetable, 🗑
    }
    applySortFromColumn(sortIdx);
}

void SkinPage::applySortFromColumn(int comboIndex) {
    if (m_sortCombo->currentIndex() == comboIndex) {
        onSortDirToggled();
    } else {
        m_sortAsc = true;
        m_sortDirBtn->setText("↑");
        m_sortCombo->setCurrentIndex(comboIndex); // déclenche refresh() via le signal déjà connecté
    }
}

void SkinPage::refresh() {
    const auto& skins = m_controller->skins();
    int eo = m_controller->essenceOrange();

    // ── Stats globales (toujours sur la collection complète, pas filtrée) ──
    int totalOwned = 0, totalAchetable = 0;
    for (const auto& s : skins) {
        if (s.possede) ++totalOwned;
        if (m_controller->canBuySkin(s)) ++totalAchetable;
    }

    // ── Filtrage par recherche (nom du skin ou du champion) ─────────────────
    const QString searchTxt = m_search ? m_search->text().trimmed().toLower() : QString();
    QVector<int> idx;
    idx.reserve(skins.size());
    for (int i = 0; i < skins.size(); ++i) {
        if (!searchTxt.isEmpty() &&
            !skins[i].nom.toLower().contains(searchTxt) &&
            !skins[i].champion.toLower().contains(searchTxt))
            continue;
        idx.append(i);
    }

    // ── Tri ──────────────────────────────────────────────────────────────
    int  mode = m_sortCombo ? m_sortCombo->currentIndex() : 0;
    bool asc  = m_sortAsc;

    std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
        const Skin& sa = skins[a];
        const Skin& sb = skins[b];
        bool res = false;
        switch (mode) {
        case 0: res = sa.nom.toLower()      < sb.nom.toLower();      break; // Nom
        case 1: res = sa.champion.toLower() < sb.champion.toLower(); break; // Champion
        case 2: res = rareteRank(sa.rarete) < rareteRank(sb.rarete); break; // Rareté
        case 3: res = sa.prix               < sb.prix;               break; // Prix
        case 4: res = (sa.possede > sb.possede);                     break; // Possédé d'abord
        }
        return asc ? res : !res;
    });

    // Indicateur visuel (petite flèche) sur l'en-tête de colonne correspondant
    int sortCol = -1;
    switch (mode) {
    case 0: sortCol = 0; break; // Nom
    case 1: sortCol = 1; break; // Champion
    case 2: sortCol = 3; break; // Rareté
    case 3: sortCol = 2; break; // Prix
    case 4: sortCol = 5; break; // Possédé
    }
    if (sortCol >= 0)
        m_table->horizontalHeader()->setSortIndicator(sortCol, asc ? Qt::AscendingOrder : Qt::DescendingOrder);

    // ── Remplissage du tableau ────────────────────────────────────────────
    m_table->setRowCount(idx.size());

    for (int di = 0; di < idx.size(); ++di) {
        const Skin& s = skins[idx[di]];

        bool champPos = m_controller->isChampionOwned(s.champion);
        bool canBuy   = m_controller->canBuySkin(s);

        auto makeItem = [](const QString& txt, const QColor& col = QColor(0xC8, 0xAA, 0x6E)) {
            auto* it = new QTableWidgetItem(txt);
            it->setForeground(col);
            return it;
        };

        m_table->setItem(di, 0, makeItem(s.nom));
        m_table->setItem(di, 1, makeItem(s.champion));
        m_table->setItem(di, 2, makeItem(s.gratuit ? "Gratuit" : QString::number(s.prix)));

        // Couleur par rareté
        QColor rarCol("#C8AA6E");
        if      (s.rarete == "Epique")        rarCol = QColor(0x9B, 0x59, 0xB6);
        else if (s.rarete == "Legendaire")    rarCol = QColor(0xFF, 0x8C, 0x00);
        else if (s.rarete == "Fantastique")   rarCol = QColor(0xE9, 0x1E, 0x8C);
        else if (s.rarete == "Ultime")        rarCol = QColor(0xE7, 0x4C, 0x3C);
        else if (s.rarete == "Exalte")        rarCol = QColor(0xFF, 0xD7, 0x00);
        else if (s.rarete == "Transcendant")  rarCol = QColor(0x00, 0xBC, 0xD4);
        else if (s.rarete == "Gratuit")       rarCol = QColor(0x2E, 0xCC, 0x71);
        m_table->setItem(di, 3, makeItem(s.rarete, rarCol));

        QColor posCol = champPos ? QColor(0x2E, 0xCC, 0x71) : QColor(0xE7, 0x4C, 0x3C);
        m_table->setItem(di, 4, makeItem(champPos ? "✔ Oui" : "✘ Non", posCol));

        // Checkbox Possédé
        QWidget* cbWidget = new QWidget;
        QHBoxLayout* cbLay = new QHBoxLayout(cbWidget);
        cbLay->setContentsMargins(0, 0, 0, 0);
        cbLay->setAlignment(Qt::AlignCenter);
        QCheckBox* cb = new QCheckBox;
        cb->setChecked(s.possede);
        cb->setStyleSheet(R"(
            QCheckBox::indicator{width:16px;height:16px;border:2px solid #C89B3C;border-radius:3px;background:#1E2328;}
            QCheckBox::indicator:checked{background:#C89B3C;}
        )");
        connect(cb, &QCheckBox::toggled, this, [this, nom = s.nom](bool checked) {
            const auto& cur = m_controller->skins();
            for (int r = 0; r < cur.size(); ++r)
                if (cur[r].nom == nom) { m_controller->setSkinOwned(r, checked); break; }
            Toast::show(this,
                        checked ? QString("« %1 » marqué possédé").arg(nom)
                                : QString("« %1 » marqué non possédé").arg(nom),
                        checked ? Toast::Success : Toast::Info);
        });
        cbLay->addWidget(cb);
        m_table->setCellWidget(di, 5, cbWidget);

        // Colonne Achetable
        QColor buyCol = canBuy ? QColor(0x2E, 0xCC, 0x71) : QColor(0xE7, 0x4C, 0x3C);
        QString buyTxt;
        if (s.possede)        buyTxt = "—  Déjà possédé";
        else if (!champPos)   buyTxt = "✘ Champ manquant";
        else if (s.gratuit)   buyTxt = "✔ Gratuit";
        else if (eo < s.prix) buyTxt = QString("✘ Manque %1 EO").arg(s.prix - eo);
        else                  buyTxt = "✔ Oui";
        if (s.possede) buyCol = QColor(0x88, 0x88, 0x88);
        m_table->setItem(di, 6, makeItem(buyTxt, buyCol));

        // Bouton suppression
        QPushButton* delBtn = new QPushButton("🗑");
        delBtn->setObjectName("delRowBtn");
        delBtn->setCursor(Qt::PointingHandCursor);
        connect(delBtn, &QPushButton::clicked, this, [this, nom = s.nom] {
            int row = -1;
            const auto& cur = m_controller->skins();
            for (int r = 0; r < cur.size(); ++r)
                if (cur[r].nom == nom) { row = r; break; }
            if (row < 0) return;

            // On garde une copie avant suppression : un clic sur « Annuler »
            // dans le toast la réinjecte via addSkin(), sans avoir à
            // interrompre l'utilisateur avec une boîte de confirmation
            // avant l'action.
            Skin removed = cur[row];
            m_controller->removeSkin(row);
            Toast::show(this, QString("Skin « %1 » supprimé").arg(nom), Toast::Danger,
                        "Annuler", [this, removed] { m_controller->addSkin(removed); });
        });
        m_table->setCellWidget(di, 7, delBtn);
    }

    m_infoLbl->setText(
        QString("  ✨ %1/%2 possédés  •  %3 achetable(s)  •  EO disponible : %4")
            .arg(totalOwned).arg(skins.size()).arg(totalAchetable).arg(eo));
}

void SkinPage::onAddSkinClicked() {
    QStringList champNames;
    for (const auto& c : m_controller->champions())
        champNames << c.nom;
    champNames.sort(Qt::CaseInsensitive);

    AddSkinDialog dlg(champNames, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Skin s = dlg.getSkin();
    if (s.nom.isEmpty()) {
        QMessageBox::warning(this, "Nom manquant", "Donne un nom au skin avant de l'ajouter.");
        return;
    }
    if (!m_controller->addSkin(s)) {
        QMessageBox::warning(this, "Skin existant",
                             QString("« %1 » est déjà dans ta liste de skins.").arg(s.nom));
    }
}