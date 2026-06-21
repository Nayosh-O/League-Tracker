#pragma once
#include <QDialog>
#include <QString>
#include <QVector>
#include <QHash>
#include <QPair>

class QLabel;
class QProgressBar;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;
class AppController;

/*
 * ImageDownloadDialog — Vue
 * ───────────────────────────
 * Remplace le script curl manuel du README : récupère la dernière
 * version de Data Dragon, la liste des champions (fr_FR, pour faire
 * correspondre le nom français utilisé par l'app à l'identifiant
 * Data Dragon — ex. "Wukong" → "MonkeyKing", "Kai'Sa" → "Kaisa" —
 * puis télécharge uniquement les portraits des champions qui n'ont
 * pas déjà une image locale dans images/.
 *
 * Téléchargement séquentiel (un champion à la fois) : plus simple à
 * fiabiliser qu'un pool de requêtes parallèles, et largement suffisant
 * vu la fréquence d'usage de cette fonctionnalité.
 */
class ImageDownloadDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImageDownloadDialog(AppController* controller, QWidget* parent = nullptr);

    int downloadedCount() const { return m_okCount; }
    int failedCount()     const { return m_failCount; }

private slots:
    void onActionClicked();

private:
    void start();
    void fetchVersion();
    void onVersionReply(QNetworkReply* reply);
    void fetchChampionData();
    void onChampionDataReply(QNetworkReply* reply);
    void computeMissingAndStart();
    void downloadNext();
    void onImageReply(QNetworkReply* reply);
    void finishUp(const QString& statusMsg);
    void fail(const QString& reason);

    AppController*          m_controller = nullptr;
    QNetworkAccessManager*  m_nam        = nullptr;
    QNetworkReply*          m_currentReply = nullptr;

    QLabel*       m_statusLbl = nullptr;
    QProgressBar* m_progress  = nullptr;
    QPushButton*  m_actionBtn = nullptr;

    QString m_version;
    QHash<QString, QString> m_nameToId; // nom FR (lowercase) -> id Data Dragon

    QVector<QPair<QString, QString>> m_toDownload; // (nom du champion, id Data Dragon)
    int  m_idx            = 0;
    int  m_okCount        = 0;
    int  m_failCount      = 0;
    int  m_unmatchedCount = 0; // champions sans correspondance trouvée côté Data Dragon
    bool m_cancelled      = false;
    bool m_finished       = false;
};
