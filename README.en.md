# League Tracker — Qt Application (MVC architecture)

*[🇫🇷 Lire en français](README.fr.md)*

Track your League of Legends collection (champions, skins, wards) : prices, ownership, Blue Essence / Orange Essence budget, and what's currently purchasable.

## Table of contents

- [Requirements](#requirements)
- [Build](#build)
- [MVC architecture](#mvc-architecture)
- [⚔ Champions tab](#-champions-tab)
- [✨ Skins tab](#-skins-tab)
- [🚩 Wards tab](#-wards-tab)
- [📊 Stats tab](#-stats-tab)
- [Adding new champions / skins / wards](#adding-new-champions--skins--wards)
- [Deleting a champion / skin / ward](#deleting-a-champion--skin--ward)
- [Managing a champion's skins from its detail sheet](#managing-a-champions-skins-from-its-detail-sheet)
- [Exporting / importing your save data](#exporting--importing-your-save-data)
- [Appearance and application icon](#appearance-and-application-icon)
- [Quitting the application](#quitting-the-application)
- [Quality of life: keyboard shortcuts and notifications](#quality-of-life-keyboard-shortcuts-and-notifications)
- [Adding champion images](#adding-champion-images)
- [Data and save files](#data-and-save-files)
- [Technical notes](#technical-notes)
- [Legal notice](#legal-notice)

## Requirements

- **Qt 6.x** with the `core`, `gui`, `widgets`, `charts`, `network` modules
  (see [Technical notes](#technical-notes) for details on how each is used)
- A **C++17** compiler
- Qt Creator (recommended) or `qmake` + `make` on the command line

> ⚠️ This project is **not Qt 5 compatible**: `ChampionCard::enterEvent` uses `QEnterEvent`, an API introduced in Qt 6.

## Build

```bash
qmake LeagueTracker.pro
make
```
Or open `LeagueTracker.pro` in Qt Creator → Run.

## MVC architecture

The project is organized into three clearly separated layers:

```
LeagueTracker/
├── LeagueTracker.pro
├── main.cpp                   → entry point (theme, icon, creates the Controller, injects it into the window)
├── resources.qrc              → embedded resources (app icon)
├── icone/
│   ├── logo_lt.png
│   └── logo_lt.ico
├── model/                      → MODEL
│   ├── champion.h / skin.h / balise.h   (data structures)
│   └── datamanager.h/.cpp     (data + JSON persistence + pure calculations)
├── controller/                 → CONTROLLER
│   └── appcontroller.h/.cpp   (filtering, purchase rules, View actions)
└── view/                        → VIEWS
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
    ├── toast.h/.cpp                (floating, non-blocking "toast" notification)
    └── imagedownloaddialog.h/.cpp  (automatic download of Data Dragon portraits)
```

### Role of each layer

- **Model (`model/DataManager`)**: owns the data (champions, skins,
  wards, essences), handles its JSON persistence, and exposes the
  "raw" calculations tied to that data (`champsOwned()`,
  `coutTotalRestant()`, `ebApresAchat()`...). Every mutation goes
  through a dedicated Model method (`updateChampion`, `setBaliseOwned`,
  `setEssenceBleu`...) which saves and then emits `dataChanged()`.

- **Controller (`controller/AppController`)**: the sole passage point
  between the Views and the Model. It relays reads (`champions()`,
  `skins()`, `balises()`, statistics...), carries "application" logic
  that isn't raw data (grid filtering via `filteredChampionIndices()`,
  purchase rules `canBuySkin()` / `canBuyBalise()`), and forwards user
  actions to the Model (`updateChampion`, `setBaliseOwned`,
  `setEssenceBleu/Orange`).

- **Views (`view/...`)**: display and user-event capture only (clicks,
  input...). No View ever accesses `DataManager` directly: everything
  goes through the `AppController*` received in the constructor. Views
  refresh themselves by subscribing to the `AppController::dataChanged`
  signal.

Typical flow (e.g. checking a ward as owned):

```
BalisePage (View)
   → m_controller->setBaliseOwned(row, checked)   [Controller]
   → DataManager::setBaliseOwned(...)             [Model: mutation + save() + emit dataChanged()]
   → AppController::dataChanged                   [relayed by the Controller]
   → BalisePage::refresh() (+ StatsWidget, etc.)   [every subscribed View updates itself]
```

## ⚔ Champions tab

Grid of cards (image + name + status), with:

- a **search bar** by name;
- a **filter** with 7 modes: All champions, Owned, Not owned,
  Cheap (≤ 675), Mid (1575), Expensive (2400+), On sale;
- a **role filter** (Top, Jungle, Mid, ADC, Support) — a champion can
  have several roles at once (see [Technical notes](#technical-notes));
- a **sort bar** (Name, Effective price, Owned first, Not owned first)
  with order reversal (↑ / ↓);
- a counter of how many champions are currently displayed;
- clicking a card opens the champion's detail sheet (status, price,
  purchase priority, roles, and its skins — see
  [further below](#managing-a-champions-skins-from-its-detail-sheet)).

## ✨ Skins tab

Table listing every skin, with:

- **multi-criteria sorting**: Name, Champion, Rarity, Price, or Owned
  (first), either via the dedicated combo box **or** by clicking
  directly on a column header, with a button to reverse the order
  (↑ / ↓);
- **combined filters** (stackable): rarity, champion owned/not owned,
  skin owned/not owned — a "✕ Reset filters" button appears as soon as
  at least one filter is active;
- a hierarchy of **7 rarities** + Free, each with its own color:

  | Rarity | Color |
  |---|---|
  | Basic | gold (default) |
  | Epic | purple |
  | Legendary | orange |
  | Mythic | pink |
  | Ultimate | red |
  | Exalted | gold-yellow |
  | Transcendent | cyan |
  | Free | green |

- a **Purchasable** column that cross-references the associated
  champion's status with your current OE ("Already owned", "Champion
  missing", "Missing X OE", or "Yes");
- an **Owned** checkbox directly in the table (saved instantly);
- a "✚ New skin" button (with best-effort auto-completion of the name,
  rarity and OE price via Community Dragon once a recognized champion
  is selected — see `view/addskindialog.h`) and a delete button per row.

## 🚩 Wards tab

Table listing every ward, with:

- **multi-criteria sorting**: Name (A → Z), Price, Owned first, or Not
  owned first, either via the dedicated combo box **or** by clicking
  directly on a column header, with order reversal (↑ / ↓);
- an **Owned** checkbox per row (saved instantly);
- a **Purchasable** column based on your current OE;
- a "✚ New ward" button and a delete button per row.

## 📊 Stats tab

Scrollable overview combining:

- three cards: **Champions** (total / owned / to buy + progress bar),
  **Blue Essence** (total remaining cost, your BE, BE after buying
  everything — shown in red if negative), **Orange Essence**
  (OE available);
- a **Collection value** card (at purchase price): owned champions
  (in BE) and owned skins + wards (in OE);
- an **evolution chart** of your BE/OE over time (line chart), built
  from the timestamped history of every change — shows a hint message
  as long as fewer than 2 points have been recorded;
- a **pie chart** of your skins broken down by rarity;
- the **★ Purchase priority** list: unowned champions you've marked
  "buy in priority" from their detail sheet;
- the list of **champions to buy**, with their effective price and a ✔
  if you already have enough BE for it;
- the list of **champions with an active discount** (standard price →
  discounted price, with the amount saved).

## Adding new champions / skins / wards (LoL releases)

Two ways to add a new entry to your collection:

1. **Updating the code (recommended for official releases)**: open
   `model/datamanager.cpp` and add a line to the relevant reference
   list:
   - `DataManager::referenceChampions()` → `{"Name", standardPrice, discountedPrice, owned, RoleX|RoleY}`
   - `DataManager::referenceSkins()` → `{"Skin name", "Champion", price, free, "Rarity"}`
   - `DataManager::referenceBalises()` → `{"Ward name", price, owned}`

   A comment in each list shows exactly where to add the line.
   Rebuild: on next launch, `mergeNewChampions()` / `mergeNewSkins()` /
   `mergeNewBalises()` automatically append the missing entries to your
   existing save file **without touching** your already-recorded data
   (essences, ownership, checked wards...).

2. **"✚ New..." buttons (on-the-fly, no rebuild needed)**:
   - ⚔ Champions tab → "✚ New champion" (name, standard price, discounted price, owned, role(s))
   - ✨ Skins tab → "✚ New skin" (name, champion, rarity, price/free, owned)
   - 🚩 Wards tab → "✚ New ward" (name, price/free, owned)

   In all three cases, the entry is added and saved immediately. A
   name that already exists (case-insensitive) is rejected.

## Deleting a champion / skin / ward

- **Champion**: click its card to open the detail window, then click
  "🗑 Delete" (bottom left). A confirmation is required before the
  permanent deletion (a heavier action than a single table row).
- **Skin / Ward**: every table row has a "🗑" button on the right.
  Deletion is **immediate**, with no blocking confirmation dialog — a
  discreet notification appears bottom-right with an "Undo" button for
  a few seconds in case of a mistake.

In all three cases, deletion calls `removeChampion`/`removeSkin`/
`removeBalise` on the Controller → Model, which removes the entry,
saves, and notifies the Views (`dataChanged`).

## Managing a champion's skins from its detail sheet

Clicking a champion's card opens the detail window, which shows:

- its **status** (owned or not), its **standard price**, its
  **discounted price**, and the **effective price** computed
  automatically;
- a "★ Buy in priority" checkbox, to find it first in the "Champions to
  buy" list of the Stats tab;
- its **roles** (Top, Jungle, Mid, ADC, Support — stackable), used by
  the role filter on the Champions tab;
- the list of its **skins**, each with an "Owned" checkbox you can
  toggle at any time — the change is applied and saved instantly (just
  like wards), independently of the "Save" button;
- a "✚ Add a skin" button that opens the same form as on the ✨ Skins
  tab, with this champion pre-selected.

The `Skin::possede` field is taken into account on the ✨ Skins tab
("Owned" column, also toggleable from there) and in the "Purchasable"
calculation (`AppController::canBuySkin`): a skin you already own is
no longer offered as purchasable.

## Exporting / importing your save data

Two buttons at the bottom of the sidebar, below the Essence block:

- **💾 Export**: saves a copy of all your current data (champions,
  skins, wards, essences, history) to a `.json` file of your choice.
  Useful to sync between two PCs, or as a safety net before a
  reinstall.
- **📂 Import**: **entirely** replaces your current data with that of a
  previously exported `.json` file, after confirmation (irreversible
  action). The chosen file is validated at a minimum level (valid
  JSON + expected keys) before any replacement happens; if the file is
  invalid, your current data is left untouched. Just like a normal
  startup, any champions/skins/wards released since the backup was
  created are automatically merged in after the import.

Implemented on the Model side via `DataManager::exportTo()` /
`importFrom()`, relayed by the Controller (`AppController::exportData()`
/ `importData()`).

## Appearance and application icon

- Dark gold/LoL theme applied via Qt style sheets (QSS) on each page.
- `main.cpp` forces the **Fusion** style + a custom dark **QPalette**
  *before* the main window is created: this fix avoids the brief white
  flash that otherwise appeared on startup on Windows.
- The application icon (`icone/logo_lt.png`, embedded via
  `resources.qrc`) is applied to both the window **and** the taskbar
  via `app.setWindowIcon(...)`.

## Quitting the application

A "⏻ Quit" button sits at the bottom of the sidebar, below the Essence
block. It asks for confirmation before closing the application, to
avoid an accidental shutdown.

## Quality of life: keyboard shortcuts and notifications

- **Ctrl+F** (Cmd+F on macOS) focuses the search bar of the active tab
  (Champions / Skins / Wards). Handled by a single global `QShortcut`
  in `MainWindow`, which delegates to the current page via
  `focusSearch()`.
- **Escape** closes the currently open dialog (add/edit/delete). This
  is `QDialog`'s native Qt behavior — no specific code was needed, as
  long as no view captures the key before it does.
- Quick, reversible actions (toggling an "Owned" checkbox, deleting a
  skin/ward row) no longer show a blocking `QMessageBox`: a small
  notification ("toast", `view/toast.h`) appears bottom-right for a
  few seconds, with an "Undo" button for deletions. Only one toast is
  shown at a time (a new one replaces the previous) to stay
  unobtrusive even with rapid clicks. Deleting a **champion** still
  keeps its classic confirmation: it's a more committing action than a
  single table row.

## Adding champion images

Create an `images/` folder **next to the executable** and place
portraits inside it.

**Accepted filenames** (png, jpg or webp):
- `Ahri.png`
- `LeeSin.png`  (camelCase, no spaces)
- `lee_sin.png` (lowercase, underscore)

### Automatic download from within the app (recommended)

On the ⚔ Champions tab, the "⬇ Download missing images" button (next
to "✚ New champion"):

1. fetches the latest available Data Dragon version;
2. fetches the champion list (`fr_FR`) to match the French name used by
   the app to the Data Dragon identifier — useful for champions whose
   id differs from their displayed name (e.g. *Wukong* → `MonkeyKing`,
   *Kai'Sa* → `Kaisa`);
3. downloads only the portraits of champions that **don't** already
   have a local image in `images/` (sequential download, with a
   progress bar and a Cancel button);
4. saves each image under a name that `ChampionCard::loadImage()` will
   immediately recognize (image cache invalidated at the end of the
   download, no need to relaunch the app).

The manual curl script below remains useful for a bulk download
outside the app, or in an environment without a graphical interface.

### Manual download via curl (optional)

```bash
# Fetches the latest available Data Dragon version
VERSION=$(curl -s "https://ddragon.leagueoflegends.com/api/versions.json" | python3 -c "import sys,json;print(json.load(sys.stdin)[0])")
mkdir -p images
while read nom; do
  curl -s "https://ddragon.leagueoflegends.com/cdn/$VERSION/img/champion/${nom}.png" \
       -o "images/${nom}.png"
done << CHAMPS
Aatrox
Ahri
Akali
... (full list at https://ddragon.leagueoflegends.com/cdn/$VERSION/data/fr_FR/champion.json)
CHAMPS
```

## Data and save files

Data is saved automatically (by the Model) to a `lol_data.json` file,
in an application-specific folder managed by Qt
(`QStandardPaths::AppDataLocation`). Since both the organization name
and the application name are set to `"LeagueTracker"`, Qt creates a
**duplicated** folder:

- **Windows**: `%APPDATA%\LeagueTracker\LeagueTracker\lol_data.json`
- **Linux**:   `~/.local/share/LeagueTracker/LeagueTracker/lol_data.json`
- **macOS**:   `~/Library/Application Support/LeagueTracker/LeagueTracker/lol_data.json`

> 💡 To avoid this duplicated folder, you can leave
> `QCoreApplication::setOrganizationName(...)` empty in `main.cpp`, or
> give it a name different from the application's (e.g. a nickname or
> team name).

## Technical notes

- The `.pro` file declares `QT += core gui widgets charts network`: the
  `charts` module is **actually used** by `view/statswidget.h/.cpp`
  (BE/OE evolution chart via `QLineSeries`/`QDateTimeAxis`, skin-rarity
  pie chart via `QPieSeries`), and `network` is used by
  `view/addskindialog.h/.cpp` (Community Dragon auto-completion) and
  `view/imagedownloaddialog.h/.cpp` (Data Dragon downloads) via
  `QNetworkAccessManager`. Both modules are therefore required to
  build the project as-is.
- The project requires Qt 6 (see [Requirements](#requirements)).
- **Champion roles** (`model/champion.h`): stored as a bitmask
  (`RoleFlag`, combinable with `|`) so a champion can hold several
  roles at once (e.g. Akali: Mid + Top). Unlike the name or the price,
  this field has no official Data Dragon source (which only exposes
  generic archetypes like Fighter/Mage, not lanes): it is assigned by
  hand based on the current meta, and remains editable per champion
  from its detail sheet.

## Legal notice

This project is an **unofficial** fan tool, with no affiliation to
Riot Games, Inc. League of Legends, the champion and skin names and
images, and all other game data used by the application (via Data
Dragon and Community Dragon, see [Technical notes](#technical-notes))
remain the property of Riot Games. This project isn't endorsed or
sponsored by Riot Games.

If you're considering publicly sharing a fork of this project, or —
*a fortiori* — commercializing it (sales, subscriptions, advertising...),
make sure to check Riot Games' current terms before doing so, in
particular:

- the **"Legal Jibber Jabber"** policy (Riot's terms for fan use of
  their IP): <https://www.riotgames.com/en/legal> — by default, fan
  projects must remain **non-commercial**; Riot only allows three
  narrow exceptions (passive ad revenue, streaming donations/
  subscriptions, or commercial use that complies with their API Terms
  using a valid Riot API key issued specifically to you);
- the **Riot Games API Terms of Use**: <https://developer.riotgames.com/terms>,
  and the **third-party product policies** (Developer Portal), which
  notably require registering the product on the developer portal, an
  "Approved"/"Acknowledged" status before monetizing it, and displaying
  a specific legal notice visible to players;
- Riot Games' **general Terms of Service**:
  <https://www.riotgames.com/en/terms-of-service>, which state that
  Riot retains all rights to its games' content.

⚠️ As written today, this project fetches its data (champion images,
skin info) directly from Data Dragon (Riot's official public CDN, no
API key) and Community Dragon (an **unofficial** community mirror,
unaffiliated with Riot) — not through the official developer API with
a key issued by Riot. The "API Dev Terms" exception mentioned above,
which is the most realistic path for commercial use, therefore doesn't
apply as-is: it requires registering the product on Riot's Developer
Portal and obtaining a dedicated API key.

I'm neither a lawyer nor a legal professional, and these terms change
over time (the Legal Jibber Jabber page itself says so): the above is
only a pointer to the right resources, not legal advice. Always check
the official pages above before any publication or monetization. For
the project's current strictly personal and private use (tracking your
own collection), this section has no practical impact — it's meant to
protect anyone who picks up this code if the repository were ever made
public or shared/forked.
