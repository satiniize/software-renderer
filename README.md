# Software Renderer

This project is a software renderer, designed to demonstrate graphics rendering techniques and algorithms using only CPU-based computation. It is intended for educational purposes, experimentation, and as a foundation for more advanced rendering projects.

## Features

- Pure software rendering (no GPU acceleration)
- Modular codebase for easy experimentation
- Uses [SDL](https://github.com/libsdl-org/SDL) for window management and input (as a submodule)
- Bitmap image loading and saving
- Entity Component System (ECS) architecture
- Basic physics simulation
- Sprite rendering

## Architecture

The project follows an Entity Component System (ECS) architecture. This means that game objects are represented as entities, which are essentially IDs. Each entity can have multiple components, which store data related to different aspects of the entity (e.g., position, sprite, physics). Systems operate on entities based on their components.

## Key Components

- `Bitmap`: Handles bitmap image loading, saving, and pixel manipulation.
- `EntityManager`: Manages the creation and destruction of entities.
- `ComponentStorage`: Stores components for each entity.
- `TransformComponent`: Stores the position, rotation, and scale of an entity.
- `SpriteComponent`: Stores the bitmap and size of a sprite.
- `RigidbodyComponent`: Stores the physical properties of an entity, such as velocity, gravity, and AABB (Axis-Aligned Bounding Box).
- `PhysicsSystem`: Updates the position and velocity of entities based on their `RigidbodyComponent` and `TransformComponent`.
- `SpriteSystem`: Draws sprites to the screen based on their `SpriteComponent` and `TransformComponent`.

## Getting Started

### Cloning the Repository

This project uses Git submodules. To clone the repository and initialize the SDL submodule, run:

```bash
git clone --recursive https://github.com/satiniize/software-renderer.git
```

If you already cloned the repository without `--recursive`, initialize the submodule with:

```bash
git submodule update --init --recursive
```

### Building

To build the project, run:

```bash
./build.sh
```

### Running

(Provide instructions for running the renderer once built.)

## License

(Include your license information here.)
