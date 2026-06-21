#include <QApplication>
#include <QCoreApplication>
#include <QPalette>
#include <QColor>
#include <QIcon>
#include "controller/appcontroller.h"
#include "view/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("LeagueTracker");
    QCoreApplication::setApplicationName("LeagueTracker");

    // ── Icône de l'application ───────────────────────────────────────────
    // S'applique à la fenêtre ET à la barre des tâches Windows
    app.setWindowIcon(QIcon(":/icone/logo_lt.png"));

    // ── Fix du flash blanc au démarrage ─────────────────────────────────
    app.setStyle("Fusion");

    QPalette darkPalette;
    const QColor bg       (0x01, 0x0A, 0x13);
    const QColor surface  (0x0F, 0x19, 0x23);
    const QColor gold     (0xC8, 0xAA, 0x6E);
    const QColor altBg    (0x14, 0x19, 0x20);
    const QColor highlight(0xC8, 0x9B, 0x3C);

    darkPalette.setColor(QPalette::Window,          bg);
    darkPalette.setColor(QPalette::WindowText,      gold);
    darkPalette.setColor(QPalette::Base,            surface);
    darkPalette.setColor(QPalette::AlternateBase,   altBg);
    darkPalette.setColor(QPalette::Text,            gold);
    darkPalette.setColor(QPalette::BrightText,      Qt::white);
    darkPalette.setColor(QPalette::Button,          bg);
    darkPalette.setColor(QPalette::ButtonText,      gold);
    darkPalette.setColor(QPalette::Highlight,       highlight);
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::ToolTipBase,     surface);
    darkPalette.setColor(QPalette::ToolTipText,     gold);
    darkPalette.setColor(QPalette::PlaceholderText, QColor(0x55, 0x55, 0x55));
    app.setPalette(darkPalette);
    // ────────────────────────────────────────────────────────────────────

    AppController controller;

    MainWindow w(&controller);
    w.show();
    return app.exec();
}