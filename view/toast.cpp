#include "toast.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEnterEvent>

Toast* Toast::s_current = nullptr;

Toast::Toast(QWidget* parent) : QWidget(parent) {
    setObjectName("toast");
    setAttribute(Qt::WA_StyledBackground, true);
}

void Toast::show(QWidget* anchor, const QString& message, Type type,
                 const QString& actionLabel, std::function<void()> action,
                 int durationMs)
{
    if (!anchor) return;
    QWidget* root = anchor->window();
    if (!root) return;

    // Un seul toast actif à la fois : on remplace l'éventuel précédent
    // pour éviter d'empiler les notifications en cas de clics rapides
    // (ex. cocher plusieurs cases coup sur coup).
    if (s_current) {
        s_current->m_timer->stop();
        s_current->deleteLater();
        s_current = nullptr;
    }

    Toast* t = new Toast(root);
    s_current = t;

    QString accent = "#C89B3C";
    QString icon   = "ℹ";
    if (type == Success) { accent = "#2ECC71"; icon = "✔"; }
    if (type == Danger)  { accent = "#E74C3C"; icon = "🗑"; }

    t->setStyleSheet(QString(R"(
        QWidget#toast {
            background: #1E2328;
            border: 1px solid %1;
            border-radius: 6px;
        }
        QLabel#toastMsg  { color: #C8AA6E; font-size: 12px; background: transparent; }
        QLabel#toastIcon { color: %1; font-size: 14px; font-weight: bold; background: transparent; }
        QPushButton#toastAction {
            background: transparent;
            border: none;
            color: %1;
            font-size: 12px;
            font-weight: bold;
            padding: 2px 6px;
        }
        QPushButton#toastAction:hover { text-decoration: underline; }
    )").arg(accent));

    QHBoxLayout* lay = new QHBoxLayout(t);
    lay->setContentsMargins(12, 8, 10, 8);
    lay->setSpacing(8);

    QLabel* iconLbl = new QLabel(icon);
    iconLbl->setObjectName("toastIcon");
    lay->addWidget(iconLbl);

    QLabel* msgLbl = new QLabel(message);
    msgLbl->setObjectName("toastMsg");
    lay->addWidget(msgLbl);

    if (!actionLabel.isEmpty()) {
        QPushButton* actBtn = new QPushButton(actionLabel);
        actBtn->setObjectName("toastAction");
        actBtn->setCursor(Qt::PointingHandCursor);
        QObject::connect(actBtn, &QPushButton::clicked, t, [t, action] {
            if (action) action();
            t->startClose();
        });
        lay->addWidget(actBtn);
    }

    t->adjustSize();
    t->reposition();

    // Repositionne le toast si la fenêtre parente est redimensionnée
    // pendant qu'il est affiché (ex. l'utilisateur étire la fenêtre).
    // L'event filter est automatiquement retiré à la destruction du toast.
    root->installEventFilter(t);

    auto* effect = new QGraphicsOpacityEffect(t);
    t->setGraphicsEffect(effect);
    effect->setOpacity(0.0);
    t->QWidget::show();
    t->raise();

    auto* fadeIn = new QPropertyAnimation(effect, "opacity", t);
    fadeIn->setDuration(150);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    t->m_timer = new QTimer(t);
    t->m_timer->setSingleShot(true);
    QObject::connect(t->m_timer, &QTimer::timeout, t, &Toast::startClose);
    t->m_timer->start(durationMs);
}

void Toast::reposition() {
    QWidget* root = parentWidget();
    if (!root) return;
    int x = root->width()  - width()  - 24;
    int y = root->height() - height() - 24;
    move(qMax(8, x), qMax(8, y));
}

void Toast::startClose() {
    if (m_timer) m_timer->stop();
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect());
    auto* anim = new QPropertyAnimation(effect, "opacity", this);
    anim->setDuration(200);
    anim->setStartValue(effect ? effect->opacity() : 1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, this, [this] {
        if (s_current == this) s_current = nullptr;
        deleteLater();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// Pause-on-hover : laisse le temps de cliquer sur « Annuler » sans que
// le toast ne se ferme sous la souris.
void Toast::enterEvent(QEnterEvent*) {
    if (m_timer) m_timer->stop();
}

void Toast::leaveEvent(QEvent*) {
    if (m_timer) m_timer->start(1500);
}

// Reposition quand la fenêtre parente est redimensionnée ou déplacée,
// pour que le toast reste bien ancré en bas à droite.
bool Toast::eventFilter(QObject* watched, QEvent* event) {
    Q_UNUSED(watched)
    if (event->type() == QEvent::Resize || event->type() == QEvent::Move)
        reposition();
    return false; // on ne consomme pas l'événement
}