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
- ez to make cool games on
- better than unreal engine

## Things it won't have (probably)
- console commands
- CVARs
- culling
- real time lightning
- realistic physics

## Goals:
- NO AI CODE
- fully made in C
- code needs to be so ez to understand a baby can modify it
- multiplayer first
- new games are a fork of the engine
- no separate game code

## Programming tips
- KISS (keep it simple stupid)

## Things to do over time
### January:
- Movement and Camera
- Multiplayer servers working
- Server browser
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

### how to open:
./game server ip port maxplayers gamemode
./game join ip port
