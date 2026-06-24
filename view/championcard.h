#pragma once
#include <QWidget>
#include <QPixmap>
#include <QEnterEvent>
#include "../model/champion.h"

/*
 * ChampionCard — Vue
 * ────────────────────
 * Petit composant d'affichage d'un champion (image + nom + statut).
 * Reçoit ses données via updateData() et se limite au rendu : aucun
 * accès au Modèle ni au Contrôleur.
 */
class ChampionCard : public QWidget
{
    Q_OBJECT
public:
    static constexpr int W = 110;
    static constexpr int H = 150;

    explicit ChampionCard(const Champion& c, QWidget* parent = nullptr);
    void updateData(const Champion& c);
    const QString& nomChampion() const { return m_nom; }

    // Force le rechargement de la liste des fichiers présents dans
    // images/ au prochain appel de loadImage(). À appeler après un
    // téléchargement d'images (cf. ImageDownloadDialog), sinon la
    // liste mise en cache au démarrage masquerait les nouveaux fichiers.
    static void invalidateImageCache();

    // Variante "nettoyée" d'un nom de champion (espaces, apostrophes,
    // points et & retirés) utilisée comme nom de fichier candidat par
    // loadImage(). Exposée pour qu'ImageDownloadDialog enregistre les
    // images téléchargées sous un nom que loadImage() saura retrouver.
    static QString cleanFileBase(const QString& nom);

signals:
    void clicked(const QString& nom);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    QPixmap loadImage(const QString& nom);

    QString m_nom;
    bool    m_possede = false;
    bool    m_prioritaire = false;
    int     m_prixEffectif = 0;
    QPixmap m_pixmap;
    bool    m_hovered = false;
};