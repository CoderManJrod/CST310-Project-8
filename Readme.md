# Project 8 — Pac-Man 2.5D: A Playable OpenGL Demo Scene

**Course:** CST-310 — Computer Graphics and Visualization
**Institution:** Grand Canyon University
**Authors:** Jared Walker & James Hohn
**Instructor:** Ricardo Citro
**Date:** March 28, 2026
**GitHub Repository:** CST310-Project-8

---

## Overview

This project is a fully playable 2.5D Pac-Man game built in C++ and OpenGL. The maze is rendered in 3D perspective with extruded wall geometry, a lit floor, and full entity rendering for Pac-Man and four ghosts. All character meshes are built from spherical coordinate math. The ghost AI uses a target-based personality system developed collaboratively.

---

## Features

- Playable Pac-Man with smooth interpolated movement and input buffering
- Animated mouth open/close driven by a sine oscillator
- Four ghosts with distinct AI personalities (chase, flank, intercept, random)
- Ghost state machine: house, leaving, active, eaten
- Staggered ghost release (every 10 seconds)
- Power pellets with frightened mode and ghost eating
- Bobbing regular pellets and pulsing power pellets
- 3D extruded maze walls with per-face lighting
- Two-light OpenGL lighting setup
- HUD displaying score and lives
- Win and game over states with restart

---

## Controls

| Key | Action |
|-----|--------|
| Arrow Keys | Move Pac-Man |
| R | Restart game |
| ESC | Quit |

---

## Compile &amp; Run

```bash
g++ pacman.cpp -o pacman -lGL -lGLU -lglut -std=c++17
./pacman
```

---

## Software Requirements

- **OS:** Ubuntu 22.04 LTS or later
- **Compiler:** g++ with C++17 support
- **Libraries:** FreeGLUT, Mesa OpenGL, GLU

Install dependencies:

```bash
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev -y
```

---

## File Structure

```
project8/
├── pacman.cpp         # Full source code
├── instructions.txt   # Run instructions and software requirements
├── README.md          # This file
└── screenshots/
    ├── gameplay.png
    ├── frightened.png
    └── win_screen.png
```

---

## Mathematical Functions Used

| Function | Purpose |
|----------|---------|
| x = R·sin(φ)·cos(θ), y = R·cos(φ), z = R·sin(φ)·sin(θ) | Spherical mesh for Pac-Man and ghosts |
| mouthAngle += 3.5 · mouthDir | Oscillating mouth animation |
| x += (gx - x) · speed | Linear interpolation for smooth movement |
| 0.85 + 0.15 · sin(powerPulse) | Power pellet size and brightness pulse |
| min distance to target cell | Ghost AI direction selection |

---

## Video Demo

[Insert Loom / YouTube / Vimeo link here]

---

## References

- de Vries, J. (2021). *Basic lighting*. LearnOpenGL. https://learnopengl.com/Lighting/Basic-Lighting
- Khronos Group. (2024). *OpenGL wiki*. https://www.khronos.org/opengl/wiki
- Namco. (1980). *Pac-Man* [Arcade game]. Namco.
- Shreiner, D., et al. (2013). *OpenGL programming guide* (8th ed.). Addison-Wesley.
- Angel, E., &amp; Shreiner, D. (2015). *Interactive computer graphics* (7th ed.). Pearson.
