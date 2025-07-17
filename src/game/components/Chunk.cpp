#include "Chunk.hpp"
#include "ChunkRenderer.hpp"
#include <future>
#include "Map.hpp"
#include "MapGenerator.hpp"

void Chunk::Generate(GeneratorSettings settings) {
	std::future generatedTiles = std::async(MapGenerator::Generate, transform->position.x, transform->position.y, settings);
    tiles = generatedTiles.get();
	Generated = true;
	//renderer->AddChunkToSSBO(*this);
}

void Chunk::Update() {
	for (auto tile : tiles) {
		tile->UpdateComponents();
	}
}
