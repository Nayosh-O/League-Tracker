# League Tracker — Application Qt (architecture MVC)

<img src="https://flagcdn.com/gb.svg" width="20"/> [Read in English](README.en.md)

Suivi de ta collection League of Legends (champions, skins, balises) : prix, possession, budget Essence Bleue / Essence Orange, et ce qui est achetable maintenant.

## ⬇️ Téléchargement

> **Utilisateur Windows ? Pas besoin d'installer Qt ni de compiler.**
> Télécharge la dernière release, dézippe et lance `LeagueTracker.exe`.

[![Télécharger la dernière release](https://img.shields.io/github/v/release/Nayosh-O/League-Tracker?label=Télécharger&style=for-the-badge)](https://github.com/Nayosh-O/League-Tracker/releases/latest)

![Plateforme](https://img.shields.io/badge/plateforme-Windows%2010%2F11-blue?style=flat-square)
![Qt](https://img.shields.io/badge/Qt-6.x-green?style=flat-square)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)

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
- [Exporter / importer ta sauvegarde](#exporter--importer-ta-sauvegarde)
- [Apparence et icône de l'application](#apparence-et-icône-de-lapplication)
- [Quitter l'application](#quitter-lapplication)
- [Confort : raccourcis clavier et notifications](#confort--raccourcis-clavier-et-notifications)
- [Ajouter des images de champions](#ajouter-des-images-de-champions)
- [Données et sauvegarde](#données-et-sauvegarde)
- [Notes techniques](#notes-techniques)
- [Mentions légales](#mentions-légales)

## Prérequis

- **Qt 6.x** avec les modules `core`, `gui`, `widgets`, `charts`, `network`
  (voir [Notes techniques](#notes-techniques) pour le détail de leur usage)
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
    ├── statswidget.h/.cpp
    ├── toast.h/.cpp                (notification flottante "toast", non bloquante)
    └── imagedownloaddialog.h/.cpp  (téléchargement auto des portraits Data Dragon)
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
- un **filtre par rôle** (Top, Jungle, Mid, ADC, Support) — un champion peut
  cumuler plusieurs rôles (cf. [Notes techniques](#notes-techniques)) ;
- une **barre de tri** (Nom, Prix effectif, Possédé en premier, Non possédé
  en premier) avec inversion d'ordre (↑ / ↓) ;
- un compteur du nombre de champions actuellement affichés ;
- un clic sur une carte ouvre la fiche détaillée du champion (statut,
  prix, priorité d'achat, rôles, et ses skins — voir [plus bas](#gérer-les-skins-dun-champion-depuis-sa-fiche)).

## Onglet ✨ Skins

Tableau listant tous les skins, avec :

- **tri multi-critères** : Nom, Champion, Rareté, Prix, ou Possédé (en
  premier), via le combo dédié **ou** en cliquant directement sur un
  en-tête de colonne, avec un bouton pour inverser l'ordre (↑ / ↓) ;
- des **filtres combinés** (cumulables) : rareté, champion possédé/non
  possédé, skin possédé/non possédé — un bouton « ✕ Réinitialiser les
  filtres » apparaît dès qu'au moins un filtre est actif ;
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
- un bouton « ✚ Nouveau skin » (avec auto-complétion best-effort du nom,
  de la rareté et du prix EO via Community Dragon une fois un champion
  reconnu sélectionné — cf. `view/addskindialog.h`) et un bouton de
  suppression par ligne.

## Onglet 🚩 Balises

Tableau listant toutes les balises (wards), avec :

- **tri multi-critères** : Nom (A → Z), Prix, Possédée en premier, ou Non
  possédée en premier, via le combo dédié **ou** en cliquant directement
  sur un en-tête de colonne, avec inversion d'ordre (↑ / ↓) ;
- une case **Possédée** cochable par ligne (sauvegardée immédiatement) ;
- une colonne **Achetable** basée sur ton EO actuel ;
- un bouton « ✚ Nouvelle balise » et un bouton de suppression par ligne.

## Onglet 📊 Stats

Vue d'ensemble (avec défilement) regroupant :

- trois cartes : **Champions** (total / possédés / à acheter + barre de
  progression), **Essence Bleue** (coût total restant, ton EB, EB après
  tout achat — en rouge si négatif), **Essence Orange** (EO disponible) ;
- une carte **Valeur de ta collection** (au prix d'achat) : champions
  possédés (en EB) et skins + balises possédés (en EO) ;
- un **graphique d'évolution** de ton EB/EO dans le temps (courbe), à
  partir de l'historique horodaté à chaque modification — affiche un
  message d'aide tant qu'il y a moins de 2 points enregistrés ;
- un **camembert** de répartition de tes skins par rareté ;
- la liste **★ Priorité d'achat** : les champions non possédés que tu as
  marqués « à acheter en priorité » depuis leur fiche détaillée ;
- la liste des **champions à acheter**, avec leur prix effectif et une
  coche ✔ si tu as déjà assez d'EB pour celui-ci ;
- la liste des **champions avec réduction active** (prix standard → prix
  réduit, avec le montant économisé).

## Ajouter de nouveaux champions / skins / balises (sorties LoL)

Deux façons d'ajouter une nouvelle entrée à ta collection :

1. **Mise à jour du code (recommandé pour les sorties officielles)** :
   ouvre `model/datamanager.cpp` et ajoute une ligne dans la liste de
   référence concernée :
   - `DataManager::referenceChampions()` → `{"Nom", prixStandard, prixReduit, possede, RoleX|RoleY}`
   - `DataManager::referenceSkins()` → `{"Nom du skin", "Champion", prix, gratuit, "Rareté"}`
   - `DataManager::referenceBalises()` → `{"Nom de la balise", prix, possede}`

   Un commentaire indique où ajouter la ligne dans chaque liste.
   Recompile : au prochain lancement, `mergeNewChampions()` /
   `mergeNewSkins()` / `mergeNewBalises()` ajoutent automatiquement les
   entrées manquantes à ta sauvegarde existante **sans toucher** à tes
   données déjà enregistrées (essences, possessions, balises cochées...).

2. **Boutons « ✚ Nouveau... » (ajout à la volée, sans recompiler)** :
   - Onglet ⚔ Champions → « ✚ Nouveau champion » (nom, prix standard, prix réduit, possédé, rôle(s))
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
- une case « ★ À acheter en priorité », pour le retrouver en premier
  dans la liste « Champions à acheter » de l'onglet Stats ;
- ses **rôles** (Top, Jungle, Mid, ADC, Support — cumulables), utilisés
  par le filtre par rôle de l'onglet Champions ;
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

## Exporter / importer ta sauvegarde

Deux boutons en bas de la barre latérale, sous le bloc Essence :

- **💾 Exporter** : enregistre une copie de toutes tes données actuelles
  (champions, skins, balises, essences, historique) dans un fichier
  `.json` de ton choix. Utile pour synchroniser entre deux PC ou se
  prémunir avant une réinstallation.
- **📂 Importer** : remplace **entièrement** tes données actuelles par
  celles d'un fichier `.json` exporté précédemment, après confirmation
  (action irréversible). Le fichier choisi est validé a minima (JSON +
  clés attendues) avant tout remplacement ; en cas de fichier invalide,
  tes données actuelles ne sont pas touchées. Comme au démarrage normal,
  les champions/skins/balises sortis depuis la création du backup sont
  fusionnés automatiquement après l'import.

Implémenté côté Modèle par `DataManager::exportTo()` / `importFrom()`,
relayé par le Contrôleur (`AppController::exportData()` / `importData()`).

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

- Le `.pro` déclare `QT += core gui widgets charts network` : le module
  `charts` est **utilisé** par `view/statswidget.h/.cpp` (graphique
  d'évolution EB/EO en `QLineSeries`/`QDateTimeAxis`, camembert de
  rareté des skins en `QPieSeries`) et `network` par
  `view/addskindialog.h/.cpp` (auto-complétion Community Dragon) et
  `view/imagedownloaddialog.h/.cpp` (téléchargement Data Dragon) via
  `QNetworkAccessManager`. Les deux modules sont donc requis pour
  compiler le projet tel quel.
- Le projet nécessite Qt 6 (voir [Prérequis](#prérequis)).
- **Rôles des champions** (`model/champion.h`) : stockés comme un masque
  de bits (`RoleFlag`, combinable avec `|`) pour qu'un champion puisse
  cumuler plusieurs rôles (ex. Akali : Mid + Top). Contrairement au nom
  ou au prix, ce champ n'a pas de source officielle Data Dragon (qui
  n'expose que des archétypes génériques type Fighter/Mage, pas des
  lanes) : il est assigné à la main à partir de la méta actuelle, et
  reste modifiable par champion depuis sa fiche détaillée.

## Mentions légales

Ce projet est un outil de fan **non officiel**, sans aucun lien avec
Riot Games, Inc. League of Legends, les noms et images des champions et
des skins, ainsi que toutes les autres données de jeu utilisées par
l'application (via Data Dragon et Community Dragon, cf.
[Notes techniques](#notes-techniques)) restent la propriété de Riot
Games. Ce projet n'est ni endossé ni sponsorisé par Riot Games.

Si tu envisages de partager publiquement un fork de ce projet, ou —
*a fortiori* — de le commercialiser (vente, abonnement, publicité...),
réfère-toi impérativement aux conditions actuelles de Riot Games avant
de le faire, notamment :

- la politique **« Legal Jibber Jabber »** (conditions d'utilisation de
  l'IP de Riot par les fans) : <https://www.riotgames.com/en/legal> —
  par défaut, les projets de fans doivent rester **non commerciaux** ;
  Riot n'autorise que trois exceptions étroites (revenus publicitaires
  passifs, dons/abonnements en streaming, ou usage commercial conforme
  à ses API Terms avec une clé d'API Riot valide qui t'a été délivrée
  spécifiquement) ;
- les **conditions d'utilisation de l'API Riot Games** (« API Terms ») :
  <https://developer.riotgames.com/terms>, et les **politiques pour les
  produits tiers** (Developer Portal) qui imposent notamment
  l'enregistrement du produit sur le portail développeur, un statut
  « Approved »/« Acknowledged » pour pouvoir le monétiser, et l'affichage
  d'une mention légale spécifique visible par les joueurs ;
- les **conditions générales d'utilisation** de Riot Games :
  <https://www.riotgames.com/en/terms-of-service>, qui rappellent que
  Riot conserve tous les droits sur le contenu de ses jeux.

⚠️ Tel qu'il est écrit aujourd'hui, ce projet récupère ses données
(images de champions, infos de skins) directement depuis Data Dragon
(CDN public officiel de Riot, sans clé d'API) et Community Dragon (miroir
communautaire **non officiel**, sans lien avec Riot) — pas via l'API
développeur officielle avec une clé délivrée par Riot. L'exception
« API Dev Terms » mentionnée ci-dessus, qui est la voie la plus
réaliste pour un usage commercial, ne s'applique donc pas en l'état :
elle suppose un enregistrement du produit sur le Developer Portal de
Riot et l'obtention d'une clé d'API dédiée.

Je ne suis ni avocat ni juriste, et ces conditions évoluent dans le
temps (la page Legal Jibber Jabber elle-même le précise) : ce qui
précède n'est qu'un pointeur vers les bonnes ressources, pas un avis
juridique. Vérifie toujours les pages officielles ci-dessus avant toute
publication ou monétisation. Pour l'usage strictement personnel et privé
actuel de ce repo (suivi de ta propre collection), cette section n'a pas
d'impact concret — elle a vocation à protéger quiconque reprendrait ce
code si le dépôt devenait public ou était partagé/forké un jour.
