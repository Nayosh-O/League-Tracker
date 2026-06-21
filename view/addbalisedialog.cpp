#include "addbalisedialog.h"
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
    min-width: 160px;
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

AddBaliseDialog::AddBaliseDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Nouvelle balise");
    setModal(true);
    setStyleSheet(DLG_STYLE);
    setMinimumWidth(380);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(24, 20, 24, 20);
    main->setSpacing(14);

    QLabel* title = new QLabel("✚  Nouvelle balise");
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
    m_nameEdit->setPlaceholderText("Ex : Balise Saison 3 2026");
    form->addRow("Nom :", m_nameEdit);

    m_possedeCb = new QCheckBox("Déjà possédée");
    form->addRow("Statut :", m_possedeCb);

    m_gratuiteCb = new QCheckBox("Gratuite");
    form->addRow("", m_gratuiteCb);

    m_prix = new QSpinBox;
    m_prix->setRange(0, 99999);
    m_prix->setSingleStep(10);
    m_prix->setSuffix("  EO");
    m_prix->setValue(340); // Prix typique d'une balise payante
    form->addRow("Prix :", m_prix);

    main->addLayout(form);

    connect(m_gratuiteCb, &QCheckBox::toggled, this, &AddBaliseDialog::onGratuiteToggled);

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

void AddBaliseDialog::onGratuiteToggled(bool checked) {
    m_prix->setValue(0);
    m_prix->setEnabled(!checked);
}

Balise AddBaliseDialog::getBalise() const {
    Balise b;
    b.nom     = m_nameEdit->text().trimmed();
    b.possede = m_possedeCb->isChecked();
    b.prix    = m_gratuiteCb->isChecked() ? 0 : m_prix->value();
    return b;
}
