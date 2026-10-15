# Project 1 — 2D Lane Tower Defense (D MET 502 Computer Graphics)

A 2D side-view, lane-based tower defense game in the spirit of *Plants vs. Zombies*:
you buy characters (cards) with coins collected from defeated enemies, place them on
one of four horizontal lanes, and they fight the enemies that walk in from the right.

The logic layer is deliberately **theme-neutral** — the specification leaves the
setting and art style to the team, and no asset exists in the project yet (see §1 and
§6, where one suggestion is spelled out).

The assignment specification lives next to this file: `Project1_description.pdf`.
Everything in the code is traceable to a section of that document — see
[Requirements → code map](#requirements--code-map).

---

## 1. Current status (read this first)

| Area | State |
|---|---|
| Gameplay **logic** | **Complete** — placement, economy, waves, combat, dodging, shielding, win/lose, menus, cutscene |
| Console **test harness** | **Complete** — 10 groups / **134 checks**, all passing |
| **Rendering** | **Not implemented.** The OpenGL host opens a window and clears it to a dark colour. There is a marked seam in `main.cpp` where draw calls belong |
| **UI** (coins HUD, player cards, health bars, subtitles) | **Not implemented** (data is exposed by the logic, see §6) |
| **Assets** (models, textures, sounds) | **None**. No file is loaded from disk anywhere in the project |
| **Audio** | Interface only (`Audio.h`). Every sound trigger is emitted as an event; volume/mute state is real. No audio backend and no sound files |
| Extra game modes / level design | Not started |

So: **run it today and you get a blank window that plays a complete, testable game
underneath.** The whole remaining job is presentation plus an audio backend.

### Controls that already work
| Input | Effect |
|---|---|
| Mouse move | Hover highlight follows the lane under the cursor (logic space) |
| Left click | Toggle the selection of a lane; with a card armed, click places the character |
| `1` / `2` / `3` | Arm the Melee / Ranged / Defense card (placeholder for the card UI) |
| `W` or `Up` / `S` or `Down` | Make every player of the **selected lanes** jump / duck |
| `Esc` | Cutscene: skip · Playing: pause menu · Pause menu: resume |
| Arrow keys + `Enter` | Navigate the menus |
| `Q` | Close the window (always) |

---

## 2. Build and run

Everything the project needs is vendored in `External/` (glad, GLFW headers +
`glfw3.lib`, glm), so `Project1` builds on its own — no other folder of the
repository is required.

### Visual Studio
1. Open `Project1.sln`.
2. **Project1** → the game window (OpenGL 4.3 core, GLFW + glad).
3. **Project1Tests** → the console logic tests; run it and read the printed report.

`Project1Tests` compiles only the logic translation units (no OpenGL, no window),
which is why it can be built and run anywhere.

### Command line
Logic tests (no GL needed):

```
g++ -std=c++17 -ITests -I. Tests/logic_tests.cpp \
    Playfield.cpp Player.cpp Enemy.cpp Projectile.cpp Collectibles.cpp \
    Economy.cpp Waves.cpp Menus.cpp Cutscene.cpp Game.cpp -o logic_tests
./logic_tests          # exit code 0 = every check passed
```

The window host additionally needs the vendored GL and the platform link flags:

```
g++ -std=c++17 -I. -IExternal -IExternal/include main.cpp External/src/glad.c \
    <all logic .cpp files> -o Project1 \
    -LExternal/lib -lglfw3 -lopengl32 -lgdi32 -luser32     # Windows / MinGW
```

Test-file header comment repeats the same instructions.

---

## 3. Architecture

**Hard rule: the gameplay logic knows nothing about OpenGL, windows, images or
sound files.** Only `main.cpp` touches GL. The logic is a library that a host
drives one frame at a time:

```
main.cpp (GLFW + glad)                 Game (logic facade)
   callbacks ──► InputState ──────────► Game::update(dt, in)
   ◄─────────── GameEvent list ────────  drainEvents()
   ◄─────────── read-only accessors ───  players() / enemies() / playfield() / ...
```

### Files

| File | Ownership |
|---|---|
| `Core/Types.h` | `Vec2`, `clampf`, every enum (`GameState`, `PlayerType`, `EnemyType`, `CharState`, `AttackHeight`, `Faction`, `WavePhase`), **all tunable constants**, `toString()` helpers |
| `Core/Random.h` | `Random` — one `std::mt19937`, seeded from `random_device`; `seed(n)` for deterministic tests |
| `Core/Input.h` | `InputState` — one frame of input. All buttons are **edges**; mouse is already in logic units; `clearEdges()` is called by the host after `Game::update` |
| `Core/Events.h` | `SfxId` (25 ids) + `GameEvent{sfx, message}` — the logic's only outward "noise" |
| `Character.h` | `CharacterBase` — the part every player and enemy shares: `lane`, `x`, `hp/maxHp`, `attackCooldown`, `animTime`, `hitFlash`, `attacking`, `state`, `alive()`, `healthRatio()`, `receiveDamage()`, `tick()` |
| `Playfield.*` | Lane geometry (bottoms/tops/centres), lane under a point, home base + spawn columns, hovered lane and the selected-lane set |
| `Player.*` | `Player` (Melee / Ranged / Defense), `makePlayer`, `commandJump`, `commandDuck`, `advancePlayer`, `findDefenseShield` |
| `Enemy.*` | `Enemy` (Easy-Upper / Medium-Lower / Hard-Melee), `makeEnemy`, stats helpers (`enemyMaxHp`, `enemyDamage`, `enemyCoinDrop`, …) |
| `Projectile.*` | One flying shot: faction, lane, x, y, velocity, **origin + range**, attack height, damage, source id |
| `Collectibles.*` | Drops of defeated enemies + overlap pickup against the player list |
| `Economy.*` | Coins, total ever collected, costs, affordability, unlock/re-lock state |
| `Waves.*` | The three waves: enemy pool per wave, spawn cadence, lane picking, wave lifecycle, "everything cleared?" |
| `Menus.*` | Menu kind, entries, labels, highlighted index, up/down/enter handling |
| `Cutscene.*` | The 14-second animated intro: actors, timeline events (move / anim / subtitle), current subtitle |
| `Audio.h` | `SoundSystem` interface (+ `NullSoundSystem` default) — volume, mute, music, `onEvent` |
| `Game.*` | The facade: owns one of everything, runs the state machine, exposes read-only accessors, collects events |
| `main.cpp` | GLFW/glad host, callbacks → `InputState`, screen→logic mouse mapping, `glm::ortho` projection, clear colour, and the `// frontend drawing goes here` marker inside the loop |
| `Tests/logic_tests.cpp` | Console harness driving the real `Game` frame by frame |
| `External/` | Vendored glad (headers + `src/glad.c`), GLFW headers, `glfw3.lib`, glm |

### Coordinate system (2D, side view)

```
 y ^  +-----------------------------+  PLAYFIELD_HEIGHT = 6.0
   |  | lane 3                      |      LANE_HEIGHT = 1.5
   |  | lane 2                      |      LANE_COUNT  = 4
   |  | lane 1                      |      PLAYFIELD_WIDTH = 24.0
   |  | lane 0                      |      HOME_BASE_X  = 2.0
   +--+-----------------------------+--> x   ENEMY_SPAWN_X = 23.5
      0                              x=24
```

* A character stores **only** `lane` and `x`. The y on screen comes from
  `Playfield::laneCenterY(lane)` — never store a y on a character.
* Players advance with `+x`, enemies with `-x`; the home base is the losing line at
  `x <= 2.0`.
* The host already converts the window mouse position into this space
  (`screenToLogicX/Y`) and uploads a `glm::ortho(0, 24, 0, 6)` projection, so a
  renderer can pass logic positions straight to the shader (1 unit = 1 unit).

---

## 4. Game rules as implemented

* **Lanes** — 4 horizontal lanes (spec asks for a minimum of 3; the sample grid in
  the spec has 4). A character can **never** change lane once it exists.
* **Placement** — click a lane to select it (several may be selected at once), arm a
  card, and the character appears at `PLAYER_SPAWN_X` (leftmost column) of that lane.
  Arming a card while a lane is already selected places immediately.
* **Players** (all cost coins, all deal 1 damage except Defense):
  | Type | Cost | HP | Behaviour |
  |---|---|---|---|
  | Melee | 10 | 5 | close range, advances right |
  | Ranged | 20 | 5 | fires straight projectiles that vanish on collision; a shot never harms players |
  | Defense | 30 | 7 | never moves, cannot jump/duck, has **no attack**, blocks and absorbs enemy attacks |
* **Enemies** (spawn at the right edge, random lane, mixed across three waves):
  | Type | HP | Damage | Attack |
  |---|---|---|---|
  | Easy-Upper | 5 | 1 | ranged, shots come **from above** → duck to dodge |
  | Medium-Lower | 5 | 1 | ranged, shots come **from below** → jump to dodge |
  | Hard-Melee | 7 | 2 | runs in, close-range swings that cannot be dodged |
* **Attack rhythm** — everyone attacks **from the moment they exist**, target or not
  (spec page 4: "even if there are no enemies" / "no players"). A swing or shot with
  nothing in front of it simply misses: a projectile only damages what it reaches
  inside its range (`RANGED_RANGE`), so stray attacks can never snipe across the map.
  `CharacterBase::attacking` is the flag for the attack animation; `state` describes
  movement/posture.
* **Duel** — when a player and an enemy meet (`ENCOUNTER_DISTANCE`) both stop and
  trade blows until one dies. Players attack every `PLAYER_ATTACK_INTERVAL`,
  enemies every `ENEMY_ATTACK_INTERVAL`.
* **Stances** — per selected lane, `W`/`Up` jumps (dodges *low*), `S`/`Down` ducks
  (dodges *high*). Straight attacks hit regardless. Defense ignores both.
* **Shield** — a Defense player protects players **behind** it in the same lane
  (`shield.x >= player.x`); once the player walks past it, or it dies, they are
  exposed again. (Implemented literally to the wording of the spec, so the protected
  window is brief for characters that keep walking.)
* **Economy** — start with 30 coins (3 Melee). Defeated enemies drop collectibles
  that are picked up on overlap (10 / 15 / 25 coins). A card is unlocked when
  `total collected >= cost` **and** `coins >= cost`; it re-locks when the balance
  falls below the cost, exactly as the spec requires.
* **Waves** — 3 waves: ≥3 easy · ≥5 easy+medium · ≥10 easy+medium+hard. A wave
  begins with its first spawn, ends with its last, and the next one starts only when
  the field is empty. Enemies never wait for the player to place anything.
* **Win / Lose** — win when all three waves are spawned and the field is cleared; lose
  the moment any enemy touches the home base, at which point the world freezes.
* **Menus** — Main (Start Game, Exit, Sound Settings), Pause (Continue, Restart,
  Return to Main Menu), Win/Lose (Restart, Return to Main Menu), Sound Settings
  (Mute, Volume Up, Volume Down, Back).
* **Cutscene** — a 14 s animated intro (spec: 10–20 s) with a player and an enemy
  actor, movement, animation cues and subtitles, so it is never text-only.

### The per-frame pipeline (order matters — do not shuffle)

`Game::updatePlaying()` runs, in this order:

```
1  pause check (Esc)                          6  projectiles fly and hit
2  grid input (hover / select / place)        7  deaths -> drops, corpses removed
3  stance input (jump / duck)                 8  collectibles picked up
4  waves spawn enemies                        9  card unlocking / re-locking
5  players think, then enemies think         10  win / lose checks
```

---

## 5. Tuning

Every balance number lives in **`Core/Types.h`** and nowhere else: geometry, speeds,
ranges, HP, damage, costs, coin drops, wave sizes, spawn cadence, stance timers,
cutscene length. Change them there and the whole game follows.

Values that are **ours, not the spec's** (the description leaves them open):
coin drops (10/15/25), attack intervals (0.5 s / 2.3 s), spawn interval (2.5 s),
advance speed (1.2), enemy speeds, cutscene length (14 s).

Those were chosen by simulation rather than by feel: a Melee-only auto-play policy
(driving `Game` through `seedRandom` + `update` with synthetic `InputState`s, exactly
the way `Tests/logic_tests.cpp` drives it — the sweep itself is not a shipped file)
wins **38 of 40 seeded games**. Re-run a sweep like that after touching any combat
constant.

---

## 6. Finishing the project — the rendering seam

`main.cpp` already has the shader program, the ortho projection, the three uniform
locations (`u_Model`, `u_View`, `u_Projection`) and a comment marking the drawing
block. Draw calls go there. The pattern used by the labs (a `struct Mesh` with
`createMesh` / `drawMesh` / `destroyMesh`, attribute layout `vec3 position + vec3
colour`, 6 floats per vertex) fits the existing shader unchanged.

Suggested order, because it also produces the fastest visible progress:

1. **Lanes** — for each `lane < playfield.laneCount()`, a quad between
   `laneBottomY(lane)` and `laneTopY(lane)`; alternate shades; draw a distinct strip
   for the home base (`0 … homeBaseX()`) and a boundary marker at `rightmostX()`.
   Highlight `playfield.hoveredLane()` and every lane where
   `playfield.isLaneSelected(lane)` — this alone makes the lane rules visible.
2. **Characters** — players and enemies at `laneCenterY(lane)`, walk/attack animation
   driven by `animTime` and `attacking`, red damage tint from `hitFlash`, health bar
   from `healthRatio()` above each one (spec §4.1 demands visible health at all times,
   and separate silhouettes per type, not recolours).
3. **Collectibles** (they have their own `bobTime` clock) and **projectiles**.
4. **HUD** — coin total (`economy().coins()` / `earnedTotal()`), the three player
   cards (grey out and ignore input when `!economy().isUnlocked(type)`, highlight the
   armed one with `game.armedCard()`), and call **`Game::selectCard(type)`** for card
   clicks so all the unlock/afford checks stay in one place.
5. **Menus and cutscene** — `game.menu()` gives kind, labels and highlighted index;
   `game.cutscene()` gives actors with positions/animation names and the current
   subtitle.
6. **Environment** — spec §4.2 wants at least five environmental elements from the
   chosen setting (towers, walls, banners, torches, gate, stone structures for an
   outpost theme).

### Audio backend
`Game` owns a `NullSoundSystem` (a concrete member — swap the member type or add a
setter to install a real one). `SoundSystem` is polymorphic on purpose: derive from
it, forward `onEvent()` to a lookup of the 25 `SfxId` values, implement
`playBackgroundMusic()` / `stopBackgroundMusic()`, and the sound-settings menu items
(`VolumeUp`, `VolumeDown`, `ToggleMute`) already route into `setVolume()` etc.
Background music + at least 4 interaction sounds + a working settings menu is the
spec's optional bonus (§5).

### Not implemented, in one list
* Every visual: models, textures, health bars, HUD, cards, subtitles, cutscene
  staging, environment — spec §4.1 and §4.2 are currently **unmet**.
* Audio playback and sound files — the §5 bonus is currently **unmet**.
* Nothing else: the logic in §3 and §4.3 of the spec is complete.

---

## 7. Conventions

* One system per file pair (`Playfield.h/.cpp`, `Waves.h/.cpp`, …), one class per
  system, plain structs for the small value types (`Player`, `Enemy`, `Projectile`,
  `Collectible`).
* Free functions where a class would add nothing (`makePlayer`, `advancePlayer`,
  `fire`, `compileShader`).
* Naming: `PascalCase` types, `camelCase` functions and fields, `m_` private members,
  `g_` globals (host only), `SCREAMING_CASE` constants. 4-space indent, braces on
  their own line for functions.
* Comments explain **why**, not what, and quote the spec where a rule comes from.
* Files use CRLF line endings.
* Logic may not include `<glad/glad.h>` or `glm` — only `main.cpp` may.

---

## 8. Tests

`Tests/logic_tests.cpp` is a console harness with its own `main()`. It drives the real
`Game` (never a copy of the rules) one frame at a time, so a passing check means the
game itself behaves.

| Group | Covers |
|---|---|
| 1 | Main menu, cutscene, state machine |
| 2 | Lane selection, placement, locked cards |
| 3 | Jumping and ducking (§3.8) |
| 4 | Defense player as a shield (§3.2) |
| 5 | Coins, unlocking, re-locking (§3.2 / §4.3) |
| 6 | The three waves (§3.3) |
| 7 | Combat, collectibles, losing (integration) |
| 8 | Pause menu (§4.3) |
| 9 | Jump / duck through the real input path |
| 10 | Attacking with nothing in front (spec page 4) + stray-shot range |

The harness is deterministic: `Game::seedRandom(n)` fixes the wave/lane randomness,
so a failure is always reproducible. Add a case by writing a `void testX()` group,
calling it from `main()`, and using `check(condition, "description")`.

---

## 9. Requirements → code map

| Spec | Where |
|---|---|
| §2 Cutscene (10–20 s, both sides, dialogue, animation) | `Cutscene.*` (14 s timeline), skipped with `Esc` |
| §3.1 Playfield, lanes, home base, enemy entry, collisions | `Playfield.*`, `Core/Types.h`, `Game::updatePlayers/updateEnemies` |
| §3.2 Player logic, cards, costs, unlocking, shield | `Player.*`, `Economy.*`, `Game::tryPlacePlayer` / `selectCard` / `hitPlayer` |
| §3.3 Enemy logic, three types, waves, drops | `Enemy.*`, `Waves.*`, `Collectibles.*` |
| §3.4 Win / lose | `Game::checkEndConditions`, `Waves::allCleared` |
| §3.8 Per-lane defence, jump/duck, red damage feedback | `Playfield` selection, `Player::dodges`, `hitFlash` |
| §4.3 The three menus + sound settings | `Menus.*`, `Game::handleMenuAction`, `Audio.h` |
| §5 Bonus audio | `GameEvent` / `SfxId` + `SoundSystem` (backend missing) |
| §4.1–4.2 Visuals, HUD, environment | **Missing** — see §6 above |

---

## 10. Things that will bite you

* **Do not reorder the frame pipeline** (§4) — the requirements depend on the order.
* `lane` is immutable after creation; movement only ever integrates `x`.
* Characters carry no y; always derive it from `Playfield::laneCenterY(lane)`.
* `InputState` fields are **edges** lasting exactly one frame; the host clears them
  after `Game::update`. Menus read `enterPressed` / `upPressed` / `downPressed`.
* `Game::drainEvents()` **empties** the event vector — call it once per frame and keep
  what you need.
* `Economy::refreshUnlocks()` reports at most **one** unlock/relock change per call;
  loop it if you want them all.
* In release/submission terms: only `.h` / `.cpp` files belong in the submitted zip,
  so `External/`, `README.md`, the `.sln`/`.vcxproj` files and the PDF stay in the
  working folder. `glad.c` is a `.c` and is fine to submit.
