# League Tracker — Application Qt (architecture MVC)

Suivi de ta collection League of Legends (champions, skins, balises) : prix, possession, budget Essence Bleue / Essence Orange, et ce qui est achetable maintenant.

## Sommaire

- [Prérequis](#prérequis)
- [Compilation](#compilation)
- [Architecture MVC](#architecture-mvc)
- [Onglet ⚔ Champions](#onglet--champions)
- [Onglet ✨ Skins](#onglet--skins)
- [Onglet 🚩 Balises](#onglet--balises)
- [Onglet 📊 Stats](#onglet--stats)
- [Ajouter de nouveaux champions / skins / balises](#ajouter-de-nouveaux-champions--skins--balises)
- [Supprimer un champion / skin / balise](#supprimer-un-champion--skin--balise)
- [Gérer les skins d'un champion depuis sa fiche](#gérer-les-skins-dun-champion-depuis-sa-fiche)
- [Apparence et icône de l'application](#apparence-et-icône-de-lapplication)
- [Quitter l'application](#quitter-lapplication)
- [Confort : raccourcis clavier et notifications](#confort--raccourcis-clavier-et-notifications)
- [Ajouter des images de champions](#ajouter-des-images-de-champions)
- [Données et sauvegarde](#données-et-sauvegarde)
- [Notes techniques](#notes-techniques)

## Prérequis

- **Qt 6.x** avec les modules `core`, `gui`, `widgets`, `network` (le
  `.pro` demande aussi `charts`, voir [Notes techniques](#notes-techniques))
- Un compilateur **C++17**
- Qt Creator (recommandé) ou `qmake` + `make` en ligne de commande

> ⚠️ Le projet n'est **pas compatible Qt 5** : `ChampionCard::enterEvent` utilise `QEnterEvent`, une API introduite avec Qt 6.

## Compilation

```bash
qmake LeagueTracker.pro
make
```
Ou ouvrir `LeagueTracker.pro` dans Qt Creator → Run.

## Architecture MVC

Le projet est organisé en trois couches clairement séparées :

```
LeagueTracker/
├── LeagueTracker.pro
├── main.cpp                   → point d'entrée (thème, icône, crée le Contrôleur, l'injecte dans la fenêtre)
├── resources.qrc              → ressources embarquées (icône de l'app)
├── icone/
│   ├── logo_lt.png
│   └── logo_lt.ico
├── model/                      → MODÈLE
│   ├── champion.h / skin.h / balise.h   (structures de données)
│   └── datamanager.h/.cpp     (données + persistance JSON + calculs purs)
├── controller/                 → CONTRÔLEUR
│   └── appcontroller.h/.cpp   (filtrage, règles d'achat, actions des Vues)
└── view/                        → VUES
    ├── mainwindow.h/.cpp
    ├── championgridpage.h/.cpp
    ├── championcard.h/.cpp
    ├── championdetaildialog.h/.cpp
    ├── addchampiondialog.h/.cpp
    ├── skinpage.h/.cpp
    ├── addskindialog.h/.cpp
    ├── balisepage.h/.cpp
    ├── addbalisedialog.h/.cpp
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

## Onglet ⚔ Champions

Grille de cartes (image + nom + statut), avec :

- une **barre de recherche** par nom ;
- un **filtre** parmi 7 modes : Tous les champions, Possédés, Non possédés,
  Pas cher (≤ 675), Moyen (1575), Cher (2400+), Avec réduction ;
- un compteur du nombre de champions actuellement affichés ;
- un clic sur une carte ouvre la fiche détaillée du champion (prix,
  possession, et ses skins — voir [plus bas](#gérer-les-skins-dun-champion-depuis-sa-fiche)).

## Onglet ✨ Skins

Tableau listant tous les skins, avec :

- **tri multi-critères** : Nom, Champion, Rareté, Prix, ou Possédé (en
  premier), avec un bouton pour inverser l'ordre (↑ / ↓) ;
- une hiérarchie de **7 raretés** + Gratuit, chacune avec sa couleur :

  | Rareté | Couleur |
  |---|---|
  | Basique | doré (défaut) |
  | Epique | violet |
  | Legendaire | orange |
  | Fantastique | rose |
  | Ultime | rouge |
  | Exalte | jaune doré |
  | Transcendant | cyan |
  | Gratuit | vert |

- une colonne **Achetable** qui croise le statut du champion associé et
  ton EO actuel (« Déjà possédé », « Champ manquant », « Manque X EO »,
  ou « Oui ») ;
- une case **Possédé** cochable directement dans le tableau (sauvegardée
  immédiatement) ;
- un bouton « ✚ Nouveau skin » et un bouton de suppression par ligne.

## Onglet 🚩 Balises

Tableau listant toutes les balises (wards), avec :

- **tri multi-critères** : Nom (A → Z), Prix, Possédée en premier, ou Non
  possédée en premier, avec inversion d'ordre (↑ / ↓) ;
- une case **Possédée** cochable par ligne (sauvegardée immédiatement) ;
- une colonne **Achetable** basée sur ton EO actuel ;
- un bouton « ✚ Nouvelle balise » et un bouton de suppression par ligne.

## Onglet 📊 Stats

Vue d'ensemble avec :

- trois cartes : **Champions** (total / possédés / à acheter + barre de
  progression), **Essence Bleue** (coût total restant, ton EB, EB après
  tout achat), **Essence Orange** (EO disponible) ;
- la liste des **champions à acheter**, avec leur prix effectif et une
  coche ✔ si tu as déjà assez d'EB pour celui-ci ;
- la liste des **champions avec réduction active** (prix standard → prix
  réduit, avec le montant économisé).

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
   - Onglet ⚔ Champions → « ✚ Nouveau champion » (nom, prix standard, prix réduit, possédé)
   - Onglet ✨ Skins → « ✚ Nouveau skin » (nom, champion, rareté, prix/gratuit, possédé)
   - Onglet 🚩 Balises → « ✚ Nouvelle balise » (nom, prix/gratuite, possédée)

   Dans les trois cas, l'élément est immédiatement ajouté et sauvegardé.
   Un nom déjà existant (insensible à la casse) est refusé.

## Supprimer un champion / skin / balise

- **Champion** : clique sur sa carte pour ouvrir la fenêtre de détails,
  puis clique sur « 🗑 Supprimer » (en bas à gauche). Une confirmation
  est demandée avant la suppression définitive (action plus lourde
  qu'une simple ligne de tableau).
- **Skin / Balise** : chaque ligne du tableau a un bouton « 🗑 » à
  droite. La suppression est **immédiate**, sans boîte de confirmation
  bloquante — une notification discrète apparaît en bas à droite avec
  un bouton « Annuler » pendant quelques secondes si tu t'es trompé.

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

Le champ `Skin::possede` est pris en compte dans l'onglet ✨ Skins
(colonne « Possédé », cochable aussi depuis là) et dans le calcul
d'« Achetable » (`AppController::canBuySkin`) : un skin déjà possédé
n'est plus proposé comme achetable.

## Apparence et icône de l'application

- Thème sombre doré/LoL appliqué via des feuilles de style Qt (QSS) sur
  chaque page.
- `main.cpp` force le style **Fusion** + une **QPalette** sombre
  personnalisée *avant* la création de la fenêtre principale : ce fix
  évite le flash blanc qui apparaissait sinon brièvement au démarrage
  sur Windows.
- L'icône de l'application (`icone/logo_lt.png`, embarquée via
  `resources.qrc`) est appliquée à la fenêtre **et** à la barre des
  tâches via `app.setWindowIcon(...)`.

## Quitter l'application

Un bouton « ⏻ Quitter » est présent en bas de la barre latérale, sous le
bloc Essence. Il demande une confirmation avant de fermer l'application,
pour éviter une fermeture accidentelle.

## Confort : raccourcis clavier et notifications

- **Ctrl+F** (Cmd+F sur macOS) donne le focus à la barre de recherche de
  l'onglet actif (Champions / Skins / Balises). Géré par un seul
  `QShortcut` global dans `MainWindow`, qui délègue à la page courante
  via `focusSearch()`.
- **Échap** ferme la boîte de dialogue ouverte (ajout/édition/suppression).
  C'est le comportement natif de `QDialog` en Qt — aucun code spécifique
  n'a été nécessaire, du moment qu'aucune vue ne capte la touche avant lui.
- Les actions rapides et réversibles (cocher une case « Possédé », supprimer
  une ligne de skin/balise) n'affichent plus de `QMessageBox` bloquante :
  une petite notification (« toast », `view/toast.h`) apparaît en bas à
  droite de la fenêtre pendant quelques secondes, avec un bouton
  « Annuler » pour les suppressions. Un seul toast est visible à la fois
  (un nouveau remplace l'ancien) pour rester discret même en cas de clics
  rapprochés. La suppression d'un **champion** garde sa confirmation
  classique : c'est une action plus engageante qu'une ligne de tableau.

## Ajouter des images de champions

Crée un dossier `images/` **à côté de l'exécutable** et place les portraits dedans.

**Noms de fichiers acceptés** (png, jpg ou webp) :
- `Ahri.png`
- `LeeSin.png`  (camelCase, sans espaces)
- `lee_sin.png` (underscore lowercase)

### Téléchargement automatique depuis l'application (recommandé)

Dans l'onglet ⚔ Champions, le bouton « ⬇ Télécharger les images
manquantes » (à côté de « ✚ Nouveau champion ») :

1. récupère la dernière version disponible de Data Dragon ;
2. récupère la liste des champions (`fr_FR`) pour faire correspondre le
   nom français utilisé par l'app à l'identifiant Data Dragon — utile
   pour les champions dont l'id diffère du nom affiché (ex. *Wukong* →
   `MonkeyKing`, *Kai'Sa* → `Kaisa`) ;
3. télécharge uniquement les portraits des champions qui n'ont **pas**
   déjà une image locale dans `images/` (téléchargement séquentiel, avec
   barre de progression et bouton Annuler) ;
4. enregistre chaque image sous un nom que `ChampionCard::loadImage()`
   saura reconnaître immédiatement (cache d'images invalidé en fin de
   téléchargement, pas besoin de relancer l'application).

Le script curl manuel ci-dessous reste utile pour un téléchargement en
masse hors de l'application, ou en environnement sans interface graphique.

### Téléchargement manuel via curl (optionnel)

```bash
# Récupère la dernière version disponible de Data Dragon
VERSION=$(curl -s "https://ddragon.leagueoflegends.com/api/versions.json" | python3 -c "import sys,json;print(json.load(sys.stdin)[0])")
mkdir -p images
while read nom; do
  curl -s "https://ddragon.leagueoflegends.com/cdn/$VERSION/img/champion/${nom}.png" \
       -o "images/${nom}.png"
done << CHAMPS
Aatrox
Ahri
Akali
... (liste complète sur https://ddragon.leagueoflegends.com/cdn/$VERSION/data/fr_FR/champion.json)
CHAMPS
```

## Données et sauvegarde

Les données sont sauvegardées automatiquement (par le Modèle) dans un
fichier `lol_data.json`, dans un dossier propre à l'application géré par
Qt (`QStandardPaths::AppDataLocation`). Comme le nom d'organisation et le
nom d'application sont tous les deux `"LeagueTracker"`, Qt crée un dossier
**en double** :

- **Windows** : `%APPDATA%\LeagueTracker\LeagueTracker\lol_data.json`
- **Linux**   : `~/.local/share/LeagueTracker/LeagueTracker/lol_data.json`
- **macOS**   : `~/Library/Application Support/LeagueTracker/LeagueTracker/lol_data.json`

> 💡 Pour éviter ce dossier dupliqué, tu peux laisser
> `QCoreApplication::setOrganizationName(...)` vide dans `main.cpp`, ou lui
> donner un nom différent de l'application (ex. un pseudo ou nom d'équipe).

## Notes techniques

- Le fichier `.pro` déclare `QT += core gui widgets charts`, mais aucune
  classe `QtCharts` n'est utilisée dans le code actuel (les graphiques de
  l'onglet Stats sont en fait des `QProgressBar`/`QLabel`). Si tu n'as pas
  prévu d'utiliser de vrais graphiques bientôt, tu peux retirer `charts`
  du `.pro` pour ne pas dépendre de cet addon Qt chez quelqu'un qui
  clonerait le repo. Sinon, c'est un bon point de départ si tu veux
  ajouter de vraies courbes (ex. évolution de ton EB/EO dans le temps).
- Le projet nécessite Qt 6 (voir [Prérequis](#prérequis)).
- `QT += network` a été ajouté au `.pro` pour le téléchargement d'images
  Data Dragon (`view/imagedownloaddialog.h/.cpp`), via `QNetworkAccessManager`.