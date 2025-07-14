#include "Chunk.hpp"
#include "ChunkRenderer.hpp"
#include <future>
#include "MapGenerator.hpp"

void Chunk::Generate(int scaleFactor) {
	std::future generatedTiles = std::async(MapGenerator::Generate, transform->position.x, transform->position.y, scaleFactor);
    tiles = generatedTiles.get();
	Generated = (scaleFactor == 1);
	renderer->AddChunkToSSBO(*this);
}

void Chunk::Update() {
	for (auto tile : tiles) {
		tile->UpdateComponents();
	}
}
