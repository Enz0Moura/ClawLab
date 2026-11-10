# LaClaw Lab

[Português (Brasil)](README.md) | **English**

A 3D claw machine simulator built with **OpenGL and freeglut**. The arm has a base, shoulder, elbow, wrist and
two articulated fingers. Control the joints, capture three aliens and release
them into the delivery tray. Capture is deterministic; the selected mode sets the
time limit, bounce strength, variation in the aliens' starting positions and tray size.

The visuals are inspired by the **Pizza Planet claw machine from Toy Story**.

## Build and run

Dependencies: a C11 compiler, Make, and OpenGL and freeglut headers/libraries.
On CachyOS/Arch: `sudo pacman -S --needed base-devel freeglut libglvnd`.
A graphical display is required to open the windows; tests do not open a window.

```sh
make lib       # build/libcg.a, independent of the application
make demo      # build build/library_demo
make example   # run the standalone example
make run       # build and run build/prize_machine
make test      # math, states, paths and allocation failures (~1 min)
make sanitize  # the same tests with Clang, ASan and UBSan
make smoke     # full automated graphical session; requires a display
make clean     # remove build/
```

`make` builds both executables. The simulator and example link against the same
`build/libcg.a`. Choose a mode in the menu with the mouse or **1–4**.
To play: click a prize; select a joint with **1–4**;
use **←/→** or **+/−**; **C** closes, **O** opens, **H** shows help.
Press **I** to make the claw follow the cursor; click an alien to start
automatic capture. Click the floor to set a target without closing the claw.
The right mouse button and wheel control the camera. **R** resets; **D** toggles
debug views. The simulator's interface is in Portuguese; the English manual
explains its labels while preserving the actual names and key bindings.
Drag with the middle mouse button to pan. **F** toggles the free camera:
right-drag looks around and scrolling moves forward or backward. **Home** resets the camera.

## Game modes

| Mode | Time | Tray (width × depth) | Starting positions | Collisions |
| --- | --- | --- | --- | --- |
| Sandbox | Unlimited | 2.50 × 1.60 | Small angular variation, constant distance | Gentle bounces for practice |
| Easy (Fácil) | 4 minutes | 2.35 × 1.40 | Small angular variation, constant distance | Light bounces and stronger damping |
| Medium (Médio) | 3 minutes | 2.20 × 1.20 | Wider angular variation and three possible distances | Moderate bounces |
| Hard (Difícil) | 2 minutes | 2.05 × 1.00 | Widest angular variation and seven possible distances | Strong bounces and weaker damping |

Dimensions use world units. The tray shrinks in width and depth while keeping its
center and height. Even on Hard, it fits three separate aliens; spawn validation
requires a delivery route with clearance from the edges. Reset keeps the current
mode's tray size, and pausing preserves the game.

Every new game or reset randomizes positions and shuffles colors among them.
Aliens start supported by the floor, separated from each other and clear of the
tray and obstacles. Each layout is checked for a route to approach, capture, lift
and transport every alien. Pausing and continuing preserves the current layout.

Released aliens bounce off the floor, cabinet and each other. Delivery counts only
once an alien settles directly on the tray, entirely inside it. **P**, **Esc** or
the **P PAUSA** button opens the pause menu and freezes the simulation and timer.
**Continuar** (continue) resumes the current game, preserving the camera, claw,
aliens and score. In the pause menu, **Resetar partida** restarts the current mode
and **Voltar aos modos** ends the game to choose another mode. **R** also restarts
during gameplay. Help freezes the timer. Victory or timeout ends the round.

## Project layout

```text
include/cg/   public library headers
src/cg/       library: math, geometries, scene, camera, rendering
src/app/      simulator: machine, alien physics, interface, configuration and theme
examples/     standalone example that uses only the library
tests/        logic and allocation-failure tests
```

