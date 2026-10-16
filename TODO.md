# JavardTex

## Required features

* BSP30 loading and rendering
* BSP30 collision detection
* WAD3 loading
* Entity system
* Cvars
* Developer console
* Material properties
* Particle system
* glTF 2.0 loading
* Skeletal animations
* Parallax sky
* Game packaging/export

## Optional features

* Planar reflections
* RayTracing
* Destructible environments
* AI navigation
* Decals
* Gore

## Engine Structure

```
JavardTex/
├── src/
├── include/
├── game/
├── Makefile
└── README.md
```

C source files are stored in `src/`.

Headers are stored in `include/`.

Game assets are stored in `game/`.

## Development Roadmap

### August 2026

* [x] Clean up the current core/client separation
* [x] Remove unnecessary `cl_` prefixes from shared systems
* [x] Game directory loading
* [x] Game definition
* [x] Game metadata
* [x] Game title
* [x] Game version
* [x] Basic game configuration
* [x] Basic title screen

### September 2026

* [x] BSP30 parser
* [x] WAD3 loader
* [x] BSP rendering
* [x] BSP collision

### October 2026

* [x] Map entities
* [x] Basic entity processing
* [x] Basic triggers
* [x] TrenchBroom FGD support

### November 2026

* [ ] Player gameplay
* [ ] Damage system
* [ ] Game entity definitions

### December 2026

* [ ] Console commands
* [ ] Game rules
* [ ] Game-specific Cvars

### January 2027

* [ ] Weapons
* [ ] Enemy AI

### February 2027

* [ ] Material properties
* [ ] Parallax sky
* [ ] Particle system
* [ ] Rendering improvements
* [ ] Basic visual effects

### March 2027

* [ ] glTF 2.0 loading
* [ ] Static model rendering
* [ ] Skeletal animations
* [ ] Character rendering
* [ ] Animated entities

### April 2027

* [ ] Game packaging
* [ ] Game export
* [ ] Game jam
* [ ] Bug fixing

### May 2027

* [ ] Example game
* [ ] Bug fixing
* [ ] Tech demo
* [ ] Testing
* [ ] Documentation
* [ ] Release candidate

### June 2027

* [ ] Final testing
* [ ] Final documentation
* [ ] JavardTex 1.0 release

## Tools

### Required

* [ ] WAD3 creation tool

## Quality

* ISO C99
* 4-space indentation
* Compile with `-Wall`, `-Wextra` and `-Werror`
* Prefer small patches over large patches
* Each patch should solve one problem
* Avoid unnecessary abstractions
* Keep the codebase buildable

## Use of AI
Code that isnt understandable by humans should not be present on the final release. AI writen code can be included meanwhile to skip certain development steps with the goal of completing other goals first in order to speed up development. Said code must be marked as AI written so developers can comeback later and reimplement.
