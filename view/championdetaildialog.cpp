#include "championdetaildialog.h"
#include "../controller/appcontroller.h"
#include "addskindialog.h"
#include "toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>
#include <QScrollArea>

static const char* DLG_STYLE = R"(
QDialog { background: #0F1923; }
QLabel  { color: #C8AA6E; font-size: 13px; }
QLabel#title { color: #C89B3C; font-size: 20px; font-weight: bold; padding: 8px 0; }
QLabel#effLabel { color: #FFFFFF; font-size: 14px; font-weight: bold; }
QLabel#sectionTitle { color: #C89B3C; font-size: 13px; font-weight: bold; padding-top: 4px; }
QLabel#noSkinsLbl { color: #7A7A7A; font-size: 12px; font-style: italic; }
QCheckBox { color: #C8AA6E; font-size: 13px; spacing: 8px; }
QCheckBox::indicator {
    width: 18px; height: 18px;
    border: 2px solid #C89B3C;
    border-radius: 3px;
    background: #1E2328;
}
QCheckBox::indicator:checked { background: #C89B3C; }
QSpinBox {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    color: #C8AA6E;
    padding: 5px;
    border-radius: 4px;
    font-size: 13px;
    min-width: 100px;
}
QSpinBox:focus { border-color: #C89B3C; }
QScrollArea#skinsScroll { background: #1E2328; border: 1px solid #3A3A3A; border-radius: 4px; }
QWidget#skinsContainer { background: #1E2328; }
QPushButton#okBtn {
    background: #C89B3C; color: #000;
    border: none; border-radius: 4px;
    padding: 8px 24px; font-weight: bold; font-size: 13px;
}
QPushButton#okBtn:hover  { background: #E0B050; }
QPushButton#cancelBtn {
    background: #1E2328; color: #888;
    border: 1px solid #3A3A3A; border-radius: 4px;
    padding: 8px 24px; font-size: 13px;
}
QPushButton#cancelBtn:hover { color: #C8AA6E; }
QPushButton#deleteBtn {
    background: transparent;
    border: 1px solid #E74C3C;
    color: #E74C3C;
    border-radius: 4px;
    padding: 8px 16px; font-size: 13px;
}
QPushButton#deleteBtn:hover { background: #E74C3C; color: #000; }
QPushButton#addSkinBtn {
    background: transparent;
    border: 1px solid #C89B3C;
    color: #C89B3C;
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 12px;
}
QPushButton#addSkinBtn:hover { background: #C89B3C; color: #000; }
QFrame#sep { background: #1E2328; }
)";

ChampionDetailDialog::ChampionDetailDialog(AppController* controller, const Champion& c, QWidget* parent)
    : QDialog(parent), m_controller(controller), m_champ(c)
{
    setWindowTitle("Détails — " + c.nom);
    setModal(true);
    setStyleSheet(DLG_STYLE);
    setMinimumWidth(380);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(24, 20, 24, 20);
    main->setSpacing(14);

    QLabel* title = new QLabel(c.nom);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    main->addWidget(title);

    QFrame* sep = new QFrame; sep->setObjectName("sep");
    sep->setFixedHeight(1); sep->setFrameShape(QFrame::HLine);
    main->addWidget(sep);

    // Formulaire
    QFormLayout* form = new QFormLayout;
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignRight);

    m_ownedCb = new QCheckBox("Possédé");
    m_ownedCb->setChecked(c.possede);
    form->addRow("Statut :", m_ownedCb);

    m_prioriteCb = new QCheckBox("★ À acheter en priorité");
    m_prioriteCb->setChecked(c.prioritaire);
    m_prioriteCb->setToolTip("Marque ce champion pour le retrouver en premier\ndans la liste « Champions à acheter » de l'onglet Stats.");
    form->addRow("Priorité :", m_prioriteCb);

    m_prixStd = new QSpinBox;
    m_prixStd->setRange(0, 99999);
    m_prixStd->setSingleStep(225);
    m_prixStd->setSuffix("  EB");
    m_prixStd->setValue(c.prixStandard);
    form->addRow("Prix standard :", m_prixStd);

    m_prixReduit = new QSpinBox;
    m_prixReduit->setRange(0, 99999);
    m_prixReduit->setSingleStep(225);
    m_prixReduit->setSuffix("  EB  (0 = aucune)");
    m_prixReduit->setValue(c.prixReduit);
    form->addRow("Prix réduit :", m_prixReduit);

    m_prixEffLbl = new QLabel;
    m_prixEffLbl->setObjectName("effLabel");
    form->addRow("Prix effectif :", m_prixEffLbl);
    main->addLayout(form);
    updatePrixEff();

    connect(m_prixStd,    QOverload<int>::of(&QSpinBox::valueChanged), this, &ChampionDetailDialog::updatePrixEff);
    connect(m_prixReduit, QOverload<int>::of(&QSpinBox::valueChanged), this, &ChampionDetailDialog::updatePrixEff);

    QLabel* rolesLbl = new QLabel("Rôle(s) :");
    main->addWidget(rolesLbl);
    QHBoxLayout* rolesL = new QHBoxLayout;
    for (const QString& role : allRoleNames()) {
        QCheckBox* cb = new QCheckBox(role);
        cb->setChecked(c.hasRole(role));
        m_roleCbs << cb;
        rolesL->addWidget(cb);
    }
    main->addLayout(rolesL);

    QFrame* sep2 = new QFrame; sep2->setObjectName("sep");
    sep2->setFixedHeight(1); sep2->setFrameShape(QFrame::HLine);
    main->addWidget(sep2);

    buildSkinsSection(main);

    // Boutons
    QHBoxLayout* btns = new QHBoxLayout;
    QPushButton* del = new QPushButton("🗑  Supprimer");
    del->setObjectName("deleteBtn");
    QPushButton* ok  = new QPushButton("✔  Enregistrer");
    ok->setObjectName("okBtn");
    QPushButton* cancel = new QPushButton("Annuler");
    cancel->setObjectName("cancelBtn");
    connect(del,    &QPushButton::clicked, this, &ChampionDetailDialog::onDeleteClicked);
    connect(ok,     &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    btns->addWidget(del);
    btns->addStretch();
    btns->addWidget(cancel);
    btns->addWidget(ok);
    main->addLayout(btns);
}

void ChampionDetailDialog::buildSkinsSection(QVBoxLayout* main) {
    QHBoxLayout* headerL = new QHBoxLayout;
    QLabel* skinsTitle = new QLabel(QString("Skins de %1").arg(m_champ.nom));
    skinsTitle->setObjectName("sectionTitle");
    QPushButton* addSkinBtn = new QPushButton("✚  Ajouter un skin");
    addSkinBtn->setObjectName("addSkinBtn");
    connect(addSkinBtn, &QPushButton::clicked, this, &ChampionDetailDialog::onAddSkinClicked);
    headerL->addWidget(skinsTitle);
    headerL->addStretch();
    headerL->addWidget(addSkinBtn);
    main->addLayout(headerL);

    QScrollArea* scroll = new QScrollArea;
    scroll->setObjectName("skinsScroll");
    scroll->setWidgetResizable(true);
    scroll->setMaximumHeight(160);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget;
    container->setObjectName("skinsContainer");
    m_skinsLayout = new QVBoxLayout(container);
    m_skinsLayout->setContentsMargins(10, 8, 10, 8);
    m_skinsLayout->setSpacing(6);

    scroll->setWidget(container);
    main->addWidget(scroll);

    refreshSkinsSection();
}

void ChampionDetailDialog::refreshSkinsSection() {
    // Vide le contenu actuel de la liste
    QLayoutItem* item;
    while ((item = m_skinsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const auto& allSkins = m_controller->skins();
    const QVector<int> indices = m_controller->skinIndicesForChampion(m_champ.nom);

    if (indices.isEmpty()) {
        // Créé ici (et non comme membre persistant) : ainsi il est toujours
        // immédiatement ajouté au layout donc toujours rattaché à un parent,
        // et détruit proprement au prochain refresh (cf. boucle de
        // nettoyage ci-dessus) — l'ancienne version créait ce label une
        // seule fois dans buildSkinsSection() sans l'ajouter au layout tant
        // que la liste de skins n'était pas vide, ce qui le laissait orphelin
        // (sans parent) et donc jamais détruit pour tout champion ayant déjà
        // au moins un skin connu à l'ouverture du dialogue.
        QLabel* noSkinsLbl = new QLabel("Aucun skin connu pour ce champion.");
        noSkinsLbl->setObjectName("noSkinsLbl");
        m_skinsLayout->addWidget(noSkinsLbl);
        return;
    }

    for (int idx : indices) {
        const Skin& s = allSkins[idx];
        QString label = s.nom;
        if (!s.rarete.isEmpty()) label += QString("  (%1)").arg(s.rarete);

        QCheckBox* cb = new QCheckBox(label);
        cb->setChecked(s.possede);
        connect(cb, &QCheckBox::toggled, this, [this, idx, nom = s.nom](bool checked){
            m_controller->setSkinOwned(idx, checked);
            Toast::show(this,
                        checked ? QString("« %1 » marqué possédé").arg(nom)
                                : QString("« %1 » marqué non possédé").arg(nom),
                        checked ? Toast::Success : Toast::Info);
        });
        m_skinsLayout->addWidget(cb);
    }
    m_skinsLayout->addStretch();
}

void ChampionDetailDialog::onAddSkinClicked() {
    QStringList champNames;
    for (const auto& c : m_controller->champions())
        champNames << c.nom;
    champNames.sort(Qt::CaseInsensitive);

    AddSkinDialog dlg(champNames, this, m_champ.nom);
    if (dlg.exec() != QDialog::Accepted) return;

    Skin s = dlg.getSkin();
    if (s.nom.isEmpty()) {
        QMessageBox::warning(this, "Nom manquant", "Donne un nom au skin avant de l'ajouter.");
        return;
    }
    if (!m_controller->addSkin(s)) {
        QMessageBox::warning(this, "Skin existant",
                             QString("« %1 » est déjà dans ta liste de skins.").arg(s.nom));
        return;
    }
    refreshSkinsSection();
}

void ChampionDetailDialog::updatePrixEff() {
    int eff = (m_prixReduit->value() > 0) ? m_prixReduit->value() : m_prixStd->value();
    m_prixEffLbl->setText(QString::number(eff) + "  EB");
}

void ChampionDetailDialog::onDeleteClicked() {
    auto rep = QMessageBox::question(this, "Supprimer le champion",
                                     QString("Supprimer « %1 » de ta collection ?\nCette action est irréversible.").arg(m_champ.nom),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (rep == QMessageBox::Yes) done(Deleted);
}

Champion ChampionDetailDialog::getChampion() const {
    Champion c = m_champ;
    c.possede      = m_ownedCb->isChecked();
    c.prioritaire  = m_prioriteCb->isChecked();
    c.prixStandard = m_prixStd->value();
    c.prixReduit   = m_prixReduit->value();
    c.roles.clear();
    for (QCheckBox* cb : m_roleCbs)
        if (cb->isChecked()) c.roles << cb->text();
    return c;
}