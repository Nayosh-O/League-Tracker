QT += core gui widgets charts
CONFIG += c++17
CONFIG -= console
CONFIG += windows
TARGET = LeagueTracker
TEMPLATE = app

SOURCES += \
    main.cpp \
    model/datamanager.cpp \
    controller/appcontroller.cpp \
    view/mainwindow.cpp \
    view/championcard.cpp \
    view/championgridpage.cpp \
    view/championdetaildialog.cpp \
    view/addchampiondialog.cpp \
    view/skinpage.cpp \
    view/addskindialog.cpp \
    view/balisepage.cpp \
    view/addbalisedialog.cpp \
    view/statswidget.cpp

HEADERS += \
    model/champion.h \
    model/skin.h \
    model/balise.h \
    model/datamanager.h \
    controller/appcontroller.h \
    view/mainwindow.h \
    view/championcard.h \
    view/championgridpage.h \
    view/championdetaildialog.h \
    view/addchampiondialog.h \
    view/skinpage.h \
    view/addskindialog.h \
    view/balisepage.h \
    view/addbalisedialog.h \
    view/statswidget.h

# ── Icône de l'application ──────────────────────────────────────────────────
# L'icône de la fenêtre est gérée via app.setWindowIcon() dans main.cpp
RESOURCES += resources.qrc