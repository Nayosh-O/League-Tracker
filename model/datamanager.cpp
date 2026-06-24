#include "datamanager.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDate>
#include <QDateTime>
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

int DataManager::valeurChampionsPossedes() const {
    int total = 0;
    for (const auto& c : m_champions)
        if (c.possede) total += c.prixEffectif();
    return total;
}

int DataManager::valeurSkinsBalisesPossedes() const {
    int total = 0;
    for (const auto& s : m_skins)   if (s.possede) total += s.prix;
    for (const auto& b : m_balises) if (b.possede) total += b.prix;
    return total;
}

void DataManager::setEssenceBleu(int v) {
    if (v == m_eb) return; // pas de changement réel : pas de nouveau point d'historique
    m_eb = v;
    logEssenceSnapshot();
    save();
    emit dataChanged();
}

void DataManager::setEssenceOrange(int v) {
    if (v == m_eo) return;
    m_eo = v;
    logEssenceSnapshot();
    save();
    emit dataChanged();
}

// Ajoute un point d'historique horodaté pour CE changement précis — un
// point par appel, sans regroupement par jour, pour que la courbe
// d'évolution reflète aussi plusieurs changements dans la même journée.
void DataManager::logEssenceSnapshot() {
    EssenceSnapshot s;
    s.date = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    s.eb   = m_eb;
    s.eo   = m_eo;
    m_history << s;
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

void DataManager::backfillRoles() {
    const QVector<Champion> refs = referenceChampions();
    bool changed = false;
    for (auto& c : m_champions) {
        if (!c.roles.isEmpty()) continue; // déjà renseigné, on ne touche à rien
        for (const auto& ref : refs) {
            if (ref.nom.compare(c.nom, Qt::CaseInsensitive) == 0) {
                c.roles = ref.roles;
                changed = true;
                break;
            }
        }
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
    m_rolesMigrated = root["rolesMigrated"].toBool(false);

    m_champions.clear();
    for (const QJsonValue& v : root["champions"].toArray()) {
        QJsonObject o = v.toObject();
        Champion c;
        c.nom          = o["nom"].toString();
        c.prixStandard = o["prixStandard"].toInt();
        c.prixReduit   = o["prixReduit"].toInt();
        c.possede      = o["possede"].toBool();
        c.prioritaire  = o["prioritaire"].toBool();
        for (const QJsonValue& rv : o["roles"].toArray())
            c.roles << rv.toString();
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

    m_history.clear();
    for (const QJsonValue& v : root["historique"].toArray()) {
        QJsonObject o = v.toObject();
        EssenceSnapshot s;
        s.date = o["date"].toString();
        s.eb   = o["eb"].toInt();
        s.eo   = o["eo"].toInt();
        m_history << s;
    }

    // Ajoute automatiquement les champions / skins / balises récemment
    // sorties : toute entrée de référence absente de la sauvegarde est
    // ajoutée (par défaut non possédée), sans toucher au reste.
    mergeNewChampions();
    mergeNewSkins();
    mergeNewBalises();

    // Rétro-compatibilité : les sauvegardes créées avant l'ajout des tags
    // de rôle n'ont pas de champ "roles" (-> liste vide après le parsing
    // ci-dessus). Pour ces champions déjà connus dans referenceChampions(),
    // on récupère leurs rôles par défaut une bonne fois pour toutes — sans
    // toucher aux champions ajoutés manuellement par le joueur, dont les
    // rôles (potentiellement laissés vides volontairement) sont respectés.
    // Le flag m_rolesMigrated évite de refaire ce backfill à chaque
    // lancement, ce qui écraserait un choix du joueur de tout décocher.
    if (!m_rolesMigrated) {
        backfillRoles();
        m_rolesMigrated = true;
        save();
    }
}

bool DataManager::exportTo(const QString& path) {
    // S'assure que le fichier sur disque reflète bien l'état actuel
    // avant la copie (au cas où une mutation récente n'aurait, en
    // théorie, pas encore déclenché de sauvegarde).
    save();

    if (path.isEmpty()) return false;
    // QFile::copy() échoue si le fichier de destination existe déjà
    // (ex. l'utilisateur réexporte par-dessus un ancien backup) : on le
    // supprime d'abord, sans quoi l'export silencieux échouerait.
    if (QFile::exists(path) && !QFile::remove(path)) {
        qWarning() << "DataManager::exportTo() : impossible de remplacer" << path;
        return false;
    }
    if (!QFile::copy(dataPath(), path)) {
        qWarning() << "DataManager::exportTo() : échec de la copie vers" << path;
        return false;
    }
    return true;
}

bool DataManager::importFrom(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "DataManager::importFrom() : impossible d'ouvrir" << path
                   << "-" << f.errorString();
        return false;
    }
    const QByteArray bytes = f.readAll();
    f.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "DataManager::importFrom() : JSON invalide -" << err.errorString();
        return false;
    }

    // Validation a minima : un fichier de sauvegarde LeagueTracker doit
    // au moins contenir ces clés (cf. les champs écrits par save()).
    // Ça évite d'importer silencieusement un .json sans rapport (et de
    // se retrouver avec une collection vide) en cas de mauvais fichier
    // choisi dans la boîte de dialogue.
    const QJsonObject root = doc.object();
    if (!root.contains("champions") || !root.contains("skins") || !root.contains("balises")) {
        qWarning() << "DataManager::importFrom() :" << path
                   << "ne ressemble pas à une sauvegarde LeagueTracker valide";
        return false;
    }

    if (QFile::exists(dataPath()) && !QFile::remove(dataPath())) {
        qWarning() << "DataManager::importFrom() : impossible de remplacer" << dataPath();
        return false;
    }
    if (!QFile::copy(path, dataPath())) {
        qWarning() << "DataManager::importFrom() : échec de la copie depuis" << path;
        return false;
    }

    // Relit depuis le fichier importé : recalcule m_champions/m_skins/...
    // en mémoire, fusionne les éventuels champions/skins/balises sortis
    // depuis que ce backup a été créé, et migre les rôles si besoin —
    // exactement comme un chargement normal au démarrage.
    load();
    emit dataChanged();
    return true;
}

void DataManager::save() {
    QJsonArray jChamps;
    for (const auto& c : m_champions) {
        QJsonObject o;
        o["nom"]          = c.nom;
        o["prixStandard"] = c.prixStandard;
        o["prixReduit"]   = c.prixReduit;
        o["possede"]      = c.possede;
        o["prioritaire"]  = c.prioritaire;
        o["roles"]        = QJsonArray::fromStringList(c.roles);
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

    QJsonArray jHistory;
    for (const auto& s : m_history) {
        QJsonObject o;
        o["date"] = s.date;
        o["eb"]   = s.eb;
        o["eo"]   = s.eo;
        jHistory << o;
    }

    QJsonObject root;
    root["essenceBleu"]    = m_eb;
    root["essenceOrange"]  = m_eo;
    root["rolesMigrated"]  = m_rolesMigrated;
    root["champions"]      = jChamps;
    root["skins"]          = jSkins;
    root["balises"]        = jBalises;
    root["historique"]     = jHistory;

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
    // Le 5e champ ("roles") est un masque de RoleFlag (cf. champion.h),
    // combinable avec | pour les champions jouables sur plusieurs lanes
    // (ex. Akali : Mid + Top). Contrairement au nom/prix, ce champ n'a
    // pas de source officielle (Data Dragon n'expose que des archétypes
    // génériques type Fighter/Mage, pas des lanes) : il est assigné à la
    // main à partir de la méta actuelle, et reste modifiable par champion
    // depuis l'app (cf. AddChampionDialog / ChampionDetailDialog).
    struct RawChamp { const char* nom; int ps; int pr; bool pos; int roles; };
    static const RawChamp raw[] = {
        {"Aatrox",         2400, 0,    true,  RoleTop},
        {"Ahri",           1575, 0,    true,  RoleMid},
        {"Akali",          1575, 945,  true,  RoleMid|RoleTop},
        {"Akshan",         2400, 0,    true,  RoleMid|RoleTop},
        {"Alistar",        675,  0,    true,  RoleSupport},
        {"Ambessa",        2400, 1440, true,  RoleTop|RoleJungle},
        {"Amumu",          225,  0,    true,  RoleJungle|RoleSupport},
        {"Anivia",         1575, 0,    true,  RoleMid},
        {"Annie",          225,  0,    true,  RoleMid|RoleSupport},
        {"Aphelios",       2400, 1440, true,  RoleADC},
        {"Ashe",           225,  0,    true,  RoleADC|RoleSupport},
        {"Aurelion Sol",   2400, 0,    true,  RoleMid},
        {"Aurora",         2400, 0,    true,  RoleMid|RoleTop},
        {"Azir",           2400, 0,    true,  RoleMid},
        {"Bard",           2400, 0,    true,  RoleSupport},
        {"Bel'Veth",       2400, 0,    true,  RoleJungle},
        {"Blitzcrank",     675,  0,    true,  RoleSupport|RoleJungle},
        {"Brand",          225,  0,    true,  RoleSupport|RoleMid},
        {"Braum",          1575, 0,    true,  RoleSupport},
        {"Briar",          2400, 0,    true,  RoleJungle},
        {"Caitlyn",        1575, 0,    true,  RoleADC},
        {"Camille",        2400, 0,    true,  RoleTop},
        {"Cassiopeia",     2400, 1440, true,  RoleMid},
        {"Cho'Gath",       675,  0,    true,  RoleTop|RoleJungle},
        {"Corki",          1575, 0,    false, RoleMid|RoleADC},
        {"Darius",         2400, 0,    true,  RoleTop},
        {"Diana",          2400, 0,    true,  RoleJungle|RoleMid},
        {"Dr. Mundo",      675,  0,    true,  RoleTop|RoleJungle},
        {"Draven",         2400, 1440, false, RoleADC},
        {"Ekko",           2400, 0,    true,  RoleMid|RoleJungle},
        {"Elise",          2400, 0,    true,  RoleJungle},
        {"Evelynn",        1575, 0,    true,  RoleJungle},
        {"Ezreal",         1575, 0,    true,  RoleADC|RoleMid},
        {"Fiddlesticks",   675,  0,    true,  RoleJungle|RoleSupport},
        {"Fiora",          2400, 0,    true,  RoleTop},
        {"Fizz",           675,  0,    true,  RoleMid},
        {"Galio",          2400, 0,    true,  RoleMid|RoleSupport},
        {"Gangplank",      1575, 0,    true,  RoleTop},
        {"Garen",          225,  0,    true,  RoleTop},
        {"Gnar",           2400, 0,    true,  RoleTop},
        {"Gragas",         1575, 0,    true,  RoleJungle|RoleTop|RoleSupport},
        {"Graves",         2400, 0,    false, RoleJungle},
        {"Gwen",           2400, 0,    true,  RoleTop|RoleJungle},
        {"Hecarim",        2400, 0,    true,  RoleJungle},
        {"Heimerdinger",   1575, 945,  false, RoleMid|RoleTop|RoleSupport},
        {"Hwei",           2400, 0,    true,  RoleMid|RoleSupport},
        {"Illaoi",         1575, 0,    false, RoleTop},
        {"Irelia",         1575, 0,    true,  RoleTop|RoleMid},
        {"Ivern",          2400, 0,    true,  RoleJungle},
        {"Janna",          675,  0,    true,  RoleSupport},
        {"Jarvan IV",      1575, 0,    true,  RoleJungle|RoleTop},
        {"Jax",            1575, 0,    true,  RoleTop|RoleJungle},
        {"Jayce",          2400, 0,    true,  RoleTop|RoleMid},
        {"Jhin",           2400, 0,    true,  RoleADC},
        {"Jinx",           2400, 0,    true,  RoleADC},
        {"K'Sante",        2400, 0,    true,  RoleTop},
        {"Kai'Sa",         2400, 0,    true,  RoleADC},
        {"Kalista",        2400, 0,    false, RoleADC},
        {"Karma",          1575, 0,    true,  RoleSupport|RoleMid},
        {"Karthus",        1575, 0,    false, RoleJungle|RoleMid},
        {"Kassadin",       1575, 945,  true,  RoleMid},
        {"Katarina",       1575, 0,    true,  RoleMid},
        {"Kayle",          2400, 0,    true,  RoleTop|RoleMid},
        {"Kayn",           2400, 0,    true,  RoleJungle},
        {"Kennen",         2400, 1440, true,  RoleTop|RoleMid},
        {"Kha'Zix",        2400, 1440, true,  RoleJungle},
        {"Kindred",        2400, 0,    true,  RoleJungle|RoleADC},
        {"Kled",           2400, 0,    false, RoleTop},
        {"LeBlanc",        1575, 0,    true,  RoleMid},
        {"Kog'Maw",        2400, 0,    false, RoleADC|RoleSupport},
        {"Lee Sin",        675,  0,    true,  RoleJungle},
        {"Leona",          225,  135,  true,  RoleSupport},
        {"Lillia",         2400, 0,    true,  RoleJungle|RoleTop},
        {"Lissandra",      2400, 0,    true,  RoleMid|RoleSupport},
        {"Lucian",         2400, 0,    true,  RoleADC|RoleMid},
        {"Lulu",           2400, 0,    true,  RoleSupport},
        {"Lux",            2400, 0,    true,  RoleSupport|RoleMid},
        {"Maitre Yi",      225,  0,    true,  RoleJungle},
        {"Malphite",       675,  0,    true,  RoleTop|RoleJungle},
        {"Malzahar",       2400, 0,    true,  RoleMid|RoleSupport},
        {"Maokai",         1575, 0,    true,  RoleSupport|RoleTop|RoleJungle},
        {"Mel",            2400, 0,    true,  RoleMid|RoleSupport},
        {"Milio",          2400, 0,    true,  RoleSupport},
        {"Miss Fortune",   2400, 0,    true,  RoleADC},
        {"Mordekaiser",    2400, 0,    true,  RoleTop},
        {"Morgana",        675,  0,    true,  RoleSupport|RoleMid},
        {"Naafiri",        2400, 0,    true,  RoleMid|RoleJungle},
        {"Nami",           1575, 0,    true,  RoleSupport},
        {"Nasus",          675,  0,    true,  RoleTop},
        {"Nautilus",       2400, 0,    true,  RoleSupport|RoleJungle},
        {"Neeko",          2400, 0,    true,  RoleSupport|RoleMid},
        {"Nidalee",        1575, 0,    true,  RoleJungle},
        {"Nilah",          2400, 0,    true,  RoleADC},
        {"Nocturne",       1575, 0,    true,  RoleJungle},
        {"Nunu & Willump", 225,  0,    true,  RoleJungle},
        {"Olaf",           1575, 0,    true,  RoleJungle|RoleTop},
        {"Orianna",        2400, 1440, true,  RoleMid},
        {"Ornn",           2400, 0,    true,  RoleTop},
        {"Pantheon",       2400, 0,    true,  RoleTop|RoleSupport|RoleJungle},
        {"Poppy",          2400, 0,    true,  RoleTop|RoleJungle|RoleSupport},
        {"Pyke",           2400, 0,    true,  RoleSupport},
        {"Qiyana",         2400, 0,    true,  RoleMid|RoleJungle},
        {"Quinn",          2400, 0,    true,  RoleTop},
        {"Rakan",          2400, 0,    true,  RoleSupport},
        {"Rammus",         675,  0,    true,  RoleJungle},
        {"Rek'Sai",        2400, 0,    false, RoleJungle},
        {"Rell",           2400, 0,    true,  RoleSupport},
        {"Renata Glasc",   2400, 0,    true,  RoleSupport},
        {"Renekton",       2400, 0,    true,  RoleTop|RoleJungle},
        {"Rengar",         2400, 0,    true,  RoleJungle|RoleTop},
        {"Riven",          2400, 0,    true,  RoleTop},
        {"Rumble",         2400, 1440, false, RoleTop|RoleJungle},
        {"Ryze",           675,  0,    true,  RoleMid|RoleTop},
        {"Samira",         2400, 0,    true,  RoleADC},
        {"Sejuani",        225,  0,    true,  RoleJungle},
        {"Senna",          2400, 0,    true,  RoleSupport|RoleADC},
        {"Seraphine",      2400, 0,    true,  RoleSupport|RoleADC|RoleMid},
        {"Sett",           2400, 0,    true,  RoleTop|RoleSupport},
        {"Shaco",          1575, 0,    true,  RoleJungle},
        {"Shen",           1575, 0,    true,  RoleTop|RoleSupport},
        {"Shyvana",        1575, 0,    true,  RoleJungle},
        {"Singed",         225,  0,    true,  RoleTop},
        {"Sion",           1575, 0,    true,  RoleTop},
        {"Sivir",          225,  0,    false, RoleADC},
        {"Skarner",        2400, 0,    true,  RoleJungle},
        {"Smolder",        2400, 0,    true,  RoleADC},
        {"Sona",           675,  0,    true,  RoleSupport},
        {"Soraka",         225,  0,    true,  RoleSupport},
        {"Swain",          1575, 0,    true,  RoleSupport|RoleMid},
        {"Sylas",          2400, 0,    true,  RoleMid|RoleJungle},
        {"Syndra",         1575, 0,    true,  RoleMid},
        {"Tahm Kench",     2400, 0,    true,  RoleSupport|RoleTop},
        {"Taliyah",        2400, 0,    false, RoleJungle|RoleMid},
        {"Talon",          2400, 0,    true,  RoleJungle|RoleMid},
        {"Taric",          675,  0,    true,  RoleSupport},
        {"Teemo",          675,  0,    true,  RoleTop},
        {"Thresh",         2400, 0,    true,  RoleSupport},
        {"Tristana",       675,  0,    true,  RoleADC|RoleMid},
        {"Trundle",        1575, 0,    true,  RoleTop|RoleJungle},
        {"Tryndamere",     1575, 0,    true,  RoleTop},
        {"Twisted Fate",   675,  0,    true,  RoleMid},
        {"Twitch",         1575, 0,    true,  RoleADC},
        {"Udyr",           2400, 0,    true,  RoleJungle},
        {"Urgot",          2400, 0,    true,  RoleTop},
        {"Varus",          2400, 0,    false, RoleADC|RoleMid},
        {"Vayne",          2400, 0,    true,  RoleADC|RoleTop},
        {"Veigar",         675,  0,    true,  RoleMid|RoleSupport},
        {"Vel'Koz",        1618, 0,    true,  RoleMid|RoleSupport},
        {"Vex",            2400, 0,    true,  RoleMid},
        {"Vi",             2400, 0,    true,  RoleJungle},
        {"Viego",          2400, 0,    true,  RoleJungle},
        {"Viktor",         2400, 0,    true,  RoleMid},
        {"Vladimir",       2400, 0,    true,  RoleMid|RoleTop},
        {"Volibear",       675,  405,  true,  RoleTop|RoleJungle},
        {"Warwick",        225,  0,    true,  RoleJungle|RoleTop},
        {"Wukong",         1575, 0,    true,  RoleTop|RoleJungle},
        {"Xayah",          2400, 0,    false, RoleADC},
        {"Xerath",         2400, 0,    true,  RoleMid|RoleSupport},
        {"Xin Zhao",       675,  0,    true,  RoleJungle},
        {"Yasuo",          2400, 0,    true,  RoleMid|RoleTop},
        {"Yone",           2400, 0,    true,  RoleMid|RoleTop},
        {"Yorick",         1575, 0,    true,  RoleTop},
        {"Yunara",         2400, 0,    true,  RoleADC},
        {"Yuumi",          2400, 0,    true,  RoleSupport},
        {"Zaahen",         2400, 0,    true,  RoleTop|RoleJungle},
        {"Zac",            2400, 0,    true,  RoleJungle},
        {"Zed",            2400, 0,    true,  RoleMid|RoleJungle},
        {"Zeri",           2400, 0,    true,  RoleADC},
        {"Ziggs",          2400, 1440, true,  RoleMid|RoleADC},
        {"Zilean",         675,  0,    true,  RoleSupport|RoleMid},
        {"Zoe",            2400, 1440, true,  RoleMid|RoleSupport},
        {"Zyra",           1575, 0,    false, RoleSupport|RoleJungle},
        // 👉 Nouveau champion sorti sur LoL ? Ajoute une ligne ici, au
        //    format {"Nom", prixStandard, prixReduit (0 si aucun), possede,
        //    rôle(s) au format RoleX ou RoleX|RoleY}. Au prochain
        //    lancement, il sera ajouté automatiquement à ta sauvegarde
        //    existante (cf. DataManager::mergeNewChampions()).
    };
    QVector<Champion> result;
    result.reserve(sizeof(raw) / sizeof(raw[0]));
    for (const auto& r : raw) {
        Champion c;
        c.nom          = QString::fromUtf8(r.nom);
        c.prixStandard = r.ps;
        c.prixReduit   = r.pr;
        c.possede      = r.pos;
        c.roles        = rolesFromMask(r.roles);
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