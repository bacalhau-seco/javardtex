# JavardTex

## Scope
Modern implementation of classic Game Engines for the Development of FPS games.
The engine will have scripting capabilities to design the games. The goal is an efficient engine that can provide stable, fast and interesting games.

Required features:
- BSP30 (BSP version used by half life) loading and rendering, complete with collisions and entities
- Texture loading from WAD3 files
- Entity system
- Lua scripting for entities and game logic
- Multiplayer networking
- Multiplayer Text chat
- Multiplayer voice chat
- Diferent Material Propreties
- Skeletal animations
- Particle system
- glTF 2.0 loading
- Paralax sky
- Cvars
- Developer Console to edit Cvars
- Export game as a single binary

Other features, not required for final release include:
- Planar Reflections (for materials)
- Destructible enviornments (as brush entities)
- AI mesh navigation
- Decals (blood splater, grafitty, bullet holes...)
- Gore
- Realistic Sound Engine

## Deliverables
- LUA api Documentation
- WAD3 creation software
- Tech demo with all engine features
- Server
- Client

## Constraints
- minimal dependencies
- written in C99
- nothing proprietary

## Coding Standards

- ISO C99
- 4-space indentation

## Engine structure
javardtex/
├── docs/
├── include/
├── lib/
├── src/
├── tools/
├── games/
├── LICENSE
├── Makefile
├── README.md
└── TODO.md

For each game then there is a diferent file structure:
├── games/
│   ├── example/
│   │   ├── maps/
│   │   ├── models/
│   │   ├── textures/
│   │   ├── sounds/
│   │   ├── wad/
│   │   ├── game.lua
│   │   └── game.fgd

## Quality
Quality is ensured by always compiling with `-Werror`, `-Wall` and `-Wextra`.

Small patches are preferred over large patches.

Each patch should solve a single problem, making review easier and helping maintain code quality, safety, performance and stability.

## Timeline
The timeline includes only required features, non required features may be added when time is available if the planned required features have been added.

### July 2026
- Repository structure
- Core engine architecture
- File system
- Camera
- Player movement

### August 2026
- Game parsing
- Lua integration
- Cvars
- Start developing server

### September 2026
- BSP30 parser
- WAD3 loader
- WAD3 creation tool
- BSP30 rendering
- Collision detection

### October 2026
- Developer console
- Basic game loading
- TrenchBroom FGD

### November 2026
- Entities
- Lua entity API
- Trigger entities

### December 2026
- characters
- items
- weapons
- events
- triggers

### January 2027
- Particle system
- Parallax sky
- Material system

### February 2027
- glTF 2.0 loader
- Static model rendering
- skeletal animations

### March 2027
- Finish adding required features if late
- work on optional features

### April 2027
- Optimization
- Bug fixing
- Feature freeze

### May 2027
- Testing
- Polish
- UI
- exporting games as binaries
- Release candidate

### June 2027
- release version 1.0
