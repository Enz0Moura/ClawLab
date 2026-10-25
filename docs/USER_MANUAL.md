# User manual — LaClaw Lab

[Português (Brasil)](MANUAL_DO_USUARIO.md) | **English**

[Back to the README](../README.en.md)

## 1. Objective

You control an articulated arm, pick up three aliens — Coral, Azul (Blue) and
Amarelo (Yellow) — and release each one onto the green tray. An alien only counts
when it is released, at rest directly on the tray and entirely inside it. At
**3/3**, the collection is complete.

You set each joint angle yourself (forward kinematics). Clicking an alien only
highlights it; the arm does not move toward it on its own. Capture follows a fixed
geometric rule: there is no luck involved, and an alien cannot slip out while held.

The simulator's interface is in Portuguese. This guide keeps the on-screen labels,
prize names and key bindings as they appear, so you can match them in the app.

### First game

Start with Sandbox, which has no time limit:

1. Open the simulator and press **1**.
2. Click an alien to highlight it (optional; it just tells you which one you are aiming for).
3. Pick a joint with **1–4** and move it with **←/→**. With the claw open (40°) and
   the arm raised, rotate the base toward the alien, adjust the elbow and wrist,
   and only then lower the shoulder.
4. When the panel shows `Captura possivel` (capture possible), close the claw with
   **C**. The panel then shows `Garra ocupada` (claw occupied).
5. Raise the shoulder and rotate the base toward the tray (somewhere between −75° and −105°).
6. Wait for the arm to stop, then open the claw with **O**. When the alien's state
   changes to `Entregue` (delivered), the counter goes up.

Repeat with the other two. **H** opens the in-game help.

### Modes

The menu appears when the simulator starts. Click an option or press **1–4**:

| Key | Mode | Time | Tray (width × depth) | Behavior |
| --- | --- | --- | --- | --- |
| `1` | Sandbox | Unlimited | 2.50 × 1.60 | Small position variation and gentle bounces |
| `2` | Easy (Facil) | 4 minutes | 2.35 × 1.40 | Small position variation and light bounces |
| `3` | Medium (Medio) | 3 minutes | 2.20 × 1.20 | More varied angles and distances; moderate bounces |
| `4` | Hard (Dificil) | 2 minutes | 2.05 × 1.00 | Widest position variation; strong, lasting bounces |

The header shows the mode and the remaining time. The timer turns red below
30 seconds.

### Delivery tray

The tray gets smaller with each difficulty, so releases need to be more precise.
The table uses world units; the tray's center and height never change. On Hard,
its area is about 49% smaller than on Sandbox, but it still fits three aliens
side by side without stacking. The dividers drawn on the tray and the `ENTREGA`
(delivery) label resize with it.

Line each alien up with a free space, wait for the arm to stop, and only then open
the claw: releasing while moving passes the arm's velocity to the alien, and it may
go over the edge. Every generated layout is validated with at least 0.05 world
units of clearance between the alien and the tray edges.

Reset keeps the current mode's tray size. Pausing and continuing preserves the tray
and the deliveries so far.

### Alien positions

Positions are randomized on every new game and every reset, and the colors swap
places too (the blue alien is not always in the middle). All aliens start upright,
supported by the floor, without overlaps and clear of the tray and obstacles.

- **Sandbox and Easy:** constant distance from the base and little angular variation.
- **Medium:** three possible distances.
- **Hard:** seven distances and a wider angular range; you need to adjust the
  shoulder, elbow and wrist for every alien.

Each layout goes through a check that simulates the approach, capture, lift and
transport to the tray, so every alien is reachable. After capturing, raise the
arm before rotating. Use `[` and `]` for fine 1° adjustments.

### Pause and end of round

**P**, **Esc** or the **P PAUSA** button opens the pause menu. The timer, the arm
and the bounces freeze. Help also freezes the timer. Pause has three options:

- **Continuar** (`P` or `Esc`): resumes exactly where you left off — time, positions,
  velocities, selection, camera, claw opening, held alien and score. It does not
  start a new game.
- **Resetar partida** (`R`): restarts the current mode, including its timer and
  camera, and randomizes the positions again.
- **Voltar aos modos** (`M`): ends the game and opens mode selection. Choosing a
  mode starts a new game.

While paused, the header button changes to **P CONTINUAR** and the gameplay controls
are blocked. After continuing, start a new gesture to move with the arrow keys or
drag the camera again. Pausing, continuing and opening or closing help do not
re-roll the positions. **R** also restarts during gameplay.

The round ends when you deliver all three aliens or time runs out, and the screen
shows the result. Press **R** to try again, or open pause to go back to the modes.
Continuing after a result does not reopen the round. To exit, close the window or
press **Esc** on the mode selection screen.

## 2. Setup and execution

Environment the project was tested on: **CachyOS Linux (Arch-based)**, X11/XWayland
session, GCC 16.2.1, Clang 23.1.3, Make 4.4.1, freeglut 3.8.0, libglvnd 1.7.0
and an NVIDIA RTX 2060 with OpenGL 4.6 compatibility (driver 615.78.08). Windows
and macOS have not been tested. The program uses the compatibility pipeline and
does not require OpenGL 4.6.

On CachyOS/Arch, install the dependencies with:

```sh
sudo pacman -S --needed base-devel freeglut libglvnd
```

Open a terminal **in the project directory**, the one containing the `Makefile`,
and run:

```sh
make
./build/prize_machine
```

Or just use `make run`. Do not run the program with `sudo`. The window opens at
1280 × 800; I recommend at least 1100 × 720, since below that text and controls
get cramped. The camera adapts to the aspect ratio of the 3D area, and the panel
becomes more compact vertically.

To try only the library:

```sh
make lib
make example
```

The example shows an elongated cube and a cylinder that follows its parent's
rotation. In it, the arrow keys control the camera, `+`/`-` control zoom and `Esc`
exits. The simulator's controls are in the next section.

## 3. All simulator controls

| Action | Control |
| --- | --- |
| Select an available prize | Left-click directly on the alien |
| Select base / shoulder / elbow / wrist | `1` / `2` / `3` / `4` |
| Select a joint in the panel | Click the joint's row |
| Decrease / increase the angle continuously | Hold `←` / `→` |
| Decrease / increase the target by 5° | Tap `-` / `+` (or `=` to increase) |
| Adjust the target by −1° / +1° | `[` / `]` |
| Adjust by 5° with the mouse | `−` / `+` buttons in the joint's row |
| Close the claw | `C` or the **C FECHAR** button |
| Open and release the prize | `O` or the **O ABRIR** button |
| Toggle close / open | `Space` |
| Orbit the camera | Drag with the right mouse button in the scene |
| Pan the camera horizontally and vertically | Drag while pressing the wheel (middle mouse button) |
| Toggle orbital / free camera | `F` |
| Zoom in / out | Mouse wheel in the scene |
| Reset the camera | `Home` |
| Help | `H`, `F1` or the **H AJUDA** button |
| Debug views | `D` |
| Reset the scene and camera | `R` or the **R REINICIAR** button |
| Pause / continue | `P`, `Esc` or the **P PAUSA** / **P CONTINUAR** button |
| Restart from pause | **R RESETAR PARTIDA** or `R` |
| Return to mode selection | **M VOLTAR AOS MODOS** or `M`, while paused |
| Exit | Close the window or `Esc` on the mode selection screen |

Letter keys work in both uppercase and lowercase. For the 5° and 1° increments,
release the key after each step: holding `+` does not repeat. For continuous
movement, use the arrow keys; releasing them stops the movement. Camera and
selection use different mouse buttons so they do not get mixed up, and while help
is open you cannot select objects behind its panel.

In free mode, the right button looks around from the current position, and the
wheel moves forward or backward along the viewing direction. Middle-button
dragging pans in both modes. Start gestures inside the scene: the panel and the
help overlay do not start camera movement. Releasing the button ends the gesture.
`Home` and `R` restore the initial position and orbital mode. The free camera can
pass through the cabinet and look below the horizon.

## 4. What appears on screen

**Scene:** the machine follows the Pizza Planet theme from Toy Story: a red
rocket, yellow top, side fins, blue band and stars. The arm is metallic with red
joints. The prizes are green aliens with three eyes and an antenna, wearing
**Coral, Azul (Blue) and Amarelo (Yellow)** uniforms that match their on-screen
labels. The green platform with three spaces is the delivery tray. The dark opening
on the outside of the rocket is decoration only. The front is open so you can see
the arm.

**Selection:** a lime green outline follows the selected alien, and its row is
highlighted in the list. Clicking empty space, or a surface that blocks the
alien, clears the selection. Selecting does not extend the claw's reach.

**Crew (Tripulação):** shows the count and each alien's state: `Disponivel`
(available), `Na garra` (held by the claw), `Em movimento` (moving) or `Entregue`
(delivered). A delivered alien stays visible on the tray but cannot be captured
again. Its label disappears from the scene so it does not cover the others.

**Joints:** the lime green row is the active joint. The large number is the
**current** angle in degrees, and the range in brackets is its limit. Angles change
gradually after you press the buttons.

| Joint | Initial | Limits |
| --- | ---: | ---: |
| Base | 0° | −110° to 65° |
| Shoulder (Ombro) | −25° | −85° to 25° |
| Elbow (Cotovelo) | −85° | −135° to −15° |
| Wrist (Punho) | −70° | −110° to 80° |
| Fingers | 40°, open | 0° to 40° |

A collision can stop movement before the angular limit. The range is the joint's
capacity; it does not guarantee that any combination of angles fits inside the
cabinet.

**Claw:** the bar shows how far it is open. When capturing, the fingers stop at
around 20°, without crushing the alien. The text below the bar shows the
situation: `Aproxime a garra aberta` (bring the open claw closer),
`Captura possivel` (capture possible, alignment is good enough), `Garra ocupada`
(the alien follows the arm) and `Colecao completa!` (collection complete).

**Footer:** reports capture, release, joint limit, blocked movement and delivery.
`Movimento bloqueado` (movement blocked) means the latest attempt to advance was
rejected; move the joint in the opposite direction or adjust another joint to get
out of that position.

**Debug (D):** shows the collision boxes, colored axes, the claw's position and
the number of rejected moves. The pink box is the tolerance for the alien's
**center**, the green volume is the delivery region and the outer box marks the
cabinet limits. It is useful for studying the simulation and is not needed to play.

## 5. Game rules

- Capture requires an empty claw, an available alien near the claw's center, a
  compatible size and a clear path. Closing from far away captures nothing.
- The fingers must be open to receive the alien. Selection is optional for capture:
  what matters is real proximity, not the highlighted name.
- Once captured, the alien follows the claw's position and orientation, with no
  alignment snap. Closing stops when the gap is enough to hold it.
- The arm and its load cannot pass through the floor, back wall, columns, barrier,
  tray, other aliens or the cabinet's internal limits. The open sides let you look
  inside, but they are still limits of the play area.
- Some collision boxes are larger than the visible shape when parts are tilted, so
  movement may be blocked slightly before apparent contact.
- Opening the claw releases the alien where it is, with the arm's velocity. It
  keeps its orientation, falls under gravity and bounces off the floor, cabinet,
  arm and other aliens. Difficulty sets the bounce strength and the damping. Wait
  for the `Disponivel` state before trying to recapture a moving alien.
- To count, the alien must come to rest directly on the tray and fit entirely
  inside it. Passing over the tray while held does not count, and neither does
  landing on the edge or on top of another alien.
- A delivery can only be undone by a restart, and each alien counts once.
- `R` re-rolls the positions and restores the aliens, angles, selection, count,
  camera, active joint and the current mode's timer. Help and debug keep whatever
  state they were in.

## 6. Troubleshooting

**“GL/freeglut.h: No such file” or “cannot find -lglut”:** the freeglut headers
or library are missing. Install the packages from section 2 and build again.

**“failed to open display”:** run in a local graphical session with access to the
display. A remote terminal without graphical forwarding cannot open the window.
`make test` and `make sanitize` work without a window.

**The window opens, but the arm does not respond:** click the window to focus it,
select a joint with `1`–`4` and use the arrow keys. For increments, release and
press the key again. Check whether the footer reports a block or a limit.

**I cannot select an alien:** click the alien, not the label floating over it.
Close help. A column, part of the arm or another alien may be in front of it;
orbit with the right mouse button. `Home` restores the initial view.

**The claw closes without grabbing anything:** open with `O` and wait for it to
reach 40°. Adjust the joints until `Captura possivel` appears. If the claw is
already too closed, the alien does not fit.

**An alien is hard to pick up after falling:** its orientation is preserved, and
some tilted positions do not fit in the claw. Reposition the claw or use `R`; a
restart always returns all three aliens to reachable positions.

**An alien is on the tray, but it did not score:** it must be released, resting
directly on the tray, stationary and entirely inside it. Wait for the bounces to
stop. A stack or the edge does not count as a delivery.

**Text is too small or overlaps:** enlarge the window to 1280 × 800. Interface
text has no accents, for compatibility with freeglut's bitmap font; this does not
mean the installation is broken.

## 7. Limitations and tests

This is an educational forward kinematics simulation. It has no IK, realistic
finger physics, self-collision between arm segments, plush deformation, sound,
persistence between runs or saved games. The camera can show the back of parts,
and there is no transparent glass or cast shadows. Collisions use conservative
boxes and do not rotate the aliens.

Run the logic and memory tests with:

```sh
make test       # takes about a minute
make sanitize   # the same tests with Clang, ASan and UBSan
```

They cover delivery of all three aliens in every mode, bounces, collisions between
aliens, timeout, reset and allocation failures. They also walk through 24 seeds
per mode, with all six collection orders, floor support, no overlaps and variation
that grows with difficulty. For the tray, they check that it shrinks with each
mode, keeps its size on reset, leaves clearance for all three deliveries and does
not score an alien that is partly over the edge.

The position generator uses forward kinematics poses, a bounded number of retries
and a safe fallback layout that is validated too. `machine_create_seeded(mode, seed)`
reproduces a layout in the tests; in a normal game, the seed is picked
automatically.

To run the automated graphical session, which needs a display:

```sh
make smoke
```

It opens and closes the example and then the simulator, exercises the controls,
compares the state before and after pausing (including while holding an alien and
with aliens moving), tests returning to the modes and resetting, and saves
screenshots to `build/smoke_*.ppm`. It is only a testing tool; normal gameplay,
started with `make run`, is entirely manual.
