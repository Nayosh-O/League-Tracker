#include "addchampiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

static const char* DLG_STYLE = R"(
QDialog { background: #0F1923; }
QLabel  { color: #C8AA6E; font-size: 13px; }
QLabel#title { color: #C89B3C; font-size: 20px; font-weight: bold; padding: 8px 0; }
QLabel#effLabel { color: #FFFFFF; font-size: 14px; font-weight: bold; }
QCheckBox { color: #C8AA6E; font-size: 13px; spacing: 8px; }
QCheckBox::indicator {
    width: 18px; height: 18px;
    border: 2px solid #C89B3C;
    border-radius: 3px;
    background: #1E2328;
}
QCheckBox::indicator:checked { background: #C89B3C; }
QLineEdit, QSpinBox {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    color: #C8AA6E;
    padding: 5px;
    border-radius: 4px;
    font-size: 13px;
    min-width: 100px;
}
QLineEdit:focus, QSpinBox:focus { border-color: #C89B3C; }
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
QFrame#sep { background: #1E2328; }
)";

AddChampionDialog::AddChampionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Nouveau champion");
    setModal(true);
    setStyleSheet(DLG_STYLE);
    setMinimumWidth(380);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(24, 20, 24, 20);
    main->setSpacing(14);

    QLabel* title = new QLabel("✚  Nouveau champion");
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    main->addWidget(title);

    QFrame* sep = new QFrame; sep->setObjectName("sep");
    sep->setFixedHeight(1); sep->setFrameShape(QFrame::HLine);
    main->addWidget(sep);

    QFormLayout* form = new QFormLayout;
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignRight);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("Ex : Mel, Ambessa, Aurora...");
    form->addRow("Nom :", m_nameEdit);

    m_ownedCb = new QCheckBox("Possédé");
    form->addRow("Statut :", m_ownedCb);

    m_prixStd = new QSpinBox;
    m_prixStd->setRange(0, 99999);
    m_prixStd->setSingleStep(225);
    m_prixStd->setSuffix("  EB");
    m_prixStd->setValue(2400); // Prix typique d'un nouveau champion
    form->addRow("Prix standard :", m_prixStd);

    m_prixReduit = new QSpinBox;
    m_prixReduit->setRange(0, 99999);
    m_prixReduit->setSingleStep(225);
    m_prixReduit->setSuffix("  EB  (0 = aucune)");
    m_prixReduit->setValue(0);
    form->addRow("Prix réduit :", m_prixReduit);

    m_prixEffLbl = new QLabel;
    m_prixEffLbl->setObjectName("effLabel");
    form->addRow("Prix effectif :", m_prixEffLbl);
    main->addLayout(form);
    updatePrixEff();

    connect(m_prixStd,    QOverload<int>::of(&QSpinBox::valueChanged), this, &AddChampionDialog::updatePrixEff);
    connect(m_prixReduit, QOverload<int>::of(&QSpinBox::valueChanged), this, &AddChampionDialog::updatePrixEff);

    QLabel* rolesLbl = new QLabel("Rôle(s) :");
    main->addWidget(rolesLbl);
    QHBoxLayout* rolesL = new QHBoxLayout;
    for (const QString& role : allRoleNames()) {
        QCheckBox* cb = new QCheckBox(role);
        m_roleCbs << cb;
        rolesL->addWidget(cb);
    }
    main->addLayout(rolesL);

    QHBoxLayout* btns = new QHBoxLayout;
    QPushButton* ok  = new QPushButton("✚  Ajouter");
    ok->setObjectName("okBtn");
    QPushButton* cancel = new QPushButton("Annuler");
    cancel->setObjectName("cancelBtn");
    connect(ok,     &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    btns->addStretch();
    btns->addWidget(cancel);
    btns->addWidget(ok);
    main->addLayout(btns);

    m_nameEdit->setFocus();
}

void AddChampionDialog::updatePrixEff() {
    int eff = (m_prixReduit->value() > 0) ? m_prixReduit->value() : m_prixStd->value();
    m_prixEffLbl->setText(QString::number(eff) + "  EB");
}

Champion AddChampionDialog::getChampion() const {
    Champion c;
    c.nom          = m_nameEdit->text().trimmed();
    c.possede      = m_ownedCb->isChecked();
    c.prixStandard = m_prixStd->value();
    c.prixReduit   = m_prixReduit->value();
    for (QCheckBox* cb : m_roleCbs)
        if (cb->isChecked()) c.roles << cb->text();
    return c;
}
