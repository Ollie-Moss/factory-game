#ifndef MAP_H
#define MAP_H

#include "../engine/ecs/IComponent.hpp"
#include "../entities/ChunkEntity.hpp"
#include "Chunk.hpp"
#include "MapGenerator.hpp"
#include "../engine/Simplex.hpp"
#include "../engine/ecs/components/Camera.hpp"
#include <glm/ext/vector_float2.hpp>
#include <thread>
#include <unordered_map>
#include <utility>

namespace std {
template <>
struct hash<glm::ivec2> {
	size_t operator()(const glm::ivec2 &v) const {
		// Hash each component (x, y) of the ivec2
		size_t h1 = hash<int>()(v.x);
		size_t h2 = hash<int>()(v.y);

		size_t combined = h1;
		combined ^= h2 + 0x9e3779b9 + (combined << 6) + (combined >> 2); // Mixing h2 into h1

		return combined;
	};
};
} // namespace std

struct Map : IComponent {
	std::unordered_map<glm::ivec2, ChunkEntity *> chunks;

	Map() {};

	void Start() override {
	}

	void GenerateLODS() {
		RectBounds<int> chunkCoords = CalculateChunksInView();
		return;

		for (int y = chunkCoords.bottom - 10; y < chunkCoords.top + 10; y++) {
			for (int x = chunkCoords.left - 10; x < chunkCoords.right + 10; x++) {
				glm::ivec2 chunkCoord = glm::ivec2(x, y);
				ChunkEntity *chunk = chunks[chunkCoord];
				if (chunk != nullptr) {
					chunk->GetComponent<Chunk>()->Generate(4);
				}
			}
		}
	};

	void GenerateChunks() {
		RectBounds<int> chunkCoords = CalculateChunksInView();

		int yIgnore = 0;
		int xIgnore = 0;
		chunkCoords.top -= yIgnore;
		chunkCoords.bottom += yIgnore;
		chunkCoords.left += xIgnore;
		chunkCoords.right -= xIgnore;

		for (int y = chunkCoords.bottom; y < chunkCoords.top; y++) {
			for (int x = chunkCoords.left; x < chunkCoords.right; x++) {
				glm::ivec2 chunkCoord = glm::ivec2(x, y);

				if (chunks[chunkCoord] == nullptr) {
					chunks[chunkCoord] = new ChunkEntity(chunkCoord);
				}

				ChunkEntity *chunk = chunks[chunkCoord];
				if (chunkCoords.InBounds(chunkCoord.x, chunkCoord.y) && !chunk->GetComponent<Chunk>()->Generated) {
                    chunk->GetComponent<Chunk>()->Generate();
				}
			}
		}
	}

	void CullChunks() {
		RectBounds<int> chunkCoords = CalculateChunksInView();
		for (auto it = chunks.begin(); it != chunks.end();) {
			glm::ivec2 chunkCoord = it->first;

			if ((chunkCoord.x < chunkCoords.left - 10 || chunkCoord.x >= chunkCoords.right + 10 ||
				chunkCoord.y < chunkCoords.bottom - 10 || chunkCoord.y >= chunkCoords.top + 10) && chunks.size() > 1000) {

				delete it->second;
				it = chunks.erase(it);
			} else {
				++it;
			}
		}
	}

	RectBounds<int> CalculateChunksInView() {
		RectBounds<float> cameraBounds = Simplex::view.Camera->GetComponent<Camera>()->GetCameraBounds();
		int chunkXStart = static_cast<int>(std::floor(cameraBounds.left / CHUNK_SIZE));
		int chunkXEnd = static_cast<int>(std::floor(cameraBounds.right / CHUNK_SIZE));
		int chunkYStart = static_cast<int>(std::floor(cameraBounds.top / CHUNK_SIZE));
		int chunkYEnd = static_cast<int>(std::floor(cameraBounds.bottom / CHUNK_SIZE));

		return {chunkYEnd, chunkXEnd, chunkYStart, chunkXStart};
	}

	void Update() override {
		GenerateChunks();
		GenerateLODS();
		CullChunks();
		for (auto chunk : chunks) {
			chunk.second->UpdateComponents();
		}
	};
};

#endif
