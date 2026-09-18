# JavardTex

## Scope

Modern implementation of classic game engines for the development of FPS games.

The engine is server-authoritative. The same server code is used for local games and dedicated servers.

The client can either connect to a local server started by the engine or connect to a remote server.

Games are defined through C and engine configuration. The engine is designed specifically for Quake/Half-Life-style FPS games rather than arbitrary game types.

The initial goal is to provide a solid engine for Quake/Half-Life-style FPS games. Multiplayer should be a natural extension of the server rather than a separate game mode.

## Required features

* BSP30 loading and rendering
* BSP30 collision detection
* WAD3 loading
* Entity system
* Cvars
* Developer console
* Server-authoritative networking
* Client-side prediction
* Multiplayer text chat
* Material properties
* Particle system
* glTF 2.0 loading
* Skeletal animations
* Parallax sky
* Game packaging/export
* Dedicated server

## Optional features

* Planar reflections
* RayTracing
* Lua scripting
* Destructible environments
* AI navigation
* Decals
* Gore
* Realistic sound engine
* Multiplayer voice chat

## Engine Structure

```
JavardTex/
├── src/
├── include/
├── assets/
├── Makefile
└── README.md
```

C source files are stored in `src/`.

Headers are stored in `include/`.

Game assets are stored in `assets/`.

The engine should remain divided conceptually into shared, client, server and game code without unnecessary separation into libraries or plugins.

Client-specific code uses the `cl_` prefix.

Server-specific code uses the `sv_` prefix.

Shared engine and gameplay systems have no prefix.

The client is responsible for:

* Rendering
* Input
* Camera
* Audio
* Presentation

The server is responsible for:

* Game state
* Game logic
* Entities
* Physics
* Server-authoritative decisions

Shared systems may be used by both sides when appropriate.

Client-side prediction may run shared simulation code locally, but the server remains authoritative.

## Games

The engine is designed around a common FPS gameplay model so that creating another game primarily requires changing:

* Game name
* Game version
* Assets
* Maps
* Game configuration

Game-specific code can be changed when different gameplay behaviour is required.

The example game is shipped with JavardTex and serves as the primary demonstration and test game.

The game should be identifiable by its game ID, version and build.

## Cvars

Cvars are part of the core engine.

Games can define game-specific Cvars through C code.

Selected Cvars can be exposed to server administrators through server configuration.

Cvars may later be replicated from the server to clients when required by gameplay or client-side prediction.

## Networking

The server is authoritative.

Clients send input and commands to the server.

The server simulates the game and sends the resulting state to clients.

The networking system should eventually support:

* Client/server connection
* Connection handshake
* Client commands
* Server snapshots
* Entity synchronization
* Reliable messages
* Unreliable messages
* Client-side prediction
* Client reconciliation
* Multiplayer text chat

Networking should be developed incrementally. A complete multiplayer system is not required before the local server is functional.

## Build and Distribution

The engine and game are compiled into a single executable.

The exported game should contain the required:

* Engine
* Game code
* Assets
* Game metadata

The example game is shipped with the engine.

The same game executable should be usable as a client or dedicated server.

Game packaging should allow the engine to identify the game by its game ID, version and build.

## Development Roadmap

### August 2026 — Game Foundation

* [x] Clean up the current core/client separation
* [x] Remove unnecessary `cl_` prefixes from shared systems
* [x] Game directory loading
* [x] Game definition
* [x] Game metadata
* [x] Game title
* [x] Game version
* [x] Basic game configuration
* [x] Basic title screen
* [x] Initial server structure

### September 2026 — Server Runtime

* [ ] Server runtime
* [ ] `--server` argument
* [ ] Server game loading
* [ ] Server tick
* [ ] Local client/server connection
* [ ] Basic server-side player state
* [ ] Basic server-side movement
* [x] BSP30 parser
* [x] WAD3 loader
* [x] BSP rendering
* [ ] BSP collision

### October 2026 — World and Entities

* [ ] Entity system
* [ ] Entity creation
* [ ] Entity removal
* [ ] Entity properties
* [ ] Map entities
* [ ] Basic entity processing
* [ ] Basic triggers
* [ ] TrenchBroom FGD support

### November 2026 — Game Logic

* [ ] Player gameplay
* [ ] Weapons
* [ ] Damage system
* [ ] Enemy entities
* [ ] Enemy AI
* [ ] Game rules
* [ ] Game-specific Cvars
* [ ] Game entity definitions

### December 2026 — Core Networking

Focus: make the existing local server communicate through the network properly.

* [ ] Client/server protocol
* [ ] Connection handshake
* [ ] Client commands
* [ ] Server snapshots
* [ ] Basic entity synchronization
* [ ] Reliable messages
* [ ] Unreliable messages
* [ ] Remote server connection

### January 2027 — Prediction and Synchronization

* [ ] Multiple clients
* [ ] Player synchronization
* [ ] Client-side prediction
* [ ] Server reconciliation
* [ ] Entity interpolation
* [ ] Network error handling
* [ ] Basic multiplayer testing
* [ ] Multiplayer text chat

### February 2027 — Rendering Systems

* [ ] Material properties
* [ ] Particle system
* [ ] Sky
* [ ] Parallax sky
* [ ] Rendering improvements
* [ ] Basic visual effects

### March 2027 — Models and Animation

* [ ] glTF 2.0 loading
* [ ] Static model rendering
* [ ] Skeletal animations
* [ ] Character rendering
* [ ] Animated entities

### April 2027 — Tools and Stability

* [ ] Developer console
* [ ] Dedicated server improvements
* [ ] Performance profiling
* [ ] Memory improvements
* [ ] Bug fixing
* [ ] Stability improvements
* [ ] Example game improvements

### May 2027 — Packaging and Testing

* [ ] Game packaging
* [ ] Game export
* [ ] Client/server distribution
* [ ] Example game
* [ ] Tech demo
* [ ] Testing
* [ ] Documentation
* [ ] Release candidate

### June 2027 — Release

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

## 1.0 Goal

JavardTex 1.0 should provide a stable engine capable of running FPS games with:

* C games
* BSP30 maps
* WAD3 textures
* Entities
* C game logic
* Cvars
* A local server
* Remote servers
* Server-authoritative multiplayer
* Client-side prediction
* Dedicated servers
* Game packaging/export
* A shipped example game

## Use of AI
Code that isnt understandable by humans should not be present on the final release. AI writen code can be included meanwhile to skip certain development steps with the goal of completing other goals first in order to speed up development. Said code must be marked as AI written so developers can comeback later and reimplement.
