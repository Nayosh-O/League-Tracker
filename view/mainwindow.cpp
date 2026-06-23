#include "mainwindow.h"
#include "../controller/appcontroller.h"
#include "championgridpage.h"
#include "skinpage.h"
#include "balisepage.h"
#include "statswidget.h"
#include "toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <QApplication>
#include <QShortcut>
#include <QKeySequence>

static const char* NAV_LABELS[] = {"⚔  Champions", "✨  Skins", "🚩  Balises", "📊  Stats"};
static const char* STYLE_MAIN = R"(
QMainWindow, QWidget#root { background: #010A13; }
QWidget#sidebar {
    background: #010A13;
    border-right: 2px solid #1E2328;
    min-width: 160px; max-width: 160px;
}
QLabel#title {
    color: #C89B3C;
    font-size: 15px;
    font-weight: bold;
    padding: 16px 8px 8px 8px;
    font-family: "Beaufort for LOL", "Georgia", serif;
}
QPushButton#nav {
    background: transparent;
    color: #A0A0A0;
    border: none;
    text-align: left;
    padding: 12px 16px;
    font-size: 13px;
}
QPushButton#nav:hover  { color: #C89B3C; background: #0F1923; }
QPushButton#nav:checked {
    color: #C89B3C;
    background: #0F1923;
    border-left: 3px solid #C89B3C;
    font-weight: bold;
}
QWidget#essenceBox {
    background: #0F1923;
    border-top: 1px solid #1E2328;
    padding: 8px;
}
QLabel#essLbl { color: #7A7A7A; font-size: 11px; }
QLabel#essVal { color: #C89B3C; font-size: 13px; font-weight: bold; }
QPushButton#essBtn {
    background: transparent;
    border: 1px solid #C89B3C;
    color: #C89B3C;
    border-radius: 3px;
    padding: 2px 6px;
    font-size: 10px;
}
QPushButton#essBtn:hover { background: #C89B3C; color: #000; }
QWidget#ioBox {
    background: #0F1923;
    border-top: 1px solid #1E2328;
    padding: 8px 12px;
}
QPushButton#ioBtn {
    background: transparent;
    border: 1px solid #5B8DBE;
    color: #5B8DBE;
    border-radius: 3px;
    padding: 6px;
    font-size: 11px;
    font-weight: bold;
}
QPushButton#ioBtn:hover { background: #5B8DBE; color: #000; }
QPushButton#quitBtn {
    background: transparent;
    border: 1px solid #5C3A3A;
    color: #C97A7A;
    border-radius: 3px;
    padding: 8px;
    margin: 8px;
    font-size: 12px;
    font-weight: bold;
}
QPushButton#quitBtn:hover { background: #5C2020; border-color: #C97A7A; color: #FFFFFF; }
QWidget#topBar {
    background: #010A13;
    border-bottom: 2px solid #C89B3C;
    min-height: 48px; max-height: 48px;
}
QLabel#topTitle { color: #C89B3C; font-size: 18px; font-weight: bold; padding: 0 12px; }
)";

MainWindow::MainWindow(AppController* controller, QWidget* parent)
    : QMainWindow(parent), m_controller(controller)
{
    buildUi();
    connect(m_controller, &AppController::dataChanged,
            this, &MainWindow::refreshEssence);
    refreshEssence();
}

void MainWindow::buildUi() {
    setWindowTitle("League Tracker");
    resize(1280, 800);
    setStyleSheet(STYLE_MAIN);

    QWidget* root = new QWidget(this);
    root->setObjectName("root");
    setCentralWidget(root);

    QHBoxLayout* rootLay = new QHBoxLayout(root);
    rootLay->setContentsMargins(0,0,0,0);
    rootLay->setSpacing(0);

    // ─── Sidebar ─────────────────────────────────────────────────────────────
    QWidget* sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    QVBoxLayout* sideL = new QVBoxLayout(sidebar);
    sideL->setContentsMargins(0,0,0,0);
    sideL->setSpacing(0);

    QLabel* title = new QLabel("🎮  League\nTracker");
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    sideL->addWidget(title);
    sideL->addSpacing(12);

    for (int i = 0; i < 4; ++i) {
        m_navBtns[i] = new QPushButton(NAV_LABELS[i]);
        m_navBtns[i]->setObjectName("nav");
        m_navBtns[i]->setCheckable(true);
        m_navBtns[i]->setAutoExclusive(true);
        connect(m_navBtns[i], &QPushButton::clicked, this, [this,i]{ onNavClicked(i); });
        sideL->addWidget(m_navBtns[i]);
    }
    m_navBtns[0]->setChecked(true);
    sideL->addStretch();

    // Essence box at bottom of sidebar
    QWidget* essBox = new QWidget;
    essBox->setObjectName("essenceBox");
    QVBoxLayout* essL = new QVBoxLayout(essBox);
    essL->setContentsMargins(12,8,12,8);
    essL->setSpacing(4);

    QLabel* ebLbl = new QLabel("Essence Bleue");
    ebLbl->setObjectName("essLbl");
    m_ebLabel = new QLabel("0");
    m_ebLabel->setObjectName("essVal");
    QPushButton* ebBtn = new QPushButton("Modifier");
    ebBtn->setObjectName("essBtn");
    connect(ebBtn, &QPushButton::clicked, this, &MainWindow::onEbClicked);

    QLabel* eoLbl = new QLabel("Essence Orange");
    eoLbl->setObjectName("essLbl");
    m_eoLabel = new QLabel("0");
    m_eoLabel->setObjectName("essVal");
    QPushButton* eoBtn = new QPushButton("Modifier");
    eoBtn->setObjectName("essBtn");
    connect(eoBtn, &QPushButton::clicked, this, &MainWindow::onEoClicked);

    essL->addWidget(ebLbl);
    essL->addWidget(m_ebLabel);
    essL->addWidget(ebBtn);
    essL->addSpacing(6);
    essL->addWidget(eoLbl);
    essL->addWidget(m_eoLabel);
    essL->addWidget(eoBtn);
    sideL->addWidget(essBox);

    // Export/Import de la sauvegarde (cf. AppController::exportData() /
    // importData()) : utile en cas de réinstallation, ou pour
    // synchroniser tes données entre deux PC.
    QWidget* ioBox = new QWidget;
    ioBox->setObjectName("ioBox");
    QVBoxLayout* ioL = new QVBoxLayout(ioBox);
    ioL->setContentsMargins(0, 0, 0, 0);
    ioL->setSpacing(4);

    QPushButton* exportBtn = new QPushButton("💾  Exporter");
    exportBtn->setObjectName("ioBtn");
    exportBtn->setToolTip("Enregistre une copie de tes données (champions, skins,\nbalises, essences) dans un fichier .json de ton choix.");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportClicked);

    QPushButton* importBtn = new QPushButton("📂  Importer");
    importBtn->setObjectName("ioBtn");
    importBtn->setToolTip("Remplace tes données actuelles par celles d'un fichier\n.json exporté précédemment (depuis ce PC ou un autre).");
    connect(importBtn, &QPushButton::clicked, this, &MainWindow::onImportClicked);

    ioL->addWidget(exportBtn);
    ioL->addWidget(importBtn);
    sideL->addWidget(ioBox);

    QPushButton* quitBtn = new QPushButton("⏻  Quitter");
    quitBtn->setObjectName("quitBtn");
    connect(quitBtn, &QPushButton::clicked, this, &MainWindow::onQuitClicked);
    sideL->addWidget(quitBtn);

    rootLay->addWidget(sidebar);

    // ─── Main content ────────────────────────────────────────────────────────
    QWidget* mainArea = new QWidget;
    QVBoxLayout* mainL = new QVBoxLayout(mainArea);
    mainL->setContentsMargins(0,0,0,0);
    mainL->setSpacing(0);

    m_stack = new QStackedWidget;
    m_gridPage   = new ChampionGridPage(m_controller);
    m_skinPage   = new SkinPage(m_controller);
    m_balisePage = new BalisePage(m_controller);
    m_statsPage  = new StatsWidget(m_controller);
    m_stack->addWidget(m_gridPage);
    m_stack->addWidget(m_skinPage);
    m_stack->addWidget(m_balisePage);
    m_stack->addWidget(m_statsPage);

    mainL->addWidget(m_stack);
    rootLay->addWidget(mainArea, 1);

    // ─── Raccourci Ctrl+F : focus la recherche de la page courante ───────────
    // QKeySequence::Find correspond à Ctrl+F sous Windows/Linux et Cmd+F sur
    // macOS. Les pages sans recherche (Stats) l'ignorent simplement.
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this] {
        switch (m_stack->currentIndex()) {
        case 0: m_gridPage->focusSearch();   break;
        case 1: m_skinPage->focusSearch();   break;
        case 2: m_balisePage->focusSearch(); break;
        default: break; // Stats : pas de recherche
        }
    });
}

void MainWindow::onNavClicked(int index) {
    m_stack->setCurrentIndex(index);
    if (index == 1) m_skinPage->refresh();
    if (index == 2) m_balisePage->refresh();
    if (index == 3) m_statsPage->refresh();
}

void MainWindow::onEbClicked() {
    bool ok;
    int val = QInputDialog::getInt(this, "Essence Bleue",
                                   "Ton nombre d'Essence Bleue :", m_controller->essenceBleu(),
                                   0, 9999999, 1, &ok);
    if (ok) m_controller->setEssenceBleu(val);
}

void MainWindow::onEoClicked() {
    bool ok;
    int val = QInputDialog::getInt(this, "Essence Orange",
                                   "Ton nombre d'Essence Orange :", m_controller->essenceOrange(),
                                   0, 9999999, 1, &ok);
    if (ok) m_controller->setEssenceOrange(val);
}

void MainWindow::onExportClicked() {
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString defaultName = QString("LeagueTracker_sauvegarde_%1.json")
                                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    const QString defaultPath = defaultDir.isEmpty() ? defaultName : defaultDir + "/" + defaultName;

    const QString path = QFileDialog::getSaveFileName(this, "Exporter ta sauvegarde",
                                                      defaultPath, "Fichiers JSON (*.json)");
    if (path.isEmpty()) return; // boîte de dialogue annulée

    if (m_controller->exportData(path)) {
        Toast::show(this, "Sauvegarde exportée avec succès", Toast::Success);
    } else {
        QMessageBox::warning(this, "Export impossible",
                             "Impossible d'écrire le fichier de sauvegarde à cet emplacement.\n"
                             "Vérifie que le dossier choisi est accessible en écriture.");
    }
}

void MainWindow::onImportClicked() {
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getOpenFileName(this, "Importer une sauvegarde",
                                                      defaultDir, "Fichiers JSON (*.json)");
    if (path.isEmpty()) return; // boîte de dialogue annulée

    auto reply = QMessageBox::warning(this, "Importer une sauvegarde",
                                      "Importer ce fichier remplacera TOUTES tes données actuelles "
                                      "(champions, skins, balises, essences, historique) par celles "
                                      "du fichier choisi.\n\nCette action est irréversible. Continuer ?",
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    if (m_controller->importData(path)) {
        Toast::show(this, "Sauvegarde importée avec succès", Toast::Success);
    } else {
        QMessageBox::warning(this, "Import impossible",
                             "Ce fichier ne semble pas être une sauvegarde LeagueTracker valide "
                             "(JSON invalide ou structure inattendue).\n\n"
                             "Tes données actuelles n'ont pas été modifiées.");
    }
}

void MainWindow::onQuitClicked() {
    auto reply = QMessageBox::question(this, "Quitter",
                                       "Voulez-vous vraiment quitter League Tracker ?",
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qApp->quit();
    }
}

void MainWindow::refreshEssence() {
    m_ebLabel->setText(QString::number(m_controller->essenceBleu()) + " EB");
    m_eoLabel->setText(QString::number(m_controller->essenceOrange()) + " EO");
}