#ifndef MAP_GENERATOR_H
#define MAP_GENERATOR_H

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/string_cast.hpp>
#include <unordered_map>
#include "../../engine/utils/glm_hash.hpp"
#include "../utils/PerlinNoise.hpp"
#include <queue>
#include <vector>
#include "../utils/GeneratorSettings.hpp"
#include "../entities/TileEntity.hpp"

#define CHUNK_SIZE 32
#define maxPrimeIndex 10

struct OrePatch {
	glm::ivec2 center;
	std::string type;
	int radius;
	float richness;
};

// Modified MapGenerator Class with flexible noise settings
class MapGenerator {
  public:
	// Modified Generate function with noise parameters
	static std::vector<TileEntity *> Generate(int chunkX = 0, int chunkY = 0, GeneratorSettings settings = {}) {
		std::vector<TileEntity *> tiles;
		std::unordered_map<glm::ivec2, std::string> tileMap;

		int startX = chunkX * CHUNK_SIZE;
		int startY = chunkY * CHUNK_SIZE;

		int increment = 1;

		// --- Step 1: Terrain generation
		for (int y = startY; y < startY + CHUNK_SIZE; y += increment) {
			for (int x = startX; x < startX + CHUNK_SIZE; x += increment) {
				double h = GetHeight(x, y,
									 settings.terrainOctaves,
									 settings.terrainPersistence,
									 settings.terrainNoiseBias);
				std::string tileId;

				if (h < 0.25)
					tileId = "WATER_TILE";
				else if (h < 0.28)
					tileId = "SAND_TILE";
				else
					tileId = "GRASS_TILE_1";

				tileMap[glm::ivec2(x, y)] = tileId;
			}
		}

		std::vector<OrePatch> orePatches = GenerateOreSpots(chunkX, chunkY, settings);
		// Place ore after terrain tiles are filled
		for (const auto &patch : orePatches) {
			for (int dy = -patch.radius; dy <= patch.radius; ++dy) {
				for (int dx = -patch.radius; dx <= patch.radius; ++dx) {
					glm::ivec2 pos = patch.center + glm::ivec2(dx, dy);
					if (glm::length(glm::vec2(dx, dy)) > patch.radius)
						continue;

					// Only overwrite grass
					if (tileMap[pos] == "GRASS_TILE_1") {
						// Add noise falloff or randomness if desired
						tileMap[pos] = patch.type;
					}
				}
			}
		}

		// --- Step 3: Finalize TileEntities
		for (const auto &[pos, id] : tileMap) {
			TileEntity *tile = new TileEntity(glm::ivec3(pos.x, pos.y, 0), id);
			tile->GetComponent<TileTransform>()->size *= increment;
			tiles.push_back(tile);
		}

		return tiles;
	}

  private:
	// --- Heightmap and Ore Generation ---
	static double GetHeight(int x, int y, int numOctaves, double persistence, double noiseBias) {
		return glm::clamp((PerlinNoise::Noise(x + 100000, y + 100000, numOctaves, persistence) + noiseBias) * 1.7, 0.0, 1.0);
	}

	static std::vector<OrePatch> GenerateOreSpots(int chunkX, int chunkY, GeneratorSettings settings, int seed = 1337) {
		std::vector<OrePatch> patches;
		int regionSize = 64;
		int numSpots = 2 + rand() % 4; // Randomized per chunk

		for (int i = 0; i < numSpots; ++i) {
			glm::ivec2 center = {
				chunkX * CHUNK_SIZE + rand() % CHUNK_SIZE,
				chunkY * CHUNK_SIZE + rand() % CHUNK_SIZE};

			double height = GetHeight(center.x, center.y,
									  settings.terrainOctaves,
									  settings.terrainPersistence,
									  settings.terrainNoiseBias);
			if (height < 0.3)
				continue; // Avoid water/sand

			int r = 3 + rand() % 5; // Radius 3–7
			std::string type = "COAL_ORE_TILE";
			switch (rand() % 3) {
			case 0:
				type = "IRON_ORE_TILE";
				break;
			case 1:
				type = "COPPER_ORE_TILE";
				break;
			case 2:
				type = "COAL_ORE_TILE";
				break;
			}

			patches.push_back({center, type, r, 1.0f});
		}
		return patches;
	}
};

#endif // MAP_GENERATOR_H
