#include "appcontroller.h"
#include "../model/datamanager.h"

AppController::AppController(QObject* parent)
    : QObject(parent), m_model(DataManager::instance())
{
    m_model->load();
    connect(m_model, &DataManager::dataChanged, this, &AppController::dataChanged);
}

const QVector<Champion>& AppController::champions() const { return m_model->champions(); }
const QVector<Skin>&     AppController::skins()     const { return m_model->skins(); }
const QVector<Balise>&   AppController::balises()   const { return m_model->balises(); }

int AppController::essenceBleu()   const { return m_model->essenceBleu(); }
int AppController::essenceOrange() const { return m_model->essenceOrange(); }

int AppController::totalChampions()   const { return m_model->totalChampions(); }
int AppController::champsOwned()      const { return m_model->champsOwned(); }
int AppController::champsToBuy()      const { return m_model->champsToBuy(); }
int AppController::coutTotalRestant() const { return m_model->coutTotalRestant(); }
int AppController::ebApresAchat()     const { return m_model->ebApresAchat(); }

QVector<int> AppController::filteredChampionIndices(const ChampionFilter& filter) const {
    QVector<int> result;
    const QString txt = filter.search.trimmed().toLower();
    const auto& champs = m_model->champions();

    for (int i = 0; i < champs.size(); ++i) {
        const Champion& c = champs[i];
        bool nameOk = txt.isEmpty() || c.nom.toLower().contains(txt);
        bool filtOk = true;
        switch (filter.mode) {
        case 1: filtOk = c.possede;  break;
        case 2: filtOk = !c.possede; break;
        case 3: filtOk = c.prixEffectif() <= 675;  break;
        case 4: filtOk = c.prixEffectif() == 1575; break;
        case 5: filtOk = c.prixEffectif() >= 2400; break;
        case 6: filtOk = c.prixReduit > 0;         break;
        default: break;
        }
        if (nameOk && filtOk) result << i;
    }
    return result;
}

bool AppController::isChampionOwned(const QString& championName) const {
    for (const auto& c : m_model->champions())
        if (c.nom == championName) return c.possede;
    return false;
}

bool AppController::canBuySkin(const Skin& s) const {
    if (s.possede) return false;
    if (!isChampionOwned(s.champion)) return false;
    return s.gratuit || essenceOrange() >= s.prix;
}

bool AppController::canBuyBalise(const Balise& b) const {
    if (b.possede) return false;
    return b.prix == 0 || essenceOrange() >= b.prix;
}

QVector<int> AppController::skinIndicesForChampion(const QString& championName) const {
    QVector<int> result;
    const auto& skinList = m_model->skins();
    for (int i = 0; i < skinList.size(); ++i)
        if (skinList[i].champion == championName) result << i;
    return result;
}

void AppController::setEssenceBleu(int v)   { m_model->setEssenceBleu(v); }
void AppController::setEssenceOrange(int v) { m_model->setEssenceOrange(v); }
void AppController::updateChampion(const Champion& c) { m_model->updateChampion(c); }
void AppController::setBaliseOwned(int index, bool owned) { m_model->setBaliseOwned(index, owned); }
void AppController::setSkinOwned(int index, bool owned)   { m_model->setSkinOwned(index, owned); }
bool AppController::addChampion(const Champion& c) { return m_model->addChampion(c); }
bool AppController::addSkin(const Skin& s)         { return m_model->addSkin(s); }
bool AppController::addBalise(const Balise& b)     { return m_model->addBalise(b); }

void AppController::removeChampion(int index) { m_model->removeChampion(index); }
void AppController::removeSkin(int index)     { m_model->removeSkin(index); }
void AppController::removeBalise(int index)   { m_model->removeBalise(index); }
