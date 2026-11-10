# javardtex
Simple quake-like game engine. Goal is to make simple singleplayer and multiplayer games.

## Why the rewrite?
I started this in June 2026 but ended up using AI for the renderer, and from there I kinda got lost and felt like I wasn't learning anything

## What will be included in this repo?
Just engine code. Actual game assets will not be present on this.

## Features
- multiplayer
- BSP 30 (valve)
- wad3 (valve)
- guns
- ez to make games

## Things it won't have (probably)
- console commands
- CVARs
- culling
- real time lightning
- realistic physics

## Goals:
- NO AI CODE
- fully made in C
- code needs to be so simple a baby can modify it
- multiplayer first
- literaly copy quake's design structure
- new games are a fork of the engine
- no separate game code

## Programming tips
- follow KISS (keep it simple stupid)
- error handling is optional
- prioritize gameplay over graphics

## Things to do over time
### January:
- Movement and Camera
- Multiplayer servers working
- Server browser
- NO GRAPHICS YET
- maybe load BSPs? (at least vertices)

### March
- BSP's
- Textures
- Lightmaps
- Simple triggers for simple gameplay
- Sound
- guns?
- AI?

### June
- AI
- Guns if that wasnt added in march
- Ready to make a full campaign

## DOCS (kind of)
- net.h: netcode function declarations
- net.c: netcode (shared between client and server)
- main.c: main code which starts the client or the server
- server.h: server specific function declarations and macros
- sv_main.c: code that manages and starts the server
