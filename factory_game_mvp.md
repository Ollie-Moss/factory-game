# MVP Design Document: Modular Automation Game

## Overview
A 2D top-down automation game inspired by Factorio and Create Mod. Players build complex machines and production lines using modular, data-driven components, powered by a standard electricity-based power grid. There is no player character; all interaction is through the world view and GUI.

---

## Core Features

### World
- Chunked tile-based 2D world (32x32 or 64x64 tiles per chunk)
- Procedural terrain with resources (ore, trees, water)
- Terrain types: Grass, Dirt, Sand, Water, Stone

### Power System
- Power Sources: Burner Generator, Steam Generator, Solar Panel
- Power Distribution: Power poles or direct wire connections
- Machines consume power per tick
- Optional power storage (battery)

### Machines
All machines are modular and can be enhanced with configurable parts.

| Machine         | Size | Power Usage | Description                                  |
|----------------|------|-------------|----------------------------------------------|
| Tree Farm Core | 3x3  | 50 W/tick   | Automates tree harvesting                    |
| Planter        | 1x1  | 20 W/tick   | Replants saplings                            |
| Furnace        | 1x1  | 30 W/tick   | Smelts ores                                  |
| Assembler      | 2x2  | 40 W/tick   | Crafts items from recipes                    |
| Inserter       | 1x1  | 5 W/tick    | Moves items between machines/belts          |
| Belt           | 1x1  | 2 W/tick    | Moves items in a direction                   |

### Modules
Modules modify machine behavior.
- Range Extender
- Timer (operation cooldown)
- Filter (output type control)
- Buffer (internal inventory)

### Resources and Items

#### Raw Materials
- Iron Ore
- Copper Ore
- Coal
- Stone
- Sand
- Wood Log

#### Processed Materials
- Iron Ingot
- Copper Ingot
- Stone Brick
- Glass
- Plank

#### Components
- Gear
- Shaft
- Plate
- Crank Handle
- Belt Segment
- Circuit Board

### Environmental Objects
- Trees (harvestable)
- Rocks (stone source)
- Bushes (decorative)
- Water (for steam)

---

## Progression Flow

1. **Start Phase**
   - Manual item collection (wood, ore)
   - Basic machine placement (tree farm, furnace)
   - Use burner generator for power

2. **Automation Phase**
   - Unlock belts, inserters, and assemblers
   - Use modules to automate farming and smelting
   - Introduce simple logic (timers, filters)

3. **Expansion Phase**
   - Build larger factory layouts
   - Introduce steam and solar power
   - Begin crafting complex components (circuits, plates)

---

## MVP Requirements Checklist

- [ ] Chunked terrain with resource generation
- [ ] Power grid simulation (sources, consumers, storage)
- [ ] Basic modular machines with config-based behavior
- [ ] Item transport system (belts and inserters)
- [ ] UI for building, inspecting, and modifying machines
- [ ] JSON-based data system for defining items, machines, and recipes
- [ ] Save/load system for chunks and machine states

---

## Stretch Goals (Post-MVP)
- Wireless power module
- Logic gates and programmable controllers
- Weather system (affects solar)
- Mod support via external JSON/asset packs

