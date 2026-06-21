#include "addskindialog.h"
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
QLineEdit, QSpinBox, QComboBox {
    background: #1E2328;
    border: 1px solid #3A3A3A;
    color: #C8AA6E;
    padding: 5px;
    border-radius: 4px;
    font-size: 13px;
    min-width: 160px;
}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #C89B3C; }
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView {
    background: #1E2328;
    color: #C8AA6E;
    selection-background-color: #C89B3C;
    selection-color: #000;
}
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

AddSkinDialog::AddSkinDialog(const QStringList& championNames, QWidget* parent,
                              const QString& initialChampion)
    : QDialog(parent)
{
    setWindowTitle("Nouveau skin");
    setModal(true);
    setStyleSheet(DLG_STYLE);
    setMinimumWidth(380);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(24, 20, 24, 20);
    main->setSpacing(14);

    QLabel* title = new QLabel("✚  Nouveau skin");
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
    m_nameEdit->setPlaceholderText("Ex : Aurora Etoile Filante");
    form->addRow("Nom du skin :", m_nameEdit);

    m_champCombo = new QComboBox;
    m_champCombo->setEditable(true);
    m_champCombo->addItems(championNames);
    if (!initialChampion.isEmpty())
        m_champCombo->setCurrentText(initialChampion);
    form->addRow("Champion :", m_champCombo);

    m_rareteCombo = new QComboBox;
    m_rareteCombo->addItems({"Basique", "Epique", "Legendaire", "Fantastique", "Ultime", "Exalte", "Transcendant", "Gratuit"});
    form->addRow("Rareté :", m_rareteCombo);

    m_gratuitCb = new QCheckBox("Skin gratuit");
    form->addRow("", m_gratuitCb);

    m_possedeCb = new QCheckBox("Je possède déjà ce skin");
    form->addRow("", m_possedeCb);

    m_prix = new QSpinBox;
    m_prix->setRange(0, 99999);
    m_prix->setSingleStep(10);
    m_prix->setSuffix("  EO");
    m_prix->setValue(1050); // Prix typique d'un skin épique
    form->addRow("Prix :", m_prix);

    main->addLayout(form);

    connect(m_gratuitCb, &QCheckBox::toggled, this, &AddSkinDialog::onGratuitToggled);

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

void AddSkinDialog::onGratuitToggled(bool checked) {
    m_prix->setValue(0);
    m_prix->setEnabled(!checked);
    if (checked) {
        m_rareteCombo->setCurrentText("Gratuit");
        m_possedeCb->setChecked(true);
    }
}

Skin AddSkinDialog::getSkin() const {
    Skin s;
    s.nom      = m_nameEdit->text().trimmed();
    s.champion = m_champCombo->currentText().trimmed();
    s.gratuit  = m_gratuitCb->isChecked();
    s.prix     = s.gratuit ? 0 : m_prix->value();
    s.rarete   = m_rareteCombo->currentText();
    s.possede  = m_possedeCb->isChecked();
    return s;
}
