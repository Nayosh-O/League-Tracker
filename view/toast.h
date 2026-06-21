#pragma once
#include <QWidget>
#include <QString>
#include <functional>

class QTimer;

/*
 * Toast — Vue (composant utilitaire)
 * ─────────────────────────────────────
 * Petite notification flottante, non bloquante, qui apparaît en bas à
 * droite de la fenêtre puis disparaît automatiquement (fondu). Remplace
 * les QMessageBox pour les actions rapides et réversibles (cocher une
 * case, supprimer une ligne d'un tableau) : pas d'interruption du flux,
 * et possibilité d'« Annuler » via un bouton optionnel.
 *
 * Un seul toast est affiché à la fois : un nouvel appel remplace
 * immédiatement le précédent plutôt que de les empiler, pour rester
 * discret même en cas de clics rapides successifs.
 *
 * Usage :
 *   Toast::show(this, "Skin supprimé", Toast::Danger,
 *               "Annuler", [this, removed] { m_controller->addSkin(removed); });
 */
class Toast : public QWidget
{
    Q_OBJECT
public:
    enum Type { Info, Success, Danger };

    static void show(QWidget* anchor,
                      const QString& message,
                      Type type = Info,
                      const QString& actionLabel = QString(),
                      std::function<void()> action = nullptr,
                      int durationMs = 4000);

protected:
    void enterEvent(QEnterEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    explicit Toast(QWidget* parent);
    void reposition();
    void startClose();

    QTimer* m_timer = nullptr;

    static Toast* s_current;
};
