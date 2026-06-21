# League Tracker — Application Qt (architecture MVC)

Suivi de ta collection League of Legends (champions, skins, balises).

## Architecture MVC

Le projet est organisé en trois couches clairement séparées :

```
LeagueTracker/
├── main.cpp                 → point d'entrée (crée le Contrôleur, l'injecte dans la fenêtre)
├── model/                    → MODÈLE
│   ├── champion.h / skin.h / balise.h   (structures de données)
│   └── datamanager.h/.cpp   (données + persistance JSON + calculs purs)
├── controller/               → CONTRÔLEUR
│   └── appcontroller.h/.cpp (filtrage, règles d'achat, actions des Vues)
└── view/                      → VUES
    ├── mainwindow.h/.cpp
    ├── championgridpage.h/.cpp
    ├── championcard.h/.cpp
    ├── championdetaildialog.h/.cpp
    ├── skinpage.h/.cpp
    ├── balisepage.h/.cpp
    └── statswidget.h/.cpp
```

### Rôle de chaque couche

- **Modèle (`model/DataManager`)** : possède les données (champions, skins,
  balises, essences), assure leur sauvegarde/chargement en JSON et expose
  les calculs « bruts » liés aux données (`champsOwned()`,
  `coutTotalRestant()`, `ebApresAchat()`...). Toute mutation passe par une
  méthode du Modèle (`updateChampion`, `setBaliseOwned`,
  `setEssenceBleu`...) qui sauvegarde puis émet `dataChanged()`.

- **Contrôleur (`controller/AppController`)** : seul point de passage entre
  les Vues et le Modèle. Il relaie les lectures (`champions()`, `skins()`,
  `balises()`, statistiques...), porte la logique « applicative » qui n'est
  pas une donnée brute (filtrage de la grille via
  `filteredChampionIndices()`, règles d'achat `canBuySkin()` /
  `canBuyBalise()`), et transmet les actions utilisateur au Modèle
  (`updateChampion`, `setBaliseOwned`, `setEssenceBleu/Orange`).

- **Vues (`view/...`)** : uniquement de l'affichage et de la capture
  d'événements utilisateur (clics, saisie...). Aucune Vue n'accède
  directement à `DataManager` : tout passe par le `AppController*` reçu en
  constructeur. Les Vues se rafraîchissent en s'abonnant au signal
  `AppController::dataChanged`.

Flux typique (ex. cocher une balise comme possédée) :

```
BalisePage (Vue)
   → m_controller->setBaliseOwned(row, checked)   [Contrôleur]
   → DataManager::setBaliseOwned(...)             [Modèle : mutation + save() + emit dataChanged()]
   → AppController::dataChanged                   [relayé par le Contrôleur]
   → BalisePage::refresh() (+ StatsWidget, etc.)   [toutes les Vues abonnées se mettent à jour]
```

## Ajouter de nouveaux champions / skins / balises (sorties LoL)

Deux façons d'ajouter une nouvelle entrée à ta collection :

1. **Mise à jour du code (recommandé pour les sorties officielles)** :
   ouvre `model/datamanager.cpp` et ajoute une ligne dans la liste de
   référence concernée :
   - `DataManager::referenceChampions()` → `{"Nom", prixStandard, prixReduit, possede}`
   - `DataManager::referenceSkins()` → `{"Nom du skin", "Champion", prix, gratuit, "Rareté"}`
   - `DataManager::referenceBalises()` → `{"Nom de la balise", prix, possede}`

   Un commentaire indique où ajouter la ligne dans chaque liste.
   Recompile : au prochain lancement, `mergeNewChampions()` /
   `mergeNewSkins()` / `mergeNewBalises()` ajoutent automatiquement les
   entrées manquantes à ta sauvegarde existante **sans toucher** à tes
   données déjà enregistrées (essences, possessions, balises cochées...).

2. **Boutons « ✚ Nouveau... » (ajout à la volée, sans recompiler)** :
   - Onglet ⚔ Champions → « ✚ Nouveau champion » (nom, prix, possédé)
   - Onglet ✨ Skins → « ✚ Nouveau skin » (nom, champion, rareté, prix/gratuit)
   - Onglet 🚩 Balises → « ✚ Nouvelle balise » (nom, prix/gratuite, possédée)

   Dans les trois cas, l'élément est immédiatement ajouté et sauvegardé.
   Un nom déjà existant (insensible à la casse) est refusé.

## Supprimer un champion / skin / balise

- **Champion** : clique sur sa carte pour ouvrir la fenêtre de détails,
  puis clique sur « 🗑 Supprimer » (en bas à gauche). Une confirmation
  est demandée avant la suppression définitive.
- **Skin / Balise** : chaque ligne du tableau a un bouton « 🗑 » à
  droite. Une confirmation est demandée avant la suppression.

Dans les trois cas, la suppression appelle `removeChampion`/`removeSkin`/
`removeBalise` du Contrôleur → Modèle, qui retire l'entrée, sauvegarde et
notifie les Vues (`dataChanged`).

## Gérer les skins d'un champion depuis sa fiche

En cliquant sur la carte d'un champion, la fenêtre de détails affiche :

- son **statut** (possédé ou non), son **prix standard**, son **prix
  réduit** et le **prix effectif** calculé automatiquement ;
- la liste de ses **skins**, chacun avec une case « Possédé » que tu
  peux cocher/décocher à tout moment — le changement est appliqué et
  sauvegardé immédiatement (comme pour les balises), indépendamment
  du bouton « Enregistrer » ;
- un bouton « ✚ Ajouter un skin » qui ouvre le même formulaire que
  dans l'onglet ✨ Skins, avec ce champion pré-sélectionné.

Le champ `Skin::possede` est désormais pris en compte dans l'onglet
✨ Skins (colonne « Possédé », cochable aussi depuis là) et dans le
calcul d'« Achetable » (`AppController::canBuySkin`) : un skin déjà
possédé n'est plus proposé comme achetable.

## Compilation

```bash
qmake LeagueTracker.pro
make
```
Ou ouvrir `LeagueTracker.pro` dans Qt Creator → Run.

## Ajouter des images de champions

Crée un dossier `images/` **à côté de l'exécutable** et place les portraits dedans.

**Noms de fichiers acceptés** (png, jpg ou webp) :
- `Ahri.png`
- `LeeSin.png`  (camelCase, sans espaces)
- `lee_sin.png` (underscore lowercase)

### Téléchargement automatique depuis Data Dragon (optionnel)

```bash
# Récupère tous les portraits officiels LoL
VERSION="15.6.1"
mkdir -p images
while read nom; do
  curl -s "https://ddragon.leagueoflegends.com/cdn/$VERSION/img/champion/${nom}.png" \
       -o "images/${nom}.png"
done << CHAMPS
Aatrox
Ahri
Akali
... (liste complète sur https://ddragon.leagueoflegends.com/cdn/15.6.1/data/fr_FR/champion.json)
CHAMPS
```

## Données

Les données sont sauvegardées automatiquement (par le Modèle) dans :
- **Windows** : `%APPDATA%/LeagueTracker/lol_data.json`
- **Linux**   : `~/.local/share/LeagueTracker/lol_data.json`
- **macOS**   : `~/Library/Application Support/LeagueTracker/lol_data.json`

## Fonctionnalités

| Onglet | Ce que tu peux faire |
|---|---|
| ⚔ Champions | Voir toute ta collection en grille, rechercher, filtrer, cliquer pour modifier |
| ✨ Skins | Voir les skins achetables avec ton EO actuel |
| 🚩 Balises | Cocher les balises possédées, voir celles achetables |
| 📊 Stats | Vue d'ensemble : EB manquant, progression, listes |
