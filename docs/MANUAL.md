# OpenBolo Manual

Based on the original WinBolo 1.14 manual by John Morrison.
Portions based on the original Bolo manual © 1987–1995 Stuart Cheshire.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Introduction](#introduction)
3. [How to Play](#how-to-play)
   - [Controls](#controls)
   - [Starting the Game](#starting-the-game)
   - [Terrain](#terrain)
   - [Pillboxes](#pillboxes)
   - [Refuelling Bases](#refuelling-bases)
   - [Farming and Building](#farming-and-building)
   - [Mine Laying](#mine-laying)
   - [Mine Clearing](#mine-clearing)
   - [Alliances](#alliances)
   - [Building a Fortress](#building-a-fortress)
   - [The HUD](#the-hud)
   - [Remote Views](#remote-views)
4. [Dedicated Server](#dedicated-server)

---

## Quick Start

### Single Player

1. Run OpenBolo.
2. Enter your player name at the startup screen (Enter to confirm, ESC for default).
3. The game loads **Everard Island** automatically.
4. Find a refuelling base, collect shells, and start exploring.

---

## Introduction

### What is Bolo?

Bolo is a tank battle set on an island, for up to 16 players, each on a separate
computer so that no player can see what the others are doing. This enables players
to lay hidden minefields, hide under forest cover, and ambush other players without
being seen.

Bolo is not a two-player deathmatch. It is a multi-player team strategy game.
When you find yourself cornered by the enemy, you can call for help and have allies
break out of the forest together to rescue you.

The ultimate objective is to capture all refuelling bases on the map.

### What is WinBolo / OpenBolo?

WinBolo was an implementation of Bolo from scratch for Windows and Linux, written by
John Morrison (1998–2008) and released as open source under the GPL. LinBolo and
WinBolo are network-compatible.

OpenBolo is a modernization of WinBolo 1.15, replacing the original DirectX and
Win32 GUI layer with Raylib while preserving the game engine and network protocol
unchanged.

### History

Bolo was originally written in 1987 for the BBC Micro, then ported to the Macintosh.
The name *Bolo* is the Hindi word for *communication*, reflecting that the game is
fundamentally about players communicating and cooperating.

---

## How to Play

### Controls

| Key | Action |
|-----|--------|
| Arrow Up | Accelerate |
| Arrow Down | Decelerate |
| Arrow Left | Turn anti-clockwise |
| Arrow Right | Turn clockwise |
| Space | Fire shell |
| Tab | Quick-drop mine (visible to nearby tanks) |
| B | Send builder (LGM) to selected square |
| W | Increase gunsight range |
| S | Decrease gunsight range |
| ; (semicolon) | Cycle pillbox view (owned pillboxes) |
| Return | Return to tank view |

> **Note on quick mines (Tab):** Mines dropped this way are visible to all nearby
> tanks. If you are being chased, an enemy *will* see them and can avoid them. Use
> the builder (B) to lay invisible mines instead.

> **Note on gunsight (W/S):** Normally shells hit the first obstacle in their path
> and range adjustment is unnecessary. Use it deliberately to land shells on mines
> to clear a path through a suspected minefield.

### Starting the Game

When the game starts your tank is at sea on a boat. Head towards the island and
drive onto the land — you may need some speed to leave the boat.

Boats can sail up rivers but **cannot pass under low bridges**. To pass a bridge you
must either leave the boat and go on foot or shoot the bridge down.

> **Caution:** Boats sink in a single hit. If you are sunk in *Deep Sea* (dark
> rippling water) your tank is destroyed immediately. Stay in shallow water or
> on land.

### Terrain

| Terrain | Speed | Notes |
|---------|-------|-------|
| Grass | Medium | Covers most of the island |
| Water | Very slow | Depletes shells and mines while submerged; avoid without a boat |
| Deep Sea | Instant death | Without a boat your tank sinks immediately |
| Swamp | Very slow | Looks similar to grass — watch carefully |
| Road | Fast | Build roads where you travel frequently |
| Bridge | Fast | Build over rivers you cross often |
| Forest | Slow | Tanks inside dense forest are **invisible** to enemies and pillboxes |
| Building | Barrier | Provides cover; can be shot down to rubble |
| Rubble | Very slow | Remains after a building is destroyed |
| Crater | Very slow | Left by mine explosions; floods if adjacent to water |
| Moored Boat | Fast | Drive onto it to travel by river or sea |
| Pillbox | Barrier | See [Pillboxes](#pillboxes) |
| Refuelling Base | Medium | See [Refuelling Bases](#refuelling-bases) |

### Pillboxes

Pillboxes automatically shoot at any enemy tank in range. They are extremely
accurate and fire rapidly when provoked — in a straight-on fight, a pillbox beats
a tank every time.

**To destroy a pillbox:** Attack quickly, score hits, then retreat out of range.
Repeat patiently. Refuel at a base between attacks — there is no need to finish
it in one run.

Pillboxes cannot be *permanently* destroyed, only disabled. When fully disabled
the pillbox becomes inactive and you can drive over it to **pick it up**. It is then
repaired and becomes loyal to you and your allies. You can later place it back on
the map at a strategic location to defend your territory.

**Claiming territory with pillboxes** is the key strategy of the game — not fighting
players directly, but building a defended fortress that enemy tanks cannot penetrate.

### Refuelling Bases

Tanks start the game with limited shells and mines. Find a refuelling base early to
replenish supplies.

- Drive onto a base to refuel automatically.
- The base is also automatically **captured** when you drive onto it.
- An enemy tank cannot refuel at your base but can shoot it to deplete its armour
  stock. Once armour is gone the enemy can drive on and capture it.
- The right side of the screen shows the closest friendly base's shell, mine, and
  armour stocks.

**The objective of the game** is to eventually capture all refuelling bases.

### Farming and Building

Your tank carries a builder (the Little Green Man, or LGM). To use him:

1. Select the build mode (road, bridge, building, pillbox placement, or mine).
2. Click on the target square on the map.
3. The builder leaves the tank, runs to the square, builds the object, and returns.

You need **tree resources** to build — farm forest first. Building costs:

| Object | Cost |
|--------|------|
| Road or bridge | ½ tree |
| Building | ½ tree |
| Pillbox placement / repair | 1 tree (up to) |
| Boat | 5 trees |

**Restrictions:** You cannot build on deep sea, on a moored boat, or on existing
forest (farm it or shoot it first).

**Repairing a pillbox:** Select pillbox mode and target a damaged pillbox. The builder
repairs it — even if it belongs to someone else. Note that *repairing* a pillbox
does **not** capture it for you; you must pick it up with your tank first.

**Builder safety:** If the builder is shot while outside the tank, a replacement is
parachuted in — but this can take several minutes, during which you cannot build.

Forests regrow over time, so farmed trees are eventually replenished.

### Mine Laying

There are two ways to lay mines:

**Quick-drop (Tab key):** Instantly drops a mine under the tank. Fast, but all
nearby tanks can see it. Not useful as a surprise weapon against a pursuer — they
will see it and swerve.

**Builder mine (select mine mode, click target square):** Takes longer because the
builder must walk to the location. Advantages:
- You don't have to drive the tank to the spot.
- The mine is **invisible** even if someone is watching when you lay it.
- Allied tanks are automatically informed of its location and can see it on their maps.

### Mine Clearing

To clear a suspected minefield, reduce your gunsight range (S key) and fire to land
shells ahead of the tank. A shell will detonate any mine it lands on.

> **Tip:** Exploding mines trigger adjacent mines in a chain reaction. Lay mines in a
> checkerboard pattern to prevent your own minefield from chain-detonating. Craters
> adjacent to water will flood — a long mine chain leading to the sea creates an
> artificial river moat.

### Alliances

Pillboxes you capture shoot all other players by default. To have them protect your
friends, form a formal alliance:

1. One player selects the other from the Players menu and requests an alliance
   (WinBolo → Request Alliance).
2. The other player receives a popup and accepts or declines.

**Alliance benefits:**
- Allied pillboxes do not shoot each other.
- Allied tanks can see each other's mines as they are laid (not retroactively).

To leave an alliance, select WinBolo → Leave Alliance. Any member may also invite
a new player into an existing alliance.

### Building a Fortress

Choose a defensible location and build outward:

- **Pillboxes for defence** — place them so they provide overlapping fields of fire.
  A pillbox left alone at a corner will be picked off; pillboxes should cover each
  other. Do not place them so close that they risk shooting each other.
- **Thick building walls** on the perimeter.
- **Roads** internally for fast movement.
- **Hidden minefields** (builder-laid, not quick-drop) around approaches.
- **Natural obstacles** like swamp slow attackers, but avoid surrounding yourself
  with forest — enemies can use it for unseen approaches.

### The HUD

The right side of the screen shows:

| Display | Meaning |
|---------|---------|
| Tank icons panel | Status of all player tanks (red = hostile, green = friendly, hollow circle = you) |
| Pill icons panel | Status of all pillboxes (red square = hostile, green circle = friendly; checkerboard = neutral) |
| Base icons panel | Status of all bases (green = friendly, red = hostile; checkerboard = neutral) |
| Base stock bars | Shell / mine / armour stocks of the nearest friendly base |
| Tank stock bars | Your tank's shells / mines / armour / building materials |
| Message bar | Game messages, player name, builder reports |

**Colour conventions:**
- **Red** = hostile
- **Green** = friendly (allied or yours)
- **Checkerboard pattern** = neutral (not yet captured by anyone)
- **Your tank's turret** is always black on the map to distinguish it from allies

### Remote Views

Press **;** (semicolon) to cycle through pillbox view — each press switches to the
next pillbox owned by you or your alliance, so you can monitor your defences while
attacking elsewhere. Press **Return** to return to your tank's view. While in
pillbox view you can also use the arrow keys to switch between neighbouring pillboxes.

---

## Dedicated Server

The standalone server (`winbolo-server`) is run from the command line:

```
winbolo-server -map <filename> -port <port> -gametype <type> [options]
```

| Argument | Description |
|----------|-------------|
| `-map <file>` | Map file to load (`-inbuilt` uses Everard Island) |
| `-port <n>` | UDP port to run on |
| `-gametype <type>` | `Open`, `Tournament`, or `Strict` |
| `-mines yes\|no` | Allow hidden mines (default: yes) |
| `-ai yes\|no\|yesAdv` | Allow computer brains (default: no) |
| `-delay <secs>` | Start delay in seconds |
| `-limit <mins>` | Time limit in minutes (-1 = unlimited) |
| `-password <pw>` | Game password |
| `-tracker <addr:port>` | Internet tracker address |
| `-quiet` | No console output |
| `-maxplayers <n>` | Maximum players allowed |
| `-logfile` | Log output to file instead of console |

**Game types:**

| Type | Starting ammo |
|------|--------------|
| Open | Full shells, mines, and trees |
| Tournament | Free shells at start; decrease as neutral bases fall |
| Strict | No free starting ammo |

All game types start with full armour.

**Server commands (while running):**

| Command | Action |
|---------|--------|
| `lock` | Lock game (no new players) |
| `unlock` | Unlock game |
| `say <message>` | Send message to all players |
| `savemap` | Save current map state |
| `info` | Display game information |
| `quit` | Shut down server |
