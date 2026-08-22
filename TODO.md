# JavardTex

## Scope

Modern implementation of classic game engines for the development of FPS games.

The engine is server-authoritative. The same server code is used for local games and dedicated servers.

The client can either connect to a local server started by the engine or connect to a remote server.

Games are defined and extended through Lua.

The initial goal is to provide a solid engine for Quake/Half-Life-style FPS games. Multiplayer should be a natural extension of the server rather than a separate game mode.

## Required features

* BSP30 loading and rendering
* BSP30 collision detection
* WAD3 loading
* Entity system
* Lua scripting
* Lua API for game logic and entities
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
* Destructible environments
* AI navigation
* Decals
* Gore
* Realistic sound engine
* Multiplayer voice chat

## Runtime

The engine is one executable with different runtime modes.

Normal launch:

```
javardtex game
```

Starts both a local server and a client.

Remote server:

```
javardtex game --connect <address>
```

Starts only the client and connects to a remote server.

Dedicated server:

```
javardtex game --server
```

Starts only the server.

The local server and dedicated server use the same server code.

The difference between local and multiplayer is therefore where the server is running and how many clients are connected.

## Engine Structure

Shared engine code should not be tied to the client or server.

Client-specific code uses the `cl_` prefix.

Server-specific code uses the `sv_` prefix.

Shared gameplay and engine systems should have no prefix.

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
* Server-side Lua
* Authoritative decisions

Shared systems may be used by both sides when appropriate.

Client-side prediction may run shared simulation code locally, but the server remains authoritative.

## Games

Each game has its own directory.

```
games/
└── example/
    ├── maps/
    ├── models/
    ├── textures/
    ├── sounds/
    ├── wad/
    ├── scripts/
    ├── game.lua
    └── game.fgd
```

`game.lua` defines the game and contains information such as:

* Title
* Version
* Game settings
* Game-specific Cvars
* Lua initialization

The engine provides the Lua API used by games.

## Cvars

Cvars are part of the core engine.

Games can define Cvars through Lua.

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

The engine should eventually produce a self-contained game executable.

The exported game should contain the required:

* Engine
* Game scripts
* Lua scripts
* Assets
* Game metadata

A game should be identifiable by its game ID, version and build.

The same game executable should be usable as a client or dedicated server.

## Development Roadmap

### August 2026 — Game Foundation

Focus: define and load a game.

* [ ] Clean up the current core/client separation
* [ ] Remove unnecessary `cl_` prefixes from shared systems
* [ ] Game directory loading
* [ ] Load `game.lua`
* [ ] Game metadata
* [ ] Game title
* [ ] Game version
* [ ] Basic Lua integration
* [ ] Define initial Cvars
* [ ] Basic title screen
* [ ] Initial server structure

**Goal:** A game can be defined with `game.lua` and launched to a basic title screen.

### September 2026 — Server Runtime

Focus: make the server actually run the game.

* [ ] Server runtime
* [ ] `--server` argument
* [ ] Server game loading
* [ ] Server tick
* [ ] Local client/server connection
* [ ] Basic server-side player state
* [ ] Basic server-side movement
* [ ] BSP30 parser
* [ ] WAD3 loader
* [ ] BSP rendering
* [ ] BSP collision

**Goal:** Launch a game, start a local server and play inside a BSP map.

No server browser or player-vs-player testing is required yet.

### October 2026 — World and Entities

Focus: build the basic world framework.

* [ ] Entity system
* [ ] Entity creation
* [ ] Entity removal
* [ ] Entity properties
* [ ] Map entities
* [ ] Basic entity processing
* [ ] Basic triggers
* [ ] TrenchBroom FGD support

**Goal:** BSP maps can contain and interact with basic engine entities.

### November 2026 — Lua API

Focus: make the engine programmable.

* [ ] Lua game API
* [ ] Lua entity API
* [ ] Server-side Lua
* [ ] Client-side Lua
* [ ] Lua events
* [ ] Lua-controlled entities
* [ ] Game rules
* [ ] Server configuration through Cvars
* [ ] Initial Lua API documentation

**Goal:** A developer can create basic game logic without modifying the engine.

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

**Goal:** A client can connect to the same server code running on another machine.

### January 2027 — Prediction and Synchronization

Focus: make multiplayer technically viable.

* [ ] Multiple clients
* [ ] Player synchronization
* [ ] Client-side prediction
* [ ] Server reconciliation
* [ ] Entity interpolation
* [ ] Network error handling
* [ ] Basic multiplayer testing

**Goal:** Multiple clients can exist in the same game world with responsive movement.

### February 2027 — Rendering Systems

Focus: improve the engine's rendering capabilities.

* [ ] Material properties
* [ ] Particle system
* [ ] Sky
* [ ] Parallax sky
* [ ] Rendering improvements
* [ ] Basic visual effects

If networking is behind schedule, use this month to finish it first.

### March 2027 — Models and Animation

* [ ] glTF 2.0 loader
* [ ] Static model rendering
* [ ] Skeletal animations
* [ ] Character rendering
* [ ] Animated entities

### April 2027 — Tools and Stability

Focus: make the engine usable for actual game development.

* [ ] Developer console
* [ ] Lua API documentation
* [ ] Dedicated server improvements
* [ ] Performance profiling
* [ ] Memory improvements
* [ ] Bug fixing
* [ ] Stability improvements

Optional features may be started if the required features are complete.

### May 2027 — Packaging and Testing

Focus: prepare the engine for release.

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
* [ ] TrenchBroom FGD support

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

* Lua-defined games
* BSP30 maps
* WAD3 textures
* Entities
* Lua game logic
* Cvars
* A local server
* Remote servers
* Server-authoritative multiplayer
* Client-side prediction
* Dedicated servers
* Game packaging/export
