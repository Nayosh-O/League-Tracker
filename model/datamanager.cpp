#include "datamanager.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

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
    m_eb = root["essenceBleu"].toInt(17682);
    m_eo = root["essenceOrange"].toInt(1830);

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
        {"Aatrox",         2400, 0,    true },
        {"Ahri",           1575, 0,    true },
        {"Akali",          1575, 945,  true },
        {"Akshan",         2400, 0,    true },
        {"Alistar",        675,  0,    true },
        {"Ambessa",        2400, 1440, true },
        {"Amumu",          225,  0,    true },
        {"Anivia",         1575, 0,    true },
        {"Annie",          225,  0,    true },
        {"Aphelios",       2400, 1440, true },
        {"Ashe",           225,  0,    true },
        {"Aurelion Sol",   2400, 0,    true },
        {"Aurora",         2400, 0,    true },
        {"Azir",           2400, 0,    true },
        {"Bard",           2400, 0,    true },
        {"Bel'Veth",       2400, 0,    true },
        {"Blitzcrank",     675,  0,    true },
        {"Brand",          225,  0,    true },
        {"Braum",          1575, 0,    true },
        {"Briar",          2400, 0,    true },
        {"Caitlyn",        1575, 0,    true },
        {"Camille",        2400, 0,    true },
        {"Cassiopeia",     2400, 1440, true },
        {"Cho'Gath",       675,  0,    true },
        {"Corki",          1575, 0,    false},
        {"Darius",         2400, 0,    true },
        {"Diana",          2400, 0,    true },
        {"Dr. Mundo",      675,  0,    true },
        {"Draven",         2400, 1440, false},
        {"Ekko",           2400, 0,    true },
        {"Elise",          2400, 0,    true },
        {"Evelynn",        1575, 0,    true },
        {"Ezreal",         1575, 0,    true },
        {"Fiddlesticks",   675,  0,    true },
        {"Fiora",          2400, 0,    true },
        {"Fizz",           675,  0,    true },
        {"Galio",          2400, 0,    true },
        {"Gangplank",      1575, 0,    true },
        {"Garen",          225,  0,    true },
        {"Gnar",           2400, 0,    true },
        {"Gragas",         1575, 0,    true },
        {"Graves",         2400, 0,    false},
        {"Gwen",           2400, 0,    true },
        {"Hecarim",        2400, 0,    true },
        {"Heimerdinger",   1575, 945,  false},
        {"Hwei",           2400, 0,    true },
        {"Illaoi",         1575, 0,    false},
        {"Irelia",         1575, 0,    true },
        {"Ivern",          2400, 0,    true },
        {"Janna",          675,  0,    true },
        {"Jarvan IV",      1575, 0,    true },
        {"Jax",            1575, 0,    true },
        {"Jayce",          2400, 0,    true },
        {"Jhin",           2400, 0,    true },
        {"Jinx",           2400, 0,    true },
        {"K'Sante",        2400, 0,    true },
        {"Kai'Sa",         2400, 0,    true },
        {"Kalista",        2400, 0,    false},
        {"Karma",          1575, 0,    true },
        {"Karthus",        1575, 0,    false},
        {"Kassadin",       1575, 945,  true },
        {"Katarina",       1575, 0,    true },
        {"Kayle",          2400, 0,    true },
        {"Kayn",           2400, 0,    true },
        {"Kennen",         2400, 1440, true },
        {"Kha'Zix",        2400, 1440, true },
        {"Kindred",        2400, 0,    true },
        {"Kled",           2400, 0,    false},
        {"LeBlanc",        1575, 0,    true },
        {"Kog'Maw",        2400, 0,    false},
        {"Lee Sin",        675,  0,    true },
        {"Leona",          225,  135,  true },
        {"Lillia",         2400, 0,    true },
        {"Lissandra",      2400, 0,    true },
        {"Lucian",         2400, 0,    true },
        {"Lulu",           2400, 0,    true },
        {"Lux",            2400, 0,    true },
        {"Maitre Yi",      225,  0,    true },
        {"Malphite",       675,  0,    true },
        {"Malzahar",       2400, 0,    true },
        {"Maokai",         1575, 0,    true },
        {"Mel",            2400, 0,    true },
        {"Milio",          2400, 0,    true },
        {"Miss Fortune",   2400, 0,    true },
        {"Mordekaiser",    2400, 0,    true },
        {"Morgana",        675,  0,    true },
        {"Naafiri",        2400, 0,    true },
        {"Nami",           1575, 0,    true },
        {"Nasus",          675,  0,    true },
        {"Nautilus",       2400, 0,    true },
        {"Neeko",          2400, 0,    true },
        {"Nidalee",        1575, 0,    true },
        {"Nilah",          2400, 0,    true },
        {"Nocturne",       1575, 0,    true },
        {"Nunu & Willump", 225,  0,    true },
        {"Olaf",           1575, 0,    true },
        {"Orianna",        2400, 1440, true },
        {"Ornn",           2400, 0,    true },
        {"Pantheon",       2400, 0,    true },
        {"Poppy",          2400, 0,    true },
        {"Pyke",           2400, 0,    true },
        {"Qiyana",         2400, 0,    true },
        {"Quinn",          2400, 0,    true },
        {"Rakan",          2400, 0,    true },
        {"Rammus",         675,  0,    true },
        {"Rek'Sai",        2400, 0,    false},
        {"Rell",           2400, 0,    true },
        {"Renata Glasc",   2400, 0,    true },
        {"Renekton",       2400, 0,    true },
        {"Rengar",         2400, 0,    true },
        {"Riven",          2400, 0,    true },
        {"Rumble",         2400, 1440, false},
        {"Ryze",           675,  0,    true },
        {"Samira",         2400, 0,    true },
        {"Sejuani",        225,  0,    true },
        {"Senna",          2400, 0,    true },
        {"Seraphine",      2400, 0,    true },
        {"Sett",           2400, 0,    true },
        {"Shaco",          1575, 0,    true },
        {"Shen",           1575, 0,    true },
        {"Shyvana",        1575, 0,    true },
        {"Singed",         225,  0,    true },
        {"Sion",           1575, 0,    true },
        {"Sivir",          225,  0,    false},
        {"Skarner",        2400, 0,    true },
        {"Smolder",        2400, 0,    true },
        {"Sona",           675,  0,    true },
        {"Soraka",         225,  0,    true },
        {"Swain",          1575, 0,    true },
        {"Sylas",          2400, 0,    true },
        {"Syndra",         1575, 0,    true },
        {"Tahm Kench",     2400, 0,    true },
        {"Taliyah",        2400, 0,    false},
        {"Talon",          2400, 0,    true },
        {"Taric",          675,  0,    true },
        {"Teemo",          675,  0,    true },
        {"Thresh",         2400, 0,    true },
        {"Tristana",       675,  0,    true },
        {"Trundle",        1575, 0,    true },
        {"Tryndamere",     1575, 0,    true },
        {"Twisted Fate",   675,  0,    true },
        {"Twitch",         1575, 0,    true },
        {"Udyr",           2400, 0,    true },
        {"Urgot",          2400, 0,    true },
        {"Varus",          2400, 0,    false},
        {"Vayne",          2400, 0,    true },
        {"Veigar",         675,  0,    true },
        {"Vel'Koz",        1618, 0,    true },
        {"Vex",            2400, 0,    true },
        {"Vi",             2400, 0,    true },
        {"Viego",          2400, 0,    true },
        {"Viktor",         2400, 0,    true },
        {"Vladimir",       2400, 0,    true },
        {"Volibear",       675,  405,  true },
        {"Warwick",        225,  0,    true },
        {"Wukong",         1575, 0,    true },
        {"Xayah",          2400, 0,    false},
        {"Xerath",         2400, 0,    true },
        {"Xin Zhao",       675,  0,    true },
        {"Yasuo",          2400, 0,    true },
        {"Yone",           2400, 0,    true },
        {"Yorick",         1575, 0,    true },
        {"Yunara",         2400, 0,    true },
        {"Yuumi",          2400, 0,    true },
        {"Zaahen",         2400, 0,    true },
        {"Zac",            2400, 0,    true },
        {"Zed",            2400, 0,    true },
        {"Zeri",           2400, 0,    true },
        {"Ziggs",          2400, 1440, true },
        {"Zilean",         675,  0,    true },
        {"Zoe",            2400, 1440, true },
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
    struct RawSkin { const char* nom; const char* champ; int prix; bool gratuit; const char* rarete; };
    static const RawSkin rawSkins[] = {
        {"Heimerdinger Explosif",            "Heimerdinger", 0,    true,  "Gratuit"  },
        {"Nidalee Lapin des Neiges",         "Nidalee",      0,    true,  "Gratuit"  },
        {"Vel'Koz Infernal",                 "Vel'Koz",      0,    true,  "Gratuit"  },
        {"DRX Kindred",                      "Kindred",      1050, false, "Epique"   },
        {"Gwen Reine du Combat",             "Gwen",         1050, false, "Epique"   },
        {"Hecarim de l'Ouest",               "Hecarim",      1050, false, "Epique"   },
        {"Hecarim sans Tete",                "Hecarim",      675,  false, "Basique"  },
        {"Janna Gardienne des Sables",       "Janna",        1050, false, "Epique"   },
        {"Karma du Pulsar Sombre",           "Karma",        1050, false, "Epique"   },
        {"Lee Sin Dragon des Tempetes",      "Lee Sin",      1520, false, "Legendaire"},
        {"Maitre Yi Fleur Spirituelle",      "Maitre Yi",    1050, false, "Epique"   },
        {"Malzahar Seducteur",               "Malzahar",     1050, false, "Epique"   },
        {"Neeko des rouleaux de Shan Hai",   "Neeko",        1050, false, "Epique"   },
        {"Nidalee Fleur Spirituelle",        "Nidalee",      1050, false, "Epique"   },
        {"Renekton des Worlds 2023",         "Renekton",     1050, false, "Epique"   },
        {"Renekton Guerrier d'Encre",        "Renekton",     1050, false, "Epique"   },
        {"Swain de la Chasse eternelle",     "Swain",        1050, false, "Epique"   },
        {"Teemo de la Section Omega",        "Teemo",        1520, false, "Legendaire"},
        {"Yorick Pentakill III",             "Yorick",       1050, false, "Epique"   },
        {"Ziggs Boss de Combat",             "Ziggs",        1050, false, "Epique"   },
        {"Zilean Elu de l'Hiver",            "Zilean",       1050, false, "Epique"   },
        // 👉 Nouveau skin sorti sur LoL ? Ajoute une ligne ici, au format
        //    {"Nom du skin", "Champion", prix (0 si gratuit), gratuit, "Rareté"}.
        //    Au prochain lancement il sera ajouté automatiquement à ta
        //    sauvegarde existante (cf. DataManager::mergeNewSkins()).
    };
    QVector<Skin> result;
    result.reserve(sizeof(rawSkins) / sizeof(rawSkins[0]));
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
    struct RawBalise { const char* nom; int prix; bool pos; };
    static const RawBalise rawBalises[] = {
        {"Balise Akana de Fleur Spirituelle 2020",    0,   true },
        {"Balise Amplificateur Optique",              0,   true },
        {"Balise Annee du Cochon",                    0,   true },
        {"Balise Arcane Mysterieuse",                 0,   true },
        {"Balise Banniere du Cheval",                 0,   true },
        {"Balise Colombe de l'Amour",                 340, false},
        {"Balise Contes de la Faille 2019",           340, false},
        {"Balise Durandal de l'Academie du Combat",   340, false},
        {"Balise Fan des Chiens",                     0,   true },
        {"Balise Galaxies 2020",                      0,   true },
        {"Balise Gong",                               340, false},
        {"Balise Habit Mysterieux",                   0,   true },
        {"Balise Heartsteel",                         0,   true },
        {"Balise Legende Glorieuse",                  0,   true },
        {"Balise Musique Pop",                        0,   true },
        {"Balise Par Defaut",                         0,   true },
        {"Balise Primordien",                         340, false},
        {"Balise Pulsefire 2018",                     0,   true },
        {"Balise Saison 2 2026",                      0,   true },
        {"Balise Sucre d'Orge",                       0,   true },
        {"Balise Seducteur",                          340, false},
        {"Balise T1 2024",                            0,   true },
        {"Balise Vamporo",                            0,   true },
        {"Balise Veuve",                              0,   true },
        {"Balise A Fond",                             0,   true },
        {"Balise Elus de l'Hiver",                    340, false},
        {"Balise Epee Divine",                        340, false},
        // 👉 Nouvelle balise sortie sur LoL ? Ajoute une ligne ici, au format
        //    {"Nom de la balise", prix (0 si gratuite/déjà possédée), possede}.
        //    Au prochain lancement elle sera ajoutée automatiquement à ta
        //    sauvegarde existante (cf. DataManager::mergeNewBalises()).
    };
    QVector<Balise> result;
    result.reserve(sizeof(rawBalises) / sizeof(rawBalises[0]));
    for (const auto& r : rawBalises) {
        Balise b;
        b.nom     = QString::fromUtf8(r.nom);
        b.prix    = r.prix;
        b.possede = r.pos;
        result << b;
    }
    return result;
}