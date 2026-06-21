#include "championcard.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QSet>

ChampionCard::ChampionCard(const Champion& c, QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(W, H);
    setCursor(Qt::PointingHandCursor);
    updateData(c);
}

void ChampionCard::updateData(const Champion& c) {
    m_nom          = c.nom;
    m_possede      = c.possede;
    m_prixEffectif = c.prixEffectif();
    m_pixmap       = loadImage(c.nom);

    // Tooltip au survol : statut + prix, sans avoir à ouvrir la fiche.
    QString tip = "<b>" + m_nom.toHtmlEscaped() + "</b><br>";
    if (m_possede) {
        tip += "✔ Possédé";
    } else if (c.prixReduit > 0) {
        tip += QString("🔒 Non possédé<br>%1 EB&nbsp;&nbsp;<s>%2 EB</s>")
                   .arg(c.prixReduit).arg(c.prixStandard);
    } else {
        tip += QString("🔒 Non possédé<br>%1 EB").arg(c.prixStandard);
    }
    setToolTip(tip);

    update();
}

// Liste (en minuscules) des fichiers présents dans le dossier images/,
// lue une seule fois sur le disque puis réutilisée par toutes les cartes.
// Évite de refaire un QFile::exists() par champion et par extension
// (jusqu'à 9 accès disque chacun), ce qui ralentissait nettement le
// démarrage — surtout dans un dossier synchronisé OneDrive.
static const QSet<QString>& availableImageFiles() {
    static const QSet<QString> files = [] {
        QSet<QString> set;
        QDir dir(QCoreApplication::applicationDirPath() + "/images");
        for (const QString& f : dir.entryList(QDir::Files))
            set.insert(f.toLower());
        return set;
    }();
    return files;
}

QPixmap ChampionCard::loadImage(const QString& nom) {
    const QString base = QCoreApplication::applicationDirPath() + "/images/";

    // Variantes de nom de fichier possibles
    QString clean = nom;
    clean.replace(" ","").replace("'","").replace(".","").replace("&","");
    QString under = nom.toLower();
    under.replace(" ","_").replace("'","").replace(".","").replace("&","and");

    const auto& available = availableImageFiles();

    for (const QString& ext : {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("webp")}) {
        for (const QString& candidate : {nom, clean, under}) {
            const QString filename = candidate + "." + ext;
            if (available.contains(filename.toLower()))
                return QPixmap(base + filename).scaled(W, W, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }
    }
    return {};
}

void ChampionCard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QColor BG      ("#1E2328");
    const QColor GOLD    ("#C89B3C");
    const QColor NAMEBG  ("#010A13");

    // Fond carte
    p.fillRect(rect(), BG);

    // Image ou placeholder
    QRect imgRect(0, 0, W, W);
    if (!m_pixmap.isNull()) {
        p.drawPixmap(imgRect, m_pixmap);
    } else {
        // Placeholder avec initiales
        p.fillRect(imgRect, QColor(0x0F, 0x19, 0x23));
        p.setPen(GOLD);
        QFont f("Arial", 26, QFont::Bold);
        p.setFont(f);
        QString initials = m_nom.left(1).toUpper();
        if (m_nom.contains(" ")) {
            QStringList parts = m_nom.split(" ");
            initials = parts[0].left(1).toUpper() + parts.last().left(1).toUpper();
        }
        p.drawText(imgRect, Qt::AlignCenter, initials);
    }

    // Overlay si non possédé
    if (!m_possede) {
        p.fillRect(imgRect, QColor(0, 0, 0, 160));
        // Icône cadenas
        p.setPen(QColor(0x88, 0x88, 0x88));
        QFont lf("Arial", 22);
        p.setFont(lf);
        p.drawText(QRect(0, 15, W, W - 30), Qt::AlignCenter, "🔒");
        // Prix
        p.setPen(QColor(0xAA, 0xAA, 0xAA));
        QFont pf("Arial", 9, QFont::Bold);
        p.setFont(pf);
        p.drawText(QRect(0, W - 22, W, 22), Qt::AlignCenter,
                   QString::number(m_prixEffectif) + " EB");
    } else {
        // Barre dorée en haut = possédé
        p.fillRect(0, 0, W, 3, GOLD);
    }

    // Zone nom
    p.fillRect(0, W, W, H - W, NAMEBG);
    p.setPen(m_possede ? GOLD : QColor(0x77, 0x77, 0x77));
    QFont nf("Arial", 8, m_possede ? QFont::Bold : QFont::Normal);
    p.setFont(nf);
    p.drawText(QRect(2, W + 2, W - 4, H - W - 4), Qt::AlignCenter | Qt::TextWordWrap, m_nom);

    // Hover glow
    if (m_hovered) {
        p.fillRect(rect(), QColor(200, 155, 60, 25));
        p.setPen(QPen(GOLD, 2));
        p.drawRect(1, 1, width() - 2, height() - 2);
    }
}

void ChampionCard::mousePressEvent(QMouseEvent*) { emit clicked(m_nom); }
void ChampionCard::enterEvent(QEnterEvent*)       { m_hovered = true;  update(); }
void ChampionCard::leaveEvent(QEvent*)             { m_hovered = false; update(); }