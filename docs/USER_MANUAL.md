# User manual — LaClaw Lab

[Português (Brasil)](MANUAL_DO_USUARIO.md) | **English**

[Back to the README](../README.en.md)

## 1. Objective

Pick up all three prizes — Coral, Azul (Blue) and Amarelo (Yellow) — with the
articulated arm and release them into the green tray. Each prize counts once,
after settling directly on the tray while released. At **3/3**, the collection is complete.
Sandbox has no time limit; challenges have a countdown. Capture is not random,
and a prize cannot slip out while held.

The player controls the joint angles. Clicking a prize only highlights it;
the arm does not move toward it automatically.

The simulator's interface is in Portuguese. This guide preserves the actual
on-screen labels, prize names and key bindings so you can find them in the app.

### Choose a mode

The menu appears when the simulator starts. Click an option or press **1–4**:

| Key | Mode | Time | Tray (width × depth) | Behavior |
| --- | --- | --- | --- | --- |
| `1` | Sandbox | Unlimited | 2.50 × 1.60 | Small position variation and gentle bounces |
| `2` | Easy (Facil) | 4 minutes | 2.35 × 1.40 | Small position variation and light bounces |
| `3` | Medium (Medio) | 3 minutes | 2.20 × 1.20 | More varied angles and distances; moderate bounces |
| `4` | Hard (Dificil) | 2 minutes | 2.05 × 1.00 | Widest position variation; strong, lasting bounces |

The green delivery platform shrinks with each difficulty, requiring more precise
releases. The table uses world units; the tray's center and height stay the same.
On Hard, its area is about 49% smaller than in Sandbox, but it still fits three
separate deliveries without stacking. The dividers resize with the tray. Align
each alien with a free space, wait for the arm to stop, then open the claw:
releasing while moving transfers velocity and may send it past the edge.
The validated route for each spawn leaves at least 0.05 world units between the
alien's box and the tray edges. Reset keeps the chosen mode's tray size;
pausing and continuing preserves the tray and current deliveries.

Alien positions are randomized for every new game and reset. Colors also swap
places: the blue alien, for example, is no longer always in the center. Sandbox
and Easy keep a constant distance from the base with small angular variation.
Medium offers three possible distances; Hard offers seven and a wider angular
range, requiring adjustments to the shoulder, elbow and wrist too.

All aliens start upright, supported by the floor, without overlaps and clear of
the tray and obstacles. Every layout is checked for an approach, capture, lift
and transport route. Approach by rotating the base with the claw raised, adjusting
the elbow and wrist, then lowering the shoulder. After capturing, lift the arm
before rotating toward the tray. Use `[` and `]` for fine 1° alignment.

During gameplay, **1–4** select joints again. The header shows the mode and
remaining time; the timer turns red below 30 seconds. **P**, **Esc** or the
**P PAUSA** button opens the pause menu. The countdown, arm movement and alien
bounces freeze. The pause menu offers three options:

- **Continuar** (`P` or `Esc`): resumes the same remaining time, positions,
  velocities, selection, camera, claw opening, held prize and score. It does not create a new game.
- **Resetar partida** (`R`): restarts the current mode, including its timer and camera,
  and randomizes the aliens' positions again.
- **Voltar aos modos** (`M`, while paused): ends the current game and opens mode
  selection. Choosing a mode starts a new game.

The header button changes to **P CONTINUAR** while paused. Gameplay controls behind
the overlay are blocked; to move with the arrow keys or drag the camera again,
start a new gesture after continuing. Help also freezes the timer.
Pausing, continuing and opening or closing help do not reroll the positions.
**R** remains available to restart during gameplay.
Delivering all three aliens or running out of time stops the round and displays
the result. Use **R** to retry or open pause to return to mode selection. Continuing
after a result keeps the round finished. To exit, close the window or press
**Esc** on the mode selection screen.

## 2. Setup and execution

The verified environment was **CachyOS Linux (based on Arch)**, in a session
with X11/XWayland display `:0`, GCC 16.2.1, Clang 23.1.3, Make 4.4.1,
freeglut 3.8.0, libglvnd 1.7.0 and an NVIDIA RTX 2060 with OpenGL 4.6
compatibility, driver 615.78.08. Windows and macOS support has not been tested.
The program uses compatibility pipeline features; it does not require OpenGL 4.6.

On CachyOS/Arch, install the required packages with:

```sh
sudo pacman -S --needed base-devel freeglut libglvnd
```

These dependencies were already installed during development; the command
above shows how to obtain them. Open a terminal **in the project directory**,
the one containing `Makefile`, and run:

```sh
make
./build/prize_machine
```

Or simply use `make run`. Do not run the program with `sudo`.
The window opens at 1280 × 800. At least 1100 × 720 is recommended; below that,
text and controls may become crowded. The camera adapts to the aspect ratio
of the 3D area, and the panel becomes more compact vertically.

To try just the library:

```sh
make lib
make example
```

The example shows an elongated cube and a cylinder that follows its parent's
rotation. In that example, the arrow keys control the camera, `+`/`-` control
zoom and `Esc` exits. The simulator uses the controls in the next section.

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
| Pause / continue | `P`, `Esc` or **P PAUSA** / **P CONTINUAR** |
| Restart from pause | **R RESETAR PARTIDA** or `R` |
| Return to mode selection | **M VOLTAR AOS MODOS** or `M`, while paused |
| Exit | Close the window or `Esc` on the mode selection screen |

Letter keys work in both uppercase and lowercase. For 5° and 1° increments,
press and release the key for each step; holding `+` does not repeat the steps.
Use the arrow keys for continuous movement. Releasing them stops that movement.
Camera and selection actions use different mouse buttons to avoid mixing them.
While help is open, objects behind the help panel cannot be selected.

In free mode, drag with the right mouse button to look around from the current
position; scrolling moves forward or backward along the viewing direction.
Middle-button dragging pans in both modes. Start gestures inside the scene;
the panel and help overlay do not start camera movement. Releasing the button
ends the gesture. `Home` and `R` restore the initial position and orbital mode.
The free camera can pass through the cabinet and look below the horizon.

## 4. What appears on screen

**Scene:** the machine has a Pizza Planet theme inspired by the Toy Story
reference: a red rocket, yellow top, side fins, blue band and stars. The metal
arm has red joints. The three prizes are green aliens with three eyes, an
antenna and uniforms in **Coral, Azul (Blue) and Amarelo (Yellow)**, matching
their on-screen labels; all three have green skin. The green platform with three
spaces is the delivery tray. The dark opening on the outside of the rocket is
decorative. The open front lets you see the arm.

**Selection:** a lime green outline follows the selected alien, and its row
is highlighted in the collection. Clicking empty space or a surface that
blocks a prize clears the selection. Highlighting does not extend the claw's reach.

**Crew:** displays the count and states `Disponivel` (available), `Na garra`
(held by the claw), `Em movimento` (moving) or `Entregue` (delivered). A delivered
prize remains visible in the tray but cannot be captured again. Its label in
the scene disappears to avoid obscuring other prizes.

**Joints:** the lime green row is the active joint. The large number is the
**current** angle in degrees; the range in brackets is its limit. Angles change
gradually after pressing the buttons. Wait for the desired value before
moving to the next step of the walkthrough.

| Joint | Initial | Limits |
| --- | ---: | ---: |
| Base | 0° | −110° to 65° |
| Shoulder (Ombro) | −25° | −85° to 25° |
| Elbow (Cotovelo) | −85° | −135° to −15° |
| Wrist (Punho) | −70° | −110° to 80° |
| Fingers | 40°, open | 0° to 40° |

A collision can stop movement before the angular limit. The range describes
the joint's capacity; it does not guarantee that every combination of joint
angles fits inside the cabinet.

**Claw:** the bar indicates how far the claw is open. During capture, the
fingers stop at around 20°, without crushing the alien. `Captura possivel`
(capture possible) indicates sufficient alignment; `Garra ocupada` (claw
occupied) confirms that the prize is following the arm.

**Footer:** reports capture, release, blocked movement and delivery.
`Movimento bloqueado` (movement blocked) means the latest small movement
attempt was rejected. Move the joint in the opposite direction or adjust
another joint to leave that position.

**Debug (D):** shows boxes, colored axes, the claw's position and the number
of rejected moves. The pink box marks the tolerance for the prize's **center**.
The green volume marks the delivery region. The outer box marks the cabinet
limits. This mode is useful for studying the simulation and is not required to play.

## 5. Game rules

- Capture requires an empty claw, an available prize near the claw's center,
  a compatible size and an unobstructed path. Closing far away captures nothing.
- The fingers must be open to receive the alien. Selection is optional for
  capture: actual proximity matters, not the highlighted name.
- Once captured, the prize follows the claw's position and orientation without
  snapping into alignment. Closing stops when the gap is small enough to hold it.
- The arm and its load cannot pass through the floor, back wall, columns,
  barrier, tray, other prizes or the cabinet's internal limits. The open sides
  let you look inside, but they still bound the playing area.
- Some boxes are larger than the visible shape when parts are tilted.
  Movement may be blocked slightly before apparent contact.
- Opening releases the prize at its current position with the arm's velocity.
  It keeps its orientation, falls under gravity and bounces off the floor,
  cabinet, arm and other aliens. Difficulty controls bounce strength and damping.
  Wait for `Disponivel` before trying to recapture a moving alien.
- To count, the prize must settle directly on the tray and fit entirely inside
  it. Passing over it while held does not count. Landing partly on the edge
  or on another prize does not count either.
- Delivery is irreversible until reset. Each prize increments the count once.
- `R` generates new spawns and restores prizes, angles, selection, count, camera,
  active joint and the current mode's timer.
  Your current help and debug visibility preferences are preserved.

## 6. Troubleshooting

**“GL/freeglut.h: No such file” or “cannot find -lglut”:** the freeglut headers
or library are missing. Install the packages from section 2 and build again.

**“failed to open display”:** run in a local graphical session with access
to the display. A remote terminal without graphical forwarding cannot open
the window. `make test` and `make sanitize` still work without a window.

**The window opens, but the arm does not respond:** click to focus the window,
select a joint with `1`–`4` and use the arrow keys. For increments, release and
press again. Check whether the footer reports blocked movement or a limit.

**I cannot select a prize:** click the alien, not its floating label. Close
help. A column, part of the arm or another prize may be in front of it;
orbit with the right mouse button. `Home` restores the initial view.

**The claw closes without grabbing:** open with `O` and wait for 40°. Adjust
the joints until `Captura possivel` appears. If the claw is already too closed,
it cannot receive the alien.

**A prize is hard to pick up after falling:** its orientation is preserved,
and some tilted positions do not fit the claw. Reposition the claw or use `R`.
Resetting always returns all three prizes to usable positions.

**A prize is in the tray, but there is no point:** it must be released,
resting directly on the tray, stationary and entirely inside it. Wait for
the bounces to stop. A stack or the edge does not count as a delivery.

**Text is too small or overlaps:** enlarge the window to 1280 × 800.
Interface text omits accents for compatibility with freeglut's bitmap font;
this does not indicate an installation problem.

## 7. Limitations and verification

This is an educational forward kinematics simulation. It does not include IK,
realistic finger physics, self-collision between segments, plush deformation,
sound, persistence between runs or saved games. The camera may show the back
of parts; there is no transparent glass or cast shadows.
Collisions use conservative boxes and do not rotate aliens.

Tests cover delivery of all three aliens in every mode, bounces, collisions
between prizes, timeout, reset and allocation failures. They also check 24 seeds
per mode, covering all six collection orders, floor support, absence of overlaps
and increased variation with difficulty. Tests also verify progressive tray
shrinkage, size preservation after reset, clearance for all three deliveries
and rejection of points for an alien partly over the edge. The generator uses forward kinematics
poses, bounded retries and a safe fallback layout that is also validated.
`machine_create_seeded(mode, seed)` reproduces a layout in tests; normal gameplay
chooses a new seed automatically. The graphical test
compares state before and after pausing, including while holding a prize and
with moving aliens, and verifies returning to modes and resetting. Run the
logic and memory tests with `make test` and `make sanitize`.

To reproduce the automated verification session:

```sh
make smoke
```

It opens and closes the example and then the machine, exercises the controls
and saves actual captures to `build/smoke_*.ppm`. It is a testing tool; normal
gameplay, started with `make run`, remains entirely manual.
