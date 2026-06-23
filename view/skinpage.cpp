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

/* ── Barres de tri / filtre ── */
QLabel#sortLbl, QLabel#filterLbl { color: #888; font-size: 12px; }
QComboBox#sortCombo, QComboBox#filterCombo {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    color: #C8AA6E;
    padding: 4px 8px;
    border-radius: 4px;
    font-size: 12px;
    min-width: 150px;
}
QComboBox#sortCombo:focus,
QComboBox#filterCombo:focus { border-color: #C89B3C; }
QComboBox#sortCombo::drop-down,
QComboBox#filterCombo::drop-down { border: none; }
QComboBox#sortCombo QAbstractItemView,
QComboBox#filterCombo QAbstractItemView {
    background: #1E2328;
    color: #C8AA6E;
    selection-background-color: #C89B3C;
    selection-color: #000;
}
/* Filtre actif : bordure dorée pour indiquer visuellement qu'il filtre */
QComboBox#filterCombo[active="true"] {
    border-color: #C89B3C;
    color: #C89B3C;
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

/* Bouton "Réinitialiser les filtres" */
QPushButton#resetBtn {
    background: transparent;
    border: 1px solid #888;
    color: #888;
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 12px;
}
QPushButton#resetBtn:hover { border-color: #C89B3C; color: #C89B3C; }

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
    m_sortCombo->addItems({"Nom", "Champion", "Rareté", "Prix", "Possédé"});
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

    // ── Barre de filtres combinés ─────────────────────────────────────────
    QWidget* filterBar = new QWidget;
    filterBar->setStyleSheet(
        "QWidget { background: #080D13; border-bottom: 1px solid #1A1A2A; }");
    QHBoxLayout* filterL = new QHBoxLayout(filterBar);
    filterL->setContentsMargins(16, 5, 16, 5);
    filterL->setSpacing(8);

    QLabel* filterLbl = new QLabel("  Filtrer :");
    filterLbl->setObjectName("filterLbl");
    filterL->addWidget(filterLbl);

    // Filtre rareté
    m_filterRarete = new QComboBox;
    m_filterRarete->setObjectName("filterCombo");
    m_filterRarete->setToolTip("Filtrer par rareté de skin");
    m_filterRarete->addItems({
        "✦ Toutes raretés",
        "Basique",
        "Epique",
        "Legendaire",
        "Fantastique",
        "Ultime",
        "Exalte",
        "Transcendant",
        "Gratuit"
    });
    connect(m_filterRarete, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] { updateResetBtn(); refresh(); });
    filterL->addWidget(m_filterRarete);

    // Filtre champion possédé
    m_filterChamp = new QComboBox;
    m_filterChamp->setObjectName("filterCombo");
    m_filterChamp->setToolTip("Filtrer selon si tu possèdes le champion associé");
    m_filterChamp->addItems({
        "👤 Tous les champions",
        "✔ Champion possédé",
        "✘ Champion non possédé"
    });
    connect(m_filterChamp, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] { updateResetBtn(); refresh(); });
    filterL->addWidget(m_filterChamp);

    // Filtre skin possédé
    m_filterPossede = new QComboBox;
    m_filterPossede->setObjectName("filterCombo");
    m_filterPossede->setToolTip("Filtrer par statut de possession du skin");
    m_filterPossede->addItems({
        "🎨 Tous les skins",
        "✔ Skin possédé",
        "✘ Skin non possédé"
    });
    connect(m_filterPossede, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] { updateResetBtn(); refresh(); });
    filterL->addWidget(m_filterPossede);

    // Bouton réinitialisation (visible seulement quand un filtre est actif)
    m_resetBtn = new QPushButton("✕  Réinitialiser les filtres");
    m_resetBtn->setObjectName("resetBtn");
    m_resetBtn->setVisible(false);
    connect(m_resetBtn, &QPushButton::clicked, this, &SkinPage::onResetFilters);
    filterL->addWidget(m_resetBtn);

    filterL->addStretch();
    L->addWidget(filterBar);

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

void SkinPage::onHeaderClicked(int logicalIndex) {
    int target = -1;
    switch (logicalIndex) {
    case 0: target = 0; break;
    case 1: target = 1; break;
    case 2: target = 3; break;
    case 3: target = 2; break;
    case 5: target = 4; break;
    default: return;
    }
    if (m_sortCombo->currentIndex() == target)
        onSortDirToggled();
    else
        m_sortCombo->setCurrentIndex(target);
}

void SkinPage::applySortFromColumn(int comboIndex) {
    if (m_sortCombo->currentIndex() == comboIndex) {
        onSortDirToggled();
    } else {
        m_sortAsc = true;
        m_sortDirBtn->setText("↑");
        m_sortCombo->setCurrentIndex(comboIndex);
    }
}

// Met à jour la visibilité du bouton "Réinitialiser" et la propriété
// CSS "active" sur chaque combo de filtre (pour la bordure dorée).
void SkinPage::updateResetBtn() {
    bool anyActive = m_filterRarete->currentIndex()  != 0
                     || m_filterChamp->currentIndex()   != 0
                     || m_filterPossede->currentIndex() != 0;
    m_resetBtn->setVisible(anyActive);

    // Propriété CSS pour la bordure dorée sur les filtres actifs
    m_filterRarete->setProperty("active",
                                m_filterRarete->currentIndex() != 0 ? "true" : "false");
    m_filterChamp->setProperty("active",
                               m_filterChamp->currentIndex() != 0 ? "true" : "false");
    m_filterPossede->setProperty("active",
                                 m_filterPossede->currentIndex() != 0 ? "true" : "false");

    // Forcer le recalcul du style Qt (propriétés dynamiques nécessitent ça)
    m_filterRarete->style()->unpolish(m_filterRarete);
    m_filterRarete->style()->polish(m_filterRarete);
    m_filterChamp->style()->unpolish(m_filterChamp);
    m_filterChamp->style()->polish(m_filterChamp);
    m_filterPossede->style()->unpolish(m_filterPossede);
    m_filterPossede->style()->polish(m_filterPossede);
}

void SkinPage::onResetFilters() {
    // blockSignals pour ne déclencher qu'un seul refresh à la fin
    m_filterRarete->blockSignals(true);
    m_filterChamp->blockSignals(true);
    m_filterPossede->blockSignals(true);

    m_filterRarete->setCurrentIndex(0);
    m_filterChamp->setCurrentIndex(0);
    m_filterPossede->setCurrentIndex(0);

    m_filterRarete->blockSignals(false);
    m_filterChamp->blockSignals(false);
    m_filterPossede->blockSignals(false);

    updateResetBtn();
    refresh();
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

    // ── Lecture des filtres actifs ────────────────────────────────────────
    const QString searchTxt    = m_search        ? m_search->text().trimmed().toLower() : QString();
    const int     filtreRarete = m_filterRarete  ? m_filterRarete->currentIndex()       : 0;
    const int     filtreChamp  = m_filterChamp   ? m_filterChamp->currentIndex()        : 0;
    const int     filtrePossede= m_filterPossede ? m_filterPossede->currentIndex()      : 0;

    // Mapping index combo → nom de rareté (même ordre que addItems())
    static const QString raretesCombo[] = {
        "", "Basique", "Epique", "Legendaire", "Fantastique",
        "Ultime", "Exalte", "Transcendant", "Gratuit"
    };
    const QString rareteVoulue = (filtreRarete > 0) ? raretesCombo[filtreRarete] : QString();

    // ── Filtrage (recherche texte + filtres combinés) ─────────────────────
    QVector<int> idx;
    idx.reserve(skins.size());
    for (int i = 0; i < skins.size(); ++i) {
        const Skin& s = skins[i];

        // Recherche texte (nom du skin ou du champion)
        if (!searchTxt.isEmpty() &&
            !s.nom.toLower().contains(searchTxt) &&
            !s.champion.toLower().contains(searchTxt))
            continue;

        // Filtre rareté
        if (!rareteVoulue.isEmpty() && s.rarete != rareteVoulue)
            continue;

        // Filtre champion possédé/non possédé
        if (filtreChamp != 0) {
            bool champPossede = m_controller->isChampionOwned(s.champion);
            if (filtreChamp == 1 && !champPossede) continue; // veut possédé
            if (filtreChamp == 2 &&  champPossede) continue; // veut non possédé
        }

        // Filtre skin possédé/non possédé
        if (filtrePossede == 1 && !s.possede) continue;
        if (filtrePossede == 2 &&  s.possede) continue;

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
        case 0: res = sa.nom.toLower()      < sb.nom.toLower();      break;
        case 1: res = sa.champion.toLower() < sb.champion.toLower(); break;
        case 2: res = rareteRank(sa.rarete) < rareteRank(sb.rarete); break;
        case 3: res = sa.prix               < sb.prix;               break;
        case 4: res = (sa.possede > sb.possede);                     break;
        }
        return asc ? res : !res;
    });

    // Indicateur visuel sur l'en-tête de colonne correspondant
    int sortCol = -1;
    switch (mode) {
    case 0: sortCol = 0; break;
    case 1: sortCol = 1; break;
    case 2: sortCol = 3; break;
    case 3: sortCol = 2; break;
    case 4: sortCol = 5; break;
    }
    if (sortCol >= 0)
        m_table->horizontalHeader()->setSortIndicator(
            sortCol, asc ? Qt::AscendingOrder : Qt::DescendingOrder);

    // ── Remplissage du tableau ────────────────────────────────────────────
    // setUpdatesEnabled(false) regroupe tous les repaints en un seul à la fin.
    // Les widgets sont recréés systématiquement : setCellWidget() détruit
    // l'ancien automatiquement, donc pas de fuite mémoire ni de risque d'appeler
    // disconnect() sur un signal encore actif sur la pile d'appel.
    m_table->setUpdatesEnabled(false);
    m_table->setRowCount(idx.size());

    for (int di = 0; di < idx.size(); ++di) {
        const int   origIdx = idx[di]; // index réel dans m_controller->skins()
        const Skin& s       = skins[origIdx];

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
        QColor rarCol(0xC8, 0xAA, 0x6E);
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

        // ── Checkbox Possédé ─────────────────────────────────────────────
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
        connect(cb, &QCheckBox::toggled, this, [this, origIdx, nom = s.nom](bool checked) {
            m_controller->setSkinOwned(origIdx, checked);
            Toast::show(this,
                        checked ? QString("« %1 » marqué possédé").arg(nom)
                                : QString("« %1 » marqué non possédé").arg(nom),
                        checked ? Toast::Success : Toast::Info);
        });
        cbLay->addWidget(cb);
        m_table->setCellWidget(di, 5, cbWidget);

        // ── Colonne Achetable ─────────────────────────────────────────────
        QColor buyCol = canBuy ? QColor(0x2E, 0xCC, 0x71) : QColor(0xE7, 0x4C, 0x3C);
        QString buyTxt;
        if (s.possede)        buyTxt = "—  Déjà possédé";
        else if (!champPos)   buyTxt = "✘ Champ manquant";
        else if (s.gratuit)   buyTxt = "✔ Gratuit";
        else if (eo < s.prix) buyTxt = QString("✘ Manque %1 EO").arg(s.prix - eo);
        else                  buyTxt = "✔ Oui";
        if (s.possede) buyCol = QColor(0x88, 0x88, 0x88);
        m_table->setItem(di, 6, makeItem(buyTxt, buyCol));

        // ── Bouton suppression ────────────────────────────────────────────
        QPushButton* delBtn = new QPushButton("🗑");
        delBtn->setObjectName("delRowBtn");
        delBtn->setCursor(Qt::PointingHandCursor);
        Skin skinCopy = s;
        connect(delBtn, &QPushButton::clicked, this, [this, origIdx, skinCopy] {
            const auto& cur = m_controller->skins();
            if (origIdx < 0 || origIdx >= cur.size()) return;
            const QString nom = cur[origIdx].nom;
            m_controller->removeSkin(origIdx);
            Toast::show(this, QString("Skin « %1 » supprimé").arg(nom), Toast::Danger,
                        "Annuler", [this, skinCopy] { m_controller->addSkin(skinCopy); });
        });
        m_table->setCellWidget(di, 7, delBtn);
    }

    m_table->setUpdatesEnabled(true);

    // ── Label d'info : compte filtré vs. total ────────────────────────────
    // Si des filtres sont actifs, on indique combien de skins correspondent
    // parmi la collection totale, pour que l'utilisateur garde le contexte.
    bool filtered = !searchTxt.isEmpty()
                    || filtreRarete   != 0
                    || filtreChamp    != 0
                    || filtrePossede  != 0;

    QString info;
    if (filtered) {
        info = QString("  🔎 %1 résultat(s)  •  ✨ %2/%3 possédés  •  %4 achetable(s)  •  EO : %5")
                   .arg(idx.size())
                   .arg(totalOwned).arg(skins.size())
                   .arg(totalAchetable).arg(eo);
    } else {
        info = QString("  ✨ %1/%2 possédés  •  %3 achetable(s)  •  EO disponible : %4")
                   .arg(totalOwned).arg(skins.size()).arg(totalAchetable).arg(eo);
    }
    m_infoLbl->setText(info);
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