#include "datamanager.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>
#include <vector>

DataManager* DataManager::instance() {
    static DataManager dm;
    return &dm;
}

QString DataManager::dataPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/lol_data.json";
}

int DataManager::champsOwned() const {
    int n = 0;
    for (const auto& c : m_champions) if (c.possede) ++n;
    return n;
}
int DataManager::champsToBuy() const {
    int n = 0;
    for (const auto& c : m_champions) if (!c.possede) ++n;
    return n;
}
int DataManager::coutTotalRestant() const {
    int total = 0;
    for (const auto& c : m_champions)
        if (!c.possede) total += c.prixEffectif();
    return total;
}
int DataManager::ebApresAchat() const {
    return m_eb - coutTotalRestant();
}

void DataManager::updateChampion(const Champion& c) {
    for (auto& ch : m_champions) {
        if (ch.nom == c.nom) {
            ch = c;
            break;
        }
    }
    save();
    emit dataChanged();
}

void DataManager::setBaliseOwned(int index, bool owned) {
    if (index < 0 || index >= m_balises.size()) return;
    m_balises[index].possede = owned;
    save();
    emit dataChanged();
}

void DataManager::setSkinOwned(int index, bool owned) {
    if (index < 0 || index >= m_skins.size()) return;
    m_skins[index].possede = owned;
    save();
    emit dataChanged();
}

bool DataManager::addChampion(const Champion& c) {
    const QString nom = c.nom.trimmed();
    if (nom.isEmpty()) return false;

    for (const auto& ch : m_champions)
        if (ch.nom.compare(nom, Qt::CaseInsensitive) == 0) return false;

    Champion nc = c;
    nc.nom = nom;
    m_champions << nc;
    save();
    emit dataChanged();
    return true;
}

bool DataManager::addSkin(const Skin& s) {
    const QString nom = s.nom.trimmed();
    if (nom.isEmpty()) return false;

    for (const auto& sk : m_skins)
        if (sk.nom.compare(nom, Qt::CaseInsensitive) == 0) return false;

    Skin ns = s;
    ns.nom = nom;
    m_skins << ns;
    save();
    emit dataChanged();
    return true;
}

bool DataManager::addBalise(const Balise& b) {
    const QString nom = b.nom.trimmed();
    if (nom.isEmpty()) return false;

    for (const auto& bl : m_balises)
        if (bl.nom.compare(nom, Qt::CaseInsensitive) == 0) return false;

    Balise nb = b;
    nb.nom = nom;
    m_balises << nb;
    save();
    emit dataChanged();
    return true;
}

void DataManager::removeChampion(int index) {
    if (index < 0 || index >= m_champions.size()) return;
    m_champions.removeAt(index);
    save();
    emit dataChanged();
}

void DataManager::removeSkin(int index) {
    if (index < 0 || index >= m_skins.size()) return;
    m_skins.removeAt(index);
    save();
    emit dataChanged();
}

void DataManager::removeBalise(int index) {
    if (index < 0 || index >= m_balises.size()) return;
    m_balises.removeAt(index);
    save();
    emit dataChanged();
}

void DataManager::mergeNewChampions() {
    bool changed = false;
    for (const auto& ref : referenceChampions()) {
        bool found = false;
        for (const auto& c : m_champions) {
            if (c.nom.compare(ref.nom, Qt::CaseInsensitive) == 0) { found = true; break; }
        }
        if (!found) { m_champions << ref; changed = true; }
    }
    if (changed) save();
}

void DataManager::mergeNewSkins() {
    bool changed = false;
    for (const auto& ref : referenceSkins()) {
        bool found = false;
        for (const auto& s : m_skins) {
            if (s.nom.compare(ref.nom, Qt::CaseInsensitive) == 0) { found = true; break; }
        }
        if (!found) { m_skins << ref; changed = true; }
    }
    if (changed) save();
}

void DataManager::mergeNewBalises() {
    bool changed = false;
    for (const auto& ref : referenceBalises()) {
        bool found = false;
        for (const auto& b : m_balises) {
            if (b.nom.compare(ref.nom, Qt::CaseInsensitive) == 0) { found = true; break; }
        }
        if (!found) { m_balises << ref; changed = true; }
    }
    if (changed) save();
}

void DataManager::load() {
    QFile f(dataPath());
    if (!f.exists()) { initDefaultData(); save(); return; }
    if (!f.open(QIODevice::ReadOnly)) {
        // Le fichier existe mais n'a pas pu être ouvert (verrouillé,
        // permissions...). On ne touche pas aux données déjà en mémoire
        // pour éviter de tout effacer silencieusement, et on prévient
        // dans la console pour faciliter le diagnostic.
        qWarning() << "DataManager::load() : impossible d'ouvrir" << dataPath()
                   << "-" << f.errorString();
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    QJsonObject root = doc.object();
    m_eb = root["essenceBleu"].toInt(0);
    m_eo = root["essenceOrange"].toInt(0);

    m_champions.clear();
    for (const QJsonValue& v : root["champions"].toArray()) {
        QJsonObject o = v.toObject();
        Champion c;
        c.nom          = o["nom"].toString();
        c.prixStandard = o["prixStandard"].toInt();
        c.prixReduit   = o["prixReduit"].toInt();
        c.possede      = o["possede"].toBool();
        m_champions << c;
    }

    m_skins.clear();
    for (const QJsonValue& v : root["skins"].toArray()) {
        QJsonObject o = v.toObject();
        Skin s;
        s.nom       = o["nom"].toString();
        s.champion  = o["champion"].toString();
        s.prix      = o["prix"].toInt();
        s.gratuit   = o["gratuit"].toBool();
        s.rarete    = o["rarete"].toString();
        s.possede   = o["possede"].toBool();
        m_skins << s;
    }

    m_balises.clear();
    for (const QJsonValue& v : root["balises"].toArray()) {
        QJsonObject o = v.toObject();
        Balise b;
        b.nom     = o["nom"].toString();
        b.prix    = o["prix"].toInt();
        b.possede = o["possede"].toBool();
        m_balises << b;
    }

    // Ajoute automatiquement les champions / skins / balises récemment
    // sorties : toute entrée de référence absente de la sauvegarde est
    // ajoutée (par défaut non possédée), sans toucher au reste.
    mergeNewChampions();
    mergeNewSkins();
    mergeNewBalises();
}

void DataManager::save() {
    QJsonArray jChamps;
    for (const auto& c : m_champions) {
        QJsonObject o;
        o["nom"]          = c.nom;
        o["prixStandard"] = c.prixStandard;
        o["prixReduit"]   = c.prixReduit;
        o["possede"]      = c.possede;
        jChamps << o;
    }

    QJsonArray jSkins;
    for (const auto& s : m_skins) {
        QJsonObject o;
        o["nom"]      = s.nom;
        o["champion"] = s.champion;
        o["prix"]     = s.prix;
        o["gratuit"]  = s.gratuit;
        o["rarete"]   = s.rarete;
        o["possede"]  = s.possede;
        jSkins << o;
    }

    QJsonArray jBalises;
    for (const auto& b : m_balises) {
        QJsonObject o;
        o["nom"]     = b.nom;
        o["prix"]    = b.prix;
        o["possede"] = b.possede;
        jBalises << o;
    }

    QJsonObject root;
    root["essenceBleu"]    = m_eb;
    root["essenceOrange"]  = m_eo;
    root["champions"]      = jChamps;
    root["skins"]          = jSkins;
    root["balises"]        = jBalises;

    QFile f(dataPath());
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "DataManager::save() : impossible d'écrire dans" << dataPath()
                   << "-" << f.errorString();
        return;
    }
    f.write(QJsonDocument(root).toJson());
}

QVector<Champion> DataManager::referenceChampions() {
    // ─── 172 Champions ───────────────────────────────────────────────────────
    struct RawChamp { const char* nom; int ps; int pr; bool pos; };
    static const RawChamp raw[] = {
        {"Aatrox",         2400, 0,    false},
        {"Ahri",           1575, 0,    false},
        {"Akali",          1575, 0,  false},
        {"Akshan",         2400, 0,    false},
        {"Alistar",        675,  0,    false},
        {"Ambessa",        2400, 0, false},
        {"Amumu",          225,  0,    false},
        {"Anivia",         1575, 0,    false},
        {"Annie",          225,  0,    false},
        {"Aphelios",       2400, 0, false},
        {"Ashe",           225,  0,    false},
        {"Aurelion Sol",   2400, 0,    false},
        {"Aurora",         2400, 0,    false},
        {"Azir",           2400, 0,    false},
        {"Bard",           2400, 0,    false},
        {"Bel'Veth",       2400, 0,    false},
        {"Blitzcrank",     675,  0,    false},
        {"Brand",          225,  0,    false},
        {"Braum",          1575, 0,    false},
        {"Briar",          2400, 0,    false},
        {"Caitlyn",        1575, 0,    false},
        {"Camille",        2400, 0,    false},
        {"Cassiopeia",     2400, 0, false},
        {"Cho'Gath",       675,  0,    false},
        {"Corki",          1575, 0,    false},
        {"Darius",         2400, 0,    false},
        {"Diana",          2400, 0,    false},
        {"Dr. Mundo",      675,  0,    false},
        {"Draven",         2400, 0, false},
        {"Ekko",           2400, 0,    false},
        {"Elise",          2400, 0,    false},
        {"Evelynn",        1575, 0,    false},
        {"Ezreal",         1575, 0,    false},
        {"Fiddlesticks",   675,  0,    false},
        {"Fiora",          2400, 0,    false},
        {"Fizz",           675,  0,    false},
        {"Galio",          2400, 0,    false},
        {"Gangplank",      1575, 0,    false},
        {"Garen",          225,  0,    false},
        {"Gnar",           2400, 0,    false},
        {"Gragas",         1575, 0,    false},
        {"Graves",         2400, 0,    false},
        {"Gwen",           2400, 0,    false},
        {"Hecarim",        2400, 0,    false},
        {"Heimerdinger",   1575, 0,  false},
        {"Hwei",           2400, 0,    false},
        {"Illaoi",         1575, 0,    false},
        {"Irelia",         1575, 0,    false},
        {"Ivern",          2400, 0,    false},
        {"Janna",          675,  0,    false},
        {"Jarvan IV",      1575, 0,    false},
        {"Jax",            1575, 0,    false},
        {"Jayce",          2400, 0,    false},
        {"Jhin",           2400, 0,    false},
        {"Jinx",           2400, 0,    false},
        {"K'Sante",        2400, 0,    false},
        {"Kai'Sa",         2400, 0,    false},
        {"Kalista",        2400, 0,    false},
        {"Karma",          1575, 0,    false},
        {"Karthus",        1575, 0,    false},
        {"Kassadin",       1575, 0,  false},
        {"Katarina",       1575, 0,    false},
        {"Kayle",          2400, 0,    false},
        {"Kayn",           2400, 0,    false},
        {"Kennen",         2400, 0, false},
        {"Kha'Zix",        2400, 0, false},
        {"Kindred",        2400, 0,    false},
        {"Kled",           2400, 0,    false},
        {"LeBlanc",        1575, 0,    false},
        {"Kog'Maw",        2400, 0,    false},
        {"Lee Sin",        675,  0,    false},
        {"Leona",          225,  0,  false},
        {"Lillia",         2400, 0,    false},
        {"Lissandra",      2400, 0,    false},
        {"Lucian",         2400, 0,    false},
        {"Lulu",           2400, 0,    false},
        {"Lux",            2400, 0,    false},
        {"Maitre Yi",      225,  0,    false},
        {"Malphite",       675,  0,    false},
        {"Malzahar",       2400, 0,    false},
        {"Maokai",         1575, 0,    false},
        {"Mel",            2400, 0,    false},
        {"Milio",          2400, 0,    false},
        {"Miss Fortune",   2400, 0,    false},
        {"Mordekaiser",    2400, 0,    false},
        {"Morgana",        675,  0,    false},
        {"Naafiri",        2400, 0,    false},
        {"Nami",           1575, 0,    false},
        {"Nasus",          675,  0,    false},
        {"Nautilus",       2400, 0,    false},
        {"Neeko",          2400, 0,    false},
        {"Nidalee",        1575, 0,    false},
        {"Nilah",          2400, 0,    false},
        {"Nocturne",       1575, 0,    false},
        {"Nunu & Willump", 225,  0,    false},
        {"Olaf",           1575, 0,    false},
        {"Orianna",        2400, 0, false},
        {"Ornn",           2400, 0,    false},
        {"Pantheon",       2400, 0,    false},
        {"Poppy",          2400, 0,    false},
        {"Pyke",           2400, 0,    false},
        {"Qiyana",         2400, 0,    false},
        {"Quinn",          2400, 0,    false},
        {"Rakan",          2400, 0,    false},
        {"Rammus",         675,  0,    false},
        {"Rek'Sai",        2400, 0,    false},
        {"Rell",           2400, 0,    false},
        {"Renata Glasc",   2400, 0,    false},
        {"Renekton",       2400, 0,    false},
        {"Rengar",         2400, 0,    false},
        {"Riven",          2400, 0,    false},
        {"Rumble",         2400, 0, false},
        {"Ryze",           675,  0,    false},
        {"Samira",         2400, 0,    false},
        {"Sejuani",        225,  0,    false},
        {"Senna",          2400, 0,    false},
        {"Seraphine",      2400, 0,    false},
        {"Sett",           2400, 0,    false},
        {"Shaco",          1575, 0,    false},
        {"Shen",           1575, 0,    false},
        {"Shyvana",        1575, 0,    false},
        {"Singed",         225,  0,    false},
        {"Sion",           1575, 0,    false},
        {"Sivir",          225,  0,    false},
        {"Skarner",        2400, 0,    false},
        {"Smolder",        2400, 0,    false},
        {"Sona",           675,  0,    false},
        {"Soraka",         225,  0,    false},
        {"Swain",          1575, 0,    false},
        {"Sylas",          2400, 0,    false},
        {"Syndra",         1575, 0,    false},
        {"Tahm Kench",     2400, 0,    false},
        {"Taliyah",        2400, 0,    false},
        {"Talon",          2400, 0,    false},
        {"Taric",          675,  0,    false},
        {"Teemo",          675,  0,    false},
        {"Thresh",         2400, 0,    false},
        {"Tristana",       675,  0,    false},
        {"Trundle",        1575, 0,    false},
        {"Tryndamere",     1575, 0,    false},
        {"Twisted Fate",   675,  0,    false},
        {"Twitch",         1575, 0,    false},
        {"Udyr",           2400, 0,    false},
        {"Urgot",          2400, 0,    false},
        {"Varus",          2400, 0,    false},
        {"Vayne",          2400, 0,    false},
        {"Veigar",         675,  0,    false},
        {"Vel'Koz",        1618, 0,    false},
        {"Vex",            2400, 0,    false},
        {"Vi",             2400, 0,    false},
        {"Viego",          2400, 0,    false},
        {"Viktor",         2400, 0,    false},
        {"Vladimir",       2400, 0,    false},
        {"Volibear",       675,  0,  false},
        {"Warwick",        225,  0,    false},
        {"Wukong",         1575, 0,    false},
        {"Xayah",          2400, 0,    false},
        {"Xerath",         2400, 0,    false},
        {"Xin Zhao",       675,  0,    false},
        {"Yasuo",          2400, 0,    false},
        {"Yone",           2400, 0,    false},
        {"Yorick",         1575, 0,    false},
        {"Yunara",         2400, 0,    false},
        {"Yuumi",          2400, 0,    false},
        {"Zaahen",         2400, 0,    false},
        {"Zac",            2400, 0,    false},
        {"Zed",            2400, 0,    false},
        {"Zeri",           2400, 0,    false},
        {"Ziggs",          2400, 0, false},
        {"Zilean",         675,  0,    false},
        {"Zoe",            2400, 0, false},
        {"Zyra",           1575, 0,    false},
        // 👉 Nouveau champion sorti sur LoL ? Ajoute une ligne ici, au
        //    format {"Nom", prixStandard, prixReduit (0 si aucun), possede}.
        //    Au prochain lancement, il sera ajouté automatiquement à ta
        //    sauvegarde existante (cf. DataManager::mergeNewChampions()).
    };
    QVector<Champion> result;
    result.reserve(sizeof(raw) / sizeof(raw[0]));
    for (const auto& r : raw) {
        Champion c;
        c.nom          = QString::fromUtf8(r.nom);
        c.prixStandard = r.ps;
        c.prixReduit   = r.pr;
        c.possede      = r.pos;
        result << c;
    }
    return result;
}

void DataManager::initDefaultData() {
    m_champions = referenceChampions();
    m_skins     = referenceSkins();
    m_balises   = referenceBalises();
}

QVector<Skin> DataManager::referenceSkins() {
    // ─── Skins ───────────────────────────────────────────────────────────────
    // Aucun skin par défaut : la liste démarre vide. Ajoute-en via le
    // bouton « ✚ Nouveau skin » dans l'appli, ou ajoute des lignes
    // {"Nom du skin", "Champion", prix (0 si gratuit), gratuit, "Rareté"}
    // dans le tableau ci-dessous si tu préfères les avoir dès le premier
    // lancement (cf. DataManager::mergeNewSkins()).
    struct RawSkin { const char* nom; const char* champ; int prix; bool gratuit; const char* rarete; };
    static const std::vector<RawSkin> rawSkins = {
                                                   // {"Nom du skin", "Champion", prix, gratuit, "Rareté"},
                                                   };
    QVector<Skin> result;
    result.reserve(static_cast<int>(rawSkins.size()));
    for (const auto& r : rawSkins) {
        Skin s;
        s.nom      = QString::fromUtf8(r.nom);
        s.champion = QString::fromUtf8(r.champ);
        s.prix     = r.prix;
        s.gratuit  = r.gratuit;
        s.rarete   = QString::fromUtf8(r.rarete);
        s.possede  = r.gratuit; // un skin gratuit est considéré comme déjà acquis
        result << s;
    }
    return result;
}

QVector<Balise> DataManager::referenceBalises() {
    // ─── Balises ─────────────────────────────────────────────────────────────
    // Aucune balise par défaut : la liste démarre vide. Ajoute-en via le
    // bouton « ✚ Nouvelle balise » dans l'appli, ou ajoute des lignes
    // {"Nom de la balise", prix (0 si gratuite), possede} dans le tableau
    // ci-dessous si tu préfères les avoir dès le premier lancement
    // (cf. DataManager::mergeNewBalises()).
    struct RawBalise { const char* nom; int prix; bool pos; };
    static const std::vector<RawBalise> rawBalises = {
                                                       // {"Nom de la balise", prix, possede},
                                                       };
    QVector<Balise> result;
    result.reserve(static_cast<int>(rawBalises.size()));
    for (const auto& r : rawBalises) {
        Balise b;
        b.nom     = QString::fromUtf8(r.nom);
        b.prix    = r.prix;
        b.possede = r.pos;
        result << b;
    }
    return result;
}