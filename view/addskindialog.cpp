#include "addskindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QCompleter>
#include <QStringListModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

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

// Normalise un nom (champion ou skin) pour le matching contre les données
// Community Dragon : retire les accents et les apostrophes (Kai'Sa, Vel'Koz,
// Cho'Gath…), met en minuscule. Les noms de champions ne sont pas traduits
// en français (Annie reste Annie), donc aucune table d'alias n'est requise
// ici — contrairement au cas Data Dragon (cf. ImageDownloadDialog) qui
// convertit un nom vers un identifiant interne différent (ex. "MonkeyKing").
static QString normalize(const QString& s) {
    const QString norm = s.normalized(QString::NormalizationForm_KD);
    QString out;
    out.reserve(norm.size());
    for (const QChar& c : norm) {
        if (c.category() == QChar::Mark_NonSpacing) continue;
        if (c == QChar('\'') || c == QChar(0x2019)) continue; // ' et ’
        out += c;
    }
    return out.trimmed().toLower();
}

// Correspondance entre le champ "rarity" de Community Dragon et les libellés
// du combo m_rareteCombo. Vérifié manuellement sur skins.json : les 7 paliers
// correspondent exactement à ceux déjà utilisés dans l'app.
static QString cdragonRarityToLabel(const QString& raw) {
    static const QHash<QString, QString> kMap = {
                                                  {"kNoRarity",     "Basique"},
                                                  {"kEpic",         "Epique"},
                                                  {"kLegendary",    "Legendaire"},
                                                  {"kMythic",       "Fantastique"},
                                                  {"kUltimate",     "Ultime"},
                                                  {"kExalted",      "Exalte"},
                                                  {"kTranscendent", "Transcendant"},
                                                  };
    return kMap.value(raw, "Basique");
}

// Tarif EO fixe de craft d'un fragment de skin — uniquement connu et stable
// pour ces deux paliers. Les autres (Basique, Fantastique/Mythique, Ultime,
// Exalte, Transcendant) n'ont pas de conversion EO universelle : ce sont des
// prix RP variables ou des paliers achetés via Essence Mythique / le Sanctum,
// pas via la fragmentation classique. Le prix y reste donc en saisie manuelle.
static int eoPriceForRarete(const QString& rarete) {
    if (rarete == "Epique")     return 1050;
    if (rarete == "Legendaire") return 1520;
    return -1;
}

AddSkinDialog::AddSkinDialog(const QStringList& championNames, QWidget* parent,
                             const QString& initialChampion)
    : QDialog(parent), m_championNames(championNames)
{
    setWindowTitle("Nouveau skin");
    setModal(true);
    setStyleSheet(DLG_STYLE);
    setMinimumWidth(380);

    m_nam            = new QNetworkAccessManager(this);
    m_completerModel = new QStringListModel(this);
    m_completer      = new QCompleter(m_completerModel, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);

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
    m_nameEdit->setCompleter(m_completer);
    form->addRow("Nom du skin :", m_nameEdit);

    m_champCombo = new QComboBox;
    m_champCombo->setEditable(true);
    // Connecté avant addItems()/setCurrentText() pour capter la sélection
    // initiale (premier élément de la liste, ou initialChampion) au même
    // titre qu'un changement manuel ultérieur.
    connect(m_champCombo, &QComboBox::currentTextChanged, this, &AddSkinDialog::onChampionChanged);
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
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &AddSkinDialog::onSkinNameEditingFinished);
    connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated),
            this, [this](const QString&) { onSkinNameEditingFinished(); });

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

AddSkinDialog::~AddSkinDialog() {
    abortPendingRequest();
}

void AddSkinDialog::onGratuitToggled(bool checked) {
    m_prix->setValue(0);
    m_prix->setEnabled(!checked);
    if (checked) {
        m_rareteCombo->setCurrentText("Gratuit");
        m_possedeCb->setChecked(true);
    }
}

void AddSkinDialog::onChampionChanged(const QString& champion) {
    // Ne déclenche une recherche que sur une vraie sélection (un nom déjà
    // connu de la liste), pas sur un nom librement tapé dans le combo
    // éditable — celui-ci reste alors en saisie 100% manuelle, comme avant.
    if (!m_championNames.contains(champion)) return;
    lookupChampionSkins(champion);
}

void AddSkinDialog::onSkinNameEditingFinished() {
    applySuggestion(m_nameEdit->text());
}

void AddSkinDialog::applySuggestion(const QString& typedName) {
    if (m_gratuitCb->isChecked()) return; // choix explicite déjà fait par l'utilisateur

    const auto it = m_suggestions.constFind(normalize(typedName));
    if (it == m_suggestions.constEnd()) return; // nom non reconnu : on n'altère rien

    m_rareteCombo->setCurrentText(it->rarete);
    const int eo = eoPriceForRarete(it->rarete);
    if (eo > 0) m_prix->setValue(eo);
}

void AddSkinDialog::lookupChampionSkins(const QString& champion) {
    m_suggestions.clear();
    m_completerModel->setStringList({});

    if (!m_indexLoaded) {
        m_pendingChampion = champion;
        fetchChampionIndex();
        return;
    }

    const int id = m_champNameToId.value(normalize(champion), -1);
    if (id <= 0) return; // champion non reconnu côté Community Dragon : pas grave, saisie manuelle
    fetchSkinsFor(id);
}

void AddSkinDialog::fetchChampionIndex() {
    if (m_indexLoading) return;
    m_indexLoading = true;
    abortPendingRequest();

    const QUrl url(QString("https://raw.communitydragon.org/latest/plugins/rcp-be-lol-game-data/global/%1/v1/champion-summary.json").arg(m_locale));
    m_currentReply = m_nam->get(QNetworkRequest(url));
    connect(m_currentReply, &QNetworkReply::finished, this, [this] {
        QNetworkReply* reply = m_currentReply;
        m_currentReply = nullptr;
        onChampionIndexReply(reply);
    });
}

void AddSkinDialog::onChampionIndexReply(QNetworkReply* reply) {
    reply->deleteLater();
    m_indexLoading = false;

    if (reply->error() != QNetworkReply::NoError) {
        // Repli EN si fr_fr n'est pas servi pour cet endpoint ; au-delà,
        // simplement hors-ligne / source indisponible -> aucune suggestion,
        // dialogue intégralement manuel, sans popup.
        if (!m_localeFallbackTried && m_locale != "default") {
            m_localeFallbackTried = true;
            m_locale = "default";
            fetchChampionIndex();
        }
        return;
    }

    const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
    for (const QJsonValue& v : arr) {
        const QJsonObject obj = v.toObject();
        const int id = obj.value("id").toInt(-1);
        const QString name = obj.value("name").toString();
        if (id <= 0 || name.isEmpty()) continue;
        m_champNameToId.insert(normalize(name), id);
    }
    m_indexLoaded = true;

    if (!m_pendingChampion.isEmpty()) {
        const QString champ = m_pendingChampion;
        m_pendingChampion.clear();
        lookupChampionSkins(champ);
    }
}

void AddSkinDialog::fetchSkinsFor(int championId) {
    abortPendingRequest();

    const QUrl url(QString("https://raw.communitydragon.org/latest/plugins/rcp-be-lol-game-data/global/%1/v1/champions/%2.json")
                       .arg(m_locale).arg(championId));
    m_currentReply = m_nam->get(QNetworkRequest(url));
    connect(m_currentReply, &QNetworkReply::finished, this, [this] {
        QNetworkReply* reply = m_currentReply;
        m_currentReply = nullptr;
        onSkinsReply(reply);
    });
}

void AddSkinDialog::onSkinsReply(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) return; // best-effort : on laisse vide

    const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
    const QJsonArray skins = root.value("skins").toArray();

    QStringList displayNames;
    for (const QJsonValue& v : skins) {
        const QJsonObject o = v.toObject();
        if (o.value("isBase").toBool()) continue; // skin de base : pas pertinent à suggérer ici
        const QString name = o.value("name").toString();
        if (name.isEmpty()) continue;
        m_suggestions.insert(normalize(name), { cdragonRarityToLabel(o.value("rarity").toString()) });
        displayNames << name;
    }
    m_completerModel->setStringList(displayNames);
}

void AddSkinDialog::abortPendingRequest() {
    if (m_currentReply) {
        m_currentReply->disconnect();
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
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