#include "imagedownloaddialog.h"
#include "championcard.h"
#include "../controller/appcontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QUrl>

static const char* DLG_STYLE = R"(
QDialog { background: #0F1923; }
QLabel  { color: #C8AA6E; font-size: 13px; }
QLabel#title { color: #C89B3C; font-size: 18px; font-weight: bold; padding: 4px 0; }
QProgressBar {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    border-radius: 4px;
    color: #C8AA6E;
    text-align: center;
    height: 18px;
}
QProgressBar::chunk { background: #C89B3C; border-radius: 3px; }
QPushButton#actionBtn {
    background: transparent;
    border: 1px solid #C89B3C;
    color: #C89B3C;
    border-radius: 4px;
    padding: 6px 18px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton#actionBtn:hover  { background: #C89B3C; color: #000; }
QPushButton#actionBtn:disabled { border-color: #555; color: #555; }
)";

// Retire les accents pour fiabiliser le rapprochement entre le nom
// (français, parfois orthographié légèrement différemment) utilisé par
// l'app et le champ "name" renvoyé par Data Dragon en fr_FR.
static QString normalizeKey(const QString& s) {
    QString norm = s.normalized(QString::NormalizationForm_KD);
    QString out;
    out.reserve(norm.size());
    for (const QChar& c : norm)
        if (c.category() != QChar::Mark_NonSpacing) out += c;
    return out.toLower();
}

// Quelques champions dont le nom officiel Data Dragon (fr_FR) diffère du
// nom couramment utilisé — ex. Data Dragon renvoie "Nunu et Willump"
// alors que le nom usuel (et celui probablement saisi dans l'app) est
// "Nunu & Willump". Ajoutés en alias pour ne pas dépendre d'une
// normalisation générique de la ponctuation, qui risquerait de créer
// de fausses correspondances sur d'autres champions.
static const QVector<QPair<QString, QString>> kManualAliases = {
    { "Nunu & Willump", "Nunu" },
    };

ImageDownloadDialog::ImageDownloadDialog(AppController* controller, QWidget* parent)
    : QDialog(parent), m_controller(controller)
{
    setWindowTitle("Téléchargement des images manquantes");
    setModal(true);
    setMinimumWidth(420);
    setStyleSheet(DLG_STYLE);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(24, 20, 24, 20);
    main->setSpacing(14);

    QLabel* title = new QLabel("⬇  Images Data Dragon");
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    main->addWidget(title);

    m_statusLbl = new QLabel("Préparation…");
    m_statusLbl->setWordWrap(true);
    m_statusLbl->setMinimumHeight(36);
    main->addWidget(m_statusLbl);

    m_progress = new QProgressBar;
    m_progress->setRange(0, 0); // mode indéterminé tant que le total n'est pas connu
    m_progress->setTextVisible(false);
    main->addWidget(m_progress);

    QHBoxLayout* btns = new QHBoxLayout;
    btns->addStretch();
    m_actionBtn = new QPushButton("Annuler");
    m_actionBtn->setObjectName("actionBtn");
    connect(m_actionBtn, &QPushButton::clicked, this, &ImageDownloadDialog::onActionClicked);
    btns->addWidget(m_actionBtn);
    main->addLayout(btns);

    m_nam = new QNetworkAccessManager(this);

    // Différé pour laisser la boîte de dialogue s'afficher avant de
    // démarrer les requêtes réseau.
    QTimer::singleShot(0, this, &ImageDownloadDialog::start);
}

void ImageDownloadDialog::onActionClicked() {
    if (m_finished) {
        accept();
        return;
    }
    m_cancelled = true;
    m_actionBtn->setEnabled(false);
    m_statusLbl->setText("Annulation en cours…");
    if (m_currentReply) m_currentReply->abort();
}

void ImageDownloadDialog::start() {
    fetchVersion();
}

void ImageDownloadDialog::fetchVersion() {
    m_statusLbl->setText("Récupération de la dernière version de Data Dragon…");
    QNetworkRequest req(QUrl("https://ddragon.leagueoflegends.com/api/versions.json"));
    m_currentReply = m_nam->get(req);
    connect(m_currentReply, &QNetworkReply::finished, this, [this] {
        QNetworkReply* reply = m_currentReply;
        m_currentReply = nullptr;
        onVersionReply(reply);
    });
}

void ImageDownloadDialog::onVersionReply(QNetworkReply* reply) {
    reply->deleteLater();
    if (m_cancelled) { finishUp("Téléchargement annulé."); return; }
    if (reply->error() != QNetworkReply::NoError) {
        fail("Impossible de joindre Data Dragon. Vérifie ta connexion internet.");
        return;
    }

    QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
    if (arr.isEmpty()) { fail("Réponse Data Dragon invalide (liste des versions)."); return; }

    m_version = arr.first().toString();
    fetchChampionData();
}

void ImageDownloadDialog::fetchChampionData() {
    m_statusLbl->setText(QString("Récupération de la liste des champions (v%1)…").arg(m_version));
    QUrl url(QString("https://ddragon.leagueoflegends.com/cdn/%1/data/fr_FR/champion.json").arg(m_version));
    m_currentReply = m_nam->get(QNetworkRequest(url));
    connect(m_currentReply, &QNetworkReply::finished, this, [this] {
        QNetworkReply* reply = m_currentReply;
        m_currentReply = nullptr;
        onChampionDataReply(reply);
    });
}

void ImageDownloadDialog::onChampionDataReply(QNetworkReply* reply) {
    reply->deleteLater();
    if (m_cancelled) { finishUp("Téléchargement annulé."); return; }
    if (reply->error() != QNetworkReply::NoError) {
        fail("Impossible de récupérer la liste des champions Data Dragon.");
        return;
    }

    QJsonObject data = QJsonDocument::fromJson(reply->readAll()).object().value("data").toObject();
    if (data.isEmpty()) { fail("Réponse Data Dragon invalide (liste des champions)."); return; }

    m_nameToId.clear();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        QJsonObject obj = it.value().toObject();
        const QString id   = obj.value("id").toString();
        const QString name = obj.value("name").toString();
        if (id.isEmpty() || name.isEmpty()) continue;
        m_nameToId.insert(normalizeKey(name), id);
    }
    for (const auto& alias : kManualAliases)
        m_nameToId.insert(normalizeKey(alias.first), alias.second);

    computeMissingAndStart();
}

void ImageDownloadDialog::computeMissingAndStart() {
    // Fichiers déjà présents dans images/ (mêmes variantes que ChampionCard::loadImage).
    QDir imgDir(QCoreApplication::applicationDirPath() + "/images");
    QSet<QString> existing;
    for (const QString& f : imgDir.entryList(QDir::Files))
        existing.insert(f.toLower());

    auto hasLocalImage = [&](const QString& nom) {
        const QString clean = ChampionCard::cleanFileBase(nom);
        for (const QString& ext : {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("webp")}) {
            if (existing.contains((nom + "." + ext).toLower()))   return true;
            if (existing.contains((clean + "." + ext).toLower())) return true;
        }
        return false;
    };

    m_toDownload.clear();
    m_unmatchedCount = 0;
    for (const auto& c : m_controller->champions()) {
        if (hasLocalImage(c.nom)) continue;
        const QString id = m_nameToId.value(normalizeKey(c.nom));
        if (id.isEmpty()) { ++m_unmatchedCount; continue; }
        m_toDownload.append({c.nom, id});
    }

    if (m_toDownload.isEmpty()) {
        QString msg = "Toutes les images reconnues sont déjà présentes !";
        if (m_unmatchedCount > 0)
            msg += QString(" (%1 champion(s) non reconnu(s) par Data Dragon)").arg(m_unmatchedCount);
        finishUp(msg);
        return;
    }

    imgDir.mkpath(".");
    m_progress->setTextVisible(true);
    m_progress->setRange(0, m_toDownload.size());
    m_progress->setValue(0);
    m_idx = 0;
    downloadNext();
}

void ImageDownloadDialog::downloadNext() {
    if (m_cancelled) {
        finishUp(QString("Téléchargement annulé — %1 image(s) récupérée(s).").arg(m_okCount));
        return;
    }
    if (m_idx >= m_toDownload.size()) {
        QStringList parts;
        parts << QString("%1 image(s) téléchargée(s)").arg(m_okCount);
        if (m_failCount > 0)      parts << QString("%1 introuvable(s)").arg(m_failCount);
        if (m_unmatchedCount > 0) parts << QString("%1 non reconnu(s)").arg(m_unmatchedCount);
        finishUp(parts.join(" • ") + ".");
        return;
    }

    const auto& entry = m_toDownload[m_idx];
    m_statusLbl->setText(QString("Téléchargement %1/%2 : %3…")
                             .arg(m_idx + 1).arg(m_toDownload.size()).arg(entry.first));

    QUrl url(QString("https://ddragon.leagueoflegends.com/cdn/%1/img/champion/%2.png")
                 .arg(m_version, entry.second));
    m_currentReply = m_nam->get(QNetworkRequest(url));
    connect(m_currentReply, &QNetworkReply::finished, this, [this] {
        QNetworkReply* reply = m_currentReply;
        m_currentReply = nullptr;
        onImageReply(reply);
    });
}

void ImageDownloadDialog::onImageReply(QNetworkReply* reply) {
    reply->deleteLater();
    if (m_cancelled) {
        finishUp(QString("Téléchargement annulé — %1 image(s) récupérée(s).").arg(m_okCount));
        return;
    }

    const auto& entry = m_toDownload[m_idx];
    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray bytes = reply->readAll();
        const QString path = QCoreApplication::applicationDirPath() + "/images/"
                             + ChampionCard::cleanFileBase(entry.first) + ".png";
        QFile f(path);
        if (!bytes.isEmpty() && f.open(QIODevice::WriteOnly) && f.write(bytes) > 0) {
            ++m_okCount;
        } else {
            ++m_failCount;
        }
    } else {
        ++m_failCount;
    }

    ++m_idx;
    m_progress->setValue(m_idx);
    downloadNext();
}

void ImageDownloadDialog::finishUp(const QString& statusMsg) {
    m_finished = true;
    m_statusLbl->setText(statusMsg);
    m_progress->setRange(0, 1);
    m_progress->setValue(1);
    m_progress->setTextVisible(false);
    m_actionBtn->setText("Fermer");
    m_actionBtn->setEnabled(true);
}

void ImageDownloadDialog::fail(const QString& reason) {
    finishUp("✘  " + reason);
}