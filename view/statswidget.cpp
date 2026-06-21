#include "statswidget.h"
#include "../controller/appcontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QFont>
#include <QStyle>

static const char* STATS_STYLE = R"(
QWidget#statsRoot { background: #0A0E14; }
QWidget#card {
    background: #0F1923;
    border: 1px solid #1E2328;
    border-radius: 8px;
}
QLabel#cardTitle {
    color: #C89B3C;
    font-size: 13px;
    font-weight: bold;
    padding: 12px 16px 4px 16px;
    border-bottom: 1px solid #1E2328;
}
QLabel#statKey   { color: #7A7A7A; font-size: 12px; padding: 4px 16px; }
QLabel#statVal   { color: #C8AA6E; font-size: 14px; font-weight: bold; padding: 4px 16px; }
QLabel#statGreen { color: #2ECC71; font-size: 14px; font-weight: bold; padding: 4px 16px; }
QLabel#statRed   { color: #E74C3C; font-size: 14px; font-weight: bold; padding: 4px 16px; }
QProgressBar {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    border-radius: 6px;
    text-align: center;
    color: #000;
    font-weight: bold;
    height: 22px;
}
QProgressBar::chunk { background: #C89B3C; border-radius: 5px; }
QLabel#sectionTitle { color: #C89B3C; font-size: 15px; font-weight: bold; padding: 16px 0 6px 0; }
QLabel#listLbl { color: #C8AA6E; font-size: 12px; line-height: 1.6; }
)";

StatsWidget::StatsWidget(AppController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller)
{
    setObjectName("statsRoot");
    setStyleSheet(STATS_STYLE);
    buildUi();
    connect(m_controller, &AppController::dataChanged, this, &StatsWidget::refresh);
    refresh();
}

static QWidget* makeCard(const QString& title, QLayout* inner) {
    QWidget* card = new QWidget;
    card->setObjectName("card");
    QVBoxLayout* L = new QVBoxLayout(card);
    L->setContentsMargins(0,0,0,12);
    L->setSpacing(0);
    QLabel* t = new QLabel(title);
    t->setObjectName("cardTitle");
    L->addWidget(t);
    L->addLayout(inner);
    return card;
}

static QHBoxLayout* makeStatRow(const QString& key, QLabel*& valLbl, const QString& id = "statVal") {
    auto* row = new QHBoxLayout;
    row->setContentsMargins(0,0,0,0);
    QLabel* k = new QLabel(key); k->setObjectName("statKey");
    valLbl = new QLabel("—");   valLbl->setObjectName(id);
    row->addWidget(k);
    row->addStretch();
    row->addWidget(valLbl);
    return row;
}

void StatsWidget::buildUi() {
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{background:#0A0E14;border:none;}");
    QVBoxLayout* outerL = new QVBoxLayout(this);
    outerL->setContentsMargins(0,0,0,0);
    outerL->addWidget(scroll);

    QWidget* inner = new QWidget;
    inner->setStyleSheet("background:#0A0E14;");
    scroll->setWidget(inner);
    QVBoxLayout* L = new QVBoxLayout(inner);
    L->setContentsMargins(24,20,24,24);
    L->setSpacing(16);

    // ─── Grille de cartes stats ───────────────────────────────────────────────
    QGridLayout* grid = new QGridLayout;
    grid->setSpacing(16);

    // Carte Champions
    {
        QVBoxLayout* cl = new QVBoxLayout;
        cl->setSpacing(2);
        cl->addLayout(makeStatRow("Total champions :", m_lTotal));
        cl->addLayout(makeStatRow("Possédés :",        m_lOwned, "statGreen"));
        cl->addLayout(makeStatRow("À acheter :",       m_lToBuy, "statRed"));
        m_prog = new QProgressBar;
        QHBoxLayout* ph = new QHBoxLayout;
        ph->setContentsMargins(16,8,16,0);
        ph->addWidget(m_prog);
        cl->addLayout(ph);
        grid->addWidget(makeCard("⚔  Champions", cl), 0, 0);
    }

    // Carte Essence Bleue
    {
        QVBoxLayout* cl = new QVBoxLayout;
        cl->setSpacing(2);
        cl->addLayout(makeStatRow("Coût total restant :", m_lCout));
        cl->addLayout(makeStatRow("Ton EB actuel :",     m_lEb));
        cl->addLayout(makeStatRow("EB après tout achat :", m_lManque, "statRed"));
        grid->addWidget(makeCard("💎  Essence Bleue", cl), 0, 1);
    }

    // Carte Essence Orange
    {
        QVBoxLayout* cl = new QVBoxLayout;
        cl->setSpacing(2);
        cl->addLayout(makeStatRow("EO disponible :", m_lEo, "statGreen"));
        grid->addWidget(makeCard("✨  Essence Orange", cl), 0, 2);
    }

    L->addLayout(grid);

    // ─── Champions manquants ──────────────────────────────────────────────────
    QLabel* t1 = new QLabel("Champions à acheter");
    t1->setObjectName("sectionTitle");
    L->addWidget(t1);
    m_lListeMissing = new QLabel;
    m_lListeMissing->setObjectName("listLbl");
    m_lListeMissing->setWordWrap(true);
    L->addWidget(m_lListeMissing);

    // ─── Champions avec réduction ─────────────────────────────────────────────
    QLabel* t2 = new QLabel("Champions avec réduction disponible");
    t2->setObjectName("sectionTitle");
    L->addWidget(t2);
    m_lListeReduc = new QLabel;
    m_lListeReduc->setObjectName("listLbl");
    m_lListeReduc->setWordWrap(true);
    L->addWidget(m_lListeReduc);
    L->addStretch();
}

void StatsWidget::refresh() {
    int total  = m_controller->totalChampions();
    int owned  = m_controller->champsOwned();
    int toBuy  = m_controller->champsToBuy();
    int cout   = m_controller->coutTotalRestant();
    int eb     = m_controller->essenceBleu();
    int apres  = m_controller->ebApresAchat();
    int eo     = m_controller->essenceOrange();

    m_lTotal->setText(QString::number(total));
    m_lOwned->setText(QString::number(owned));
    m_lToBuy->setText(toBuy > 0 ? QString::number(toBuy) : "0 ✔");
    m_lCout ->setText(QString::number(cout) + " EB");
    m_lEb   ->setText(QString::number(eb)   + " EB");

    QString apresStr = QString::number(apres) + " EB";
    m_lManque->setText(apresStr);
    m_lManque->setObjectName(apres >= 0 ? "statGreen" : "statRed");
    m_lManque->style()->unpolish(m_lManque);
    m_lManque->style()->polish(m_lManque);

    m_lEo   ->setText(QString::number(eo) + " EO");

    m_prog->setRange(0, total);
    m_prog->setValue(owned);
    m_prog->setFormat(QString("%1 / %2  (%3%)").arg(owned).arg(total)
                      .arg(total > 0 ? owned * 100 / total : 0));

    // Listes
    QStringList missing, reduc;
    for (const auto& c : m_controller->champions()) {
        if (!c.possede) {
            QString entry = QString("• %1  (%2 EB)").arg(c.nom).arg(c.prixEffectif());
            if (eb >= c.prixEffectif()) entry += " ✔";
            missing << entry;
        }
        if (c.prixReduit > 0)
            reduc << QString("• %1 :  %2 EB  →  %3 EB  (−%4)")
                        .arg(c.nom).arg(c.prixStandard).arg(c.prixReduit)
                        .arg(c.prixStandard - c.prixReduit);
    }
    m_lListeMissing->setText(missing.isEmpty() ? "🎉 Tu possèdes tous les champions !" : missing.join("\n"));
    m_lListeReduc  ->setText(reduc.isEmpty()   ? "Aucune réduction active." : reduc.join("\n"));
}
