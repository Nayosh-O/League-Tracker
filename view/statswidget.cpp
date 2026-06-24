#include "statswidget.h"
#include "../controller/appcontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QFont>
#include <QStyle>
#include <QLegend>
#include <QPieSlice>
#include <QDateTime>
#include <QMap>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <algorithm>

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
QLabel#listLblGold { color: #FFD700; font-size: 12px; line-height: 1.6; }
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

// Variante pour les cartes qui enveloppent un QChartView (pas de marge basse,
// le graphique remplit toute la carte sous le titre).
static QWidget* makeChartCard(const QString& title, QWidget* chartView, QWidget* extra = nullptr) {
    QWidget* card = new QWidget;
    card->setObjectName("card");
    QVBoxLayout* L = new QVBoxLayout(card);
    L->setContentsMargins(0,0,0,0);
    L->setSpacing(0);
    QLabel* t = new QLabel(title);
    t->setObjectName("cardTitle");
    L->addWidget(t);
    L->addWidget(chartView);
    if (extra) L->addWidget(extra);
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

    // Carte Valeur de la collection (sur toute la largeur, 2e ligne de la grille)
    {
        QVBoxLayout* cl = new QVBoxLayout;
        cl->setSpacing(2);
        cl->addLayout(makeStatRow("Champions possédés :", m_lValeurChamps, "statGreen"));
        cl->addLayout(makeStatRow("Skins + balises possédés :", m_lValeurSkins, "statGreen"));
        grid->addWidget(makeCard("💰  Valeur de ta collection (au prix d'achat)", cl), 1, 0, 1, 3);
    }

    L->addLayout(grid);

    // ─── Graphiques ─────────────────────────────────────────────────────────
    QHBoxLayout* chartsL = new QHBoxLayout;
    chartsL->setSpacing(16);

    // Évolution EB / EO dans le temps
    {
        m_ebSeries = new QLineSeries;
        m_ebSeries->setName("Essence Bleue");
        QPen ebPen(QColor(0x5B, 0xC0, 0xDE)); ebPen.setWidth(2);
        m_ebSeries->setPen(ebPen);
        // Marqueurs visibles : indispensable quand il n'y a qu'un seul
        // point d'historique (ex. premier jour d'utilisation), sinon une
        // QLineSeries à un point ne trace aucun segment et n'affiche rien.
        m_ebSeries->setPointsVisible(true);

        m_eoSeries = new QLineSeries;
        m_eoSeries->setName("Essence Orange");
        QPen eoPen(QColor(0xC8, 0x9B, 0x3C)); eoPen.setWidth(2);
        m_eoSeries->setPen(eoPen);
        m_eoSeries->setPointsVisible(true);

        m_historyChart = new QChart;
        m_historyChart->addSeries(m_ebSeries);
        m_historyChart->addSeries(m_eoSeries);
        m_historyChart->legend()->setVisible(true);
        m_historyChart->legend()->setLabelColor(QColor(0xC8, 0xAA, 0x6E));
        m_historyChart->setBackgroundBrush(QBrush(QColor(0x0F, 0x19, 0x23)));
        m_historyChart->setBackgroundPen(QPen(Qt::NoPen));
        m_historyChart->setMargins(QMargins(8, 8, 8, 8));

        m_historyAxisX = new QDateTimeAxis;
        m_historyAxisX->setFormat("dd/MM");
        m_historyAxisX->setLabelsColor(QColor(0x7A, 0x7A, 0x7A));
        m_historyAxisX->setGridLineColor(QColor(0x1E, 0x23, 0x28));
        m_historyAxisX->setLinePenColor(QColor(0x3A, 0x3A, 0x3A));
        m_historyChart->addAxis(m_historyAxisX, Qt::AlignBottom);
        m_ebSeries->attachAxis(m_historyAxisX);
        m_eoSeries->attachAxis(m_historyAxisX);

        m_historyAxisY = new QValueAxis;
        m_historyAxisY->setLabelFormat("%i");
        m_historyAxisY->setLabelsColor(QColor(0x7A, 0x7A, 0x7A));
        m_historyAxisY->setGridLineColor(QColor(0x1E, 0x23, 0x28));
        m_historyAxisY->setLinePenColor(QColor(0x3A, 0x3A, 0x3A));
        m_historyChart->addAxis(m_historyAxisY, Qt::AlignLeft);
        m_ebSeries->attachAxis(m_historyAxisY);
        m_eoSeries->attachAxis(m_historyAxisY);

        m_historyChartView = new QChartView(m_historyChart);
        m_historyChartView->setRenderHint(QPainter::Antialiasing);
        m_historyChartView->setMinimumHeight(260);

        m_historyHintLbl = new QLabel;
        m_historyHintLbl->setObjectName("statKey");
        m_historyHintLbl->setAlignment(Qt::AlignCenter);
        m_historyHintLbl->setContentsMargins(0, 6, 0, 10);
        m_historyHintLbl->setWordWrap(true);

        chartsL->addWidget(makeChartCard("📈  Évolution de tes essences", m_historyChartView, m_historyHintLbl), 1);
    }

    // Répartition des skins par rareté
    {
        m_raritySeries = new QPieSeries;

        m_rarityChart = new QChart;
        m_rarityChart->addSeries(m_raritySeries);
        m_rarityChart->legend()->setVisible(true);
        m_rarityChart->legend()->setAlignment(Qt::AlignRight);
        m_rarityChart->legend()->setLabelColor(QColor(0xC8, 0xAA, 0x6E));
        m_rarityChart->setBackgroundBrush(QBrush(QColor(0x0F, 0x19, 0x23)));
        m_rarityChart->setBackgroundPen(QPen(Qt::NoPen));
        m_rarityChart->setMargins(QMargins(8, 8, 8, 8));

        m_rarityChartView = new QChartView(m_rarityChart);
        m_rarityChartView->setRenderHint(QPainter::Antialiasing);
        m_rarityChartView->setMinimumHeight(260);

        chartsL->addWidget(makeChartCard("🥧  Skins par rareté", m_rarityChartView), 1);
    }

    L->addLayout(chartsL);

    // ─── Priorité d'achat ───────────────────────────────────────────────────
    QLabel* t0 = new QLabel("★ Priorité d'achat");
    t0->setObjectName("sectionTitle");
    L->addWidget(t0);
    m_lListePriorite = new QLabel;
    m_lListePriorite->setObjectName("listLblGold");
    m_lListePriorite->setWordWrap(true);
    L->addWidget(m_lListePriorite);

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

    m_lValeurChamps->setText(QString::number(m_controller->valeurChampionsPossedes()) + " EB");
    m_lValeurSkins ->setText(QString::number(m_controller->valeurSkinsBalisesPossedes()) + " EO");

    m_prog->setRange(0, total);
    m_prog->setValue(owned);
    m_prog->setFormat(QString("%1 / %2  (%3%)").arg(owned).arg(total)
                          .arg(total > 0 ? owned * 100 / total : 0));

    // Listes : priorité d'abord, puis le reste des champions manquants
    QStringList priorite, missing, reduc;
    for (const auto& c : m_controller->champions()) {
        if (!c.possede) {
            QString entry = QString("• %1  (%2 EB)").arg(c.nom).arg(c.prixEffectif());
            if (eb >= c.prixEffectif()) entry += " ✔";
            if (c.prioritaire) priorite << ("★ " + entry.mid(2));
            else                missing  << entry;
        }
        if (c.prixReduit > 0)
            reduc << QString("• %1 :  %2 EB  →  %3 EB  (−%4)")
                         .arg(c.nom).arg(c.prixStandard).arg(c.prixReduit)
                         .arg(c.prixStandard - c.prixReduit);
    }
    m_lListePriorite->setText(priorite.isEmpty()
                                  ? "Aucun champion marqué prioritaire — ouvre la fiche d'un champion et coche « ★ À acheter en priorité »."
                                  : priorite.join("\n"));
    m_lListeMissing->setText(missing.isEmpty() ? "🎉 Tu possèdes tous les champions !" : missing.join("\n"));
    m_lListeReduc  ->setText(reduc.isEmpty()   ? "Aucune réduction active." : reduc.join("\n"));

    refreshHistoryChart();
    refreshRarityChart();
}

void StatsWidget::refreshHistoryChart() {
    const auto& hist = m_controller->essenceHistory();
    m_ebSeries->clear();
    m_eoSeries->clear();

    if (hist.isEmpty()) {
        // Pas encore d'historique (première utilisation) : on affiche
        // au moins le point actuel pour que le graphique ne soit pas vide.
        QDateTime now = QDateTime::currentDateTime();
        qint64 x = now.toMSecsSinceEpoch();
        m_ebSeries->append(x, m_controller->essenceBleu());
        m_eoSeries->append(x, m_controller->essenceOrange());
        m_historyAxisX->setRange(now.addDays(-1), now.addDays(1));
        m_historyAxisY->setRange(0, qMax(1, qMax(m_controller->essenceBleu(),
                                                 m_controller->essenceOrange())) * 1.1);
        m_historyHintLbl->setText("Un seul point pour l'instant : la courbe se dessinera "
                                  "dès ta prochaine modification d'EB/EO.");
        return;
    }

    // On parse d'abord TOUS les points, puis on les trie par date/heure
    // avant de les tracer : l'ordre d'insertion dans m_history n'est pas
    // forcément l'ordre chronologique réel d'affichage, notamment pour
    // d'anciens points sans heure (repli arbitraire à midi, cf. ci-dessous)
    // qui peuvent se retrouver positionnés avant ou après des points plus
    // récents avec heure précise. Sans ce tri, QLineSeries trace les
    // segments dans l'ordre d'insertion et la courbe peut "remonter dans
    // le temps" visuellement.
    struct Point { QDateTime dt; int eb; int eo; };
    QVector<Point> pts;
    pts.reserve(hist.size());
    for (const auto& h : hist) {
        // Historique fin : "yyyy-MM-dd HH:mm:ss". Repli sur l'ancien format
        // "yyyy-MM-dd" (un point par jour, sans heure) pour les points déjà
        // enregistrés avant ce changement — placés arbitrairement à midi.
        QDateTime dt = QDateTime::fromString(h.date, "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid()) {
            QDate d = QDate::fromString(h.date, "yyyy-MM-dd");
            dt = QDateTime(d, QTime(12, 0));
        }
        pts.append({dt, h.eb, h.eo});
    }
    std::stable_sort(pts.begin(), pts.end(),
                     [](const Point& a, const Point& b) { return a.dt < b.dt; });

    QDateTime dtMin, dtMax;
    int maxY = 1;
    for (int i = 0; i < pts.size(); ++i) {
        qint64 x = pts[i].dt.toMSecsSinceEpoch();
        if (i == 0) dtMin = pts[i].dt;
        dtMax = pts[i].dt;
        m_ebSeries->append(x, pts[i].eb);
        m_eoSeries->append(x, pts[i].eo);
        maxY = std::max({maxY, pts[i].eb, pts[i].eo});
    }

    // Marge autour de la plage de dates, proportionnelle à son étendue,
    // avec un plancher de 12h pour rester lisible même avec des points
    // très rapprochés (plusieurs changements dans la même heure).
    qint64 spanMs = qMax<qint64>(0, dtMin.msecsTo(dtMax));
    qint64 padMs  = qMax<qint64>(spanMs / 10, 12LL * 3600 * 1000);
    // En dessous de 2 jours d'étendue, affiche l'heure sur l'axe : sinon
    // plusieurs points du même jour partageraient le même libellé "dd/MM".
    m_historyAxisX->setFormat(spanMs < 2LL * 24 * 3600 * 1000 ? "dd/MM HH:mm" : "dd/MM");
    m_historyAxisX->setRange(dtMin.addMSecs(-padMs), dtMax.addMSecs(padMs));
    m_historyAxisY->setRange(0, maxY * 1.1);

    m_historyHintLbl->setText(hist.size() < 2
                                  ? "Un seul point enregistré : la courbe apparaîtra dès ta prochaine modification d'EB/EO."
                                  : QString());
}

void StatsWidget::refreshRarityChart() {
    m_raritySeries->clear();

    // Même hiérarchie et mêmes couleurs que dans SkinPage, pour rester
    // visuellement cohérent entre les deux vues.
    static const QVector<QPair<QString, QColor>> tiers = {
                                                           {"Basique",      QColor(0xC8, 0xAA, 0x6E)},
                                                           {"Epique",       QColor(0x9B, 0x59, 0xB6)},
                                                           {"Legendaire",   QColor(0xFF, 0x8C, 0x00)},
                                                           {"Fantastique",  QColor(0xE9, 0x1E, 0x8C)},
                                                           {"Ultime",       QColor(0xE7, 0x4C, 0x3C)},
                                                           {"Exalte",       QColor(0xFF, 0xD7, 0x00)},
                                                           {"Transcendant", QColor(0x00, 0xBC, 0xD4)},
                                                           {"Gratuit",      QColor(0x2E, 0xCC, 0x71)},
                                                           };

    QMap<QString, int> counts;
    for (const auto& s : m_controller->skins())
        counts[s.rarete] += 1;

    bool any = false;
    for (const auto& tier : tiers) {
        int n = counts.value(tier.first, 0);
        if (n <= 0) continue;
        any = true;
        QPieSlice* slice = m_raritySeries->append(QString("%1 (%2)").arg(tier.first).arg(n), n);
        slice->setBrush(tier.second);
        slice->setLabelVisible(false);
        slice->setBorderColor(QColor(0x0F, 0x19, 0x23));
    }
    if (!any) {
        QPieSlice* slice = m_raritySeries->append("Aucun skin", 1);
        slice->setBrush(QColor(0x3A, 0x3A, 0x3A));
    }
}