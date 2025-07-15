#ifndef CHUNK_H
#define CHUNK_H

#include "../../engine/ecs/IComponent.hpp"
#include "../entities/TileEntity.hpp"
#include "ChunkTransform.hpp"
#include "../utils/GeneratorSettings.hpp"

struct ChunkRenderer;

struct Chunk : IComponent {
	ChunkTransform *transform;
	ChunkRenderer *renderer;

	std::vector<TileEntity *> tiles;
	bool Generated = false;

	Chunk(ChunkRenderer *renderer, ChunkTransform *transform) : renderer(renderer),
																transform(transform) {};

	void Generate(GeneratorSettings settings);

	void Update() override;
};

#endif
