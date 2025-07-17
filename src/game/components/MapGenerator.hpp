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
#include <random>
#include <mutex>
#include <atomic>

#define CHUNK_SIZE 32
#define maxPrimeIndex 10

struct OrePatch {
	glm::ivec2 center;
	std::string type;
	int radius;
	float richness;
};

struct BiomeInfo {
	std::string baseTile;
	std::string altTile;
	float altTileChance;
	std::vector<std::string> oreTypes;
	std::vector<float> oreWeights;
	float oreDensity;
};

// Thread-safe MapGenerator with biome system and improved noise
class MapGenerator {

  public:
	static std::vector<TileEntity *> Generate(int chunkX = 0, int chunkY = 0, GeneratorSettings settings = {}) {
		std::vector<TileEntity *> tiles;
		std::unordered_map<glm::ivec2, std::string> tileMap;

		int startX = chunkX * CHUNK_SIZE;
		int startY = chunkY * CHUNK_SIZE;

		// Initialize biome definitions (thread-safe)
		InitializeBiomes();

		// --- Step 1: Generate base terrain with biomes
		for (int y = startY; y < startY + CHUNK_SIZE; y++) {
			for (int x = startX; x < startX + CHUNK_SIZE; x++) {
				std::string tileId = GenerateTerrainTile(x, y, settings);
				tileMap[glm::ivec2(x, y)] = tileId;
			}
		}

		// --- Step 2: Generate ore deposits
		std::vector<OrePatch> orePatches = GenerateOreSpots(chunkX, chunkY, settings);

		// Place ore patches
		for (const auto &patch : orePatches) {
			PlaceOrePatch(patch, tileMap);
		}

		// --- Step 3: Post-process for variety and smoothing
		PostProcessTerrain(tileMap, startX, startY, settings);

		// --- Step 4: Create TileEntities
		for (const auto &[pos, id] : tileMap) {
			TileEntity *tile = new TileEntity(glm::ivec3(pos.x, pos.y, 0), id);
			tiles.push_back(tile);
		}

		return tiles;
	}

  private:
	// Thread-safe static members
	static std::unordered_map<std::string, BiomeInfo> biomes;
	static std::mutex biomes_mutex;
	static std::atomic<bool> biomes_initialized;

	static void InitializeBiomes() {
		// Double-checked locking pattern for thread-safe initialization
		if (biomes_initialized.load()) {
			return;
		}

		std::lock_guard<std::mutex> lock(biomes_mutex);
		
		// Check again after acquiring lock
		if (biomes_initialized.load()) {
			return;
		}

		biomes["GRASSLAND"] = {
			"GRASS_TILE_1", "GRASS_TILE_2", 0.3f, {"IRON_ORE_TILE", "COAL_ORE_TILE"}, {0.6f, 0.4f}, 0.05f};

		biomes["FOREST"] = {
			"GRASS_TILE_1", "TREE_TILE", 0.4f, {"IRON_ORE_TILE", "COPPER_ORE_TILE"}, {0.5f, 0.5f}, 0.05f};

		biomes["DESERT"] = {
			"SAND_TILE", "SAND_DUNE_TILE", 0.2f, {"COPPER_ORE_TILE", "GOLD_ORE_TILE"}, {0.7f, 0.3f}, 0.15f};

		biomes["MOUNTAIN"] = {
			"STONE_TILE",
			"MOUNTAIN_TILE",
			0.3f,
			{"IRON_ORE_TILE", "COAL_ORE_TILE", "GOLD_ORE_TILE"},
			{0.4f, 0.4f, 0.2f},
			0.25f};

		biomes["SWAMP"] = {
			"MUD_TILE", "SWAMP_WATER_TILE", 0.4f, {"COAL_ORE_TILE"}, {1.0f}, 0.08f};

		biomes_initialized.store(true);
	}

	static std::string GenerateTerrainTile(int x, int y, GeneratorSettings settings) {
		// Multi-layered noise for more complex terrain
		double elevation = GetHeight(x, y, settings.terrainOctaves, settings.terrainPersistence, settings.terrainNoiseBias);
		double temperature = GetTemperature(x, y, settings);
		double moisture = GetMoisture(x, y, settings);

		// Determine biome based on elevation, temperature, and moisture
		std::string biome = DetermineBiome(elevation, temperature, moisture);

		// Water level check
		if (elevation < 0.21) {
			return "WATER_TILE";
		}

		// Beach/shore transition
		if (elevation < 0.24) {
			return "SAND_TILE";
		}

		// Get biome-specific tile
		return GetBiomeTile(biome, x, y);
	}

	static double GetHeight(int x, int y, int numOctaves, double persistence, double noiseBias) {
		return glm::clamp((PerlinNoise::Noise(x + 100000, y + 100000, numOctaves, persistence) + noiseBias) * 1.7, 0.0, 1.0);
	}

	static double GetTemperature(int x, int y, GeneratorSettings settings) {
		// Temperature decreases with and elevation
		double elevationEffect = GetHeight(x, y, settings.terrainOctaves, settings.terrainPersistence, settings.terrainNoiseBias);
		double temperatureNoise = PerlinNoise::Noise(x * 0.01 + 50000, y * 0.01 + 50000, 3, 0.5);

		return glm::clamp(-elevationEffect * 0.5 + temperatureNoise * 0.3, 0.0, 1.0) * settings.temperatureScale;
	}

	static double GetMoisture(int x, int y, GeneratorSettings settings) {
		// Moisture based on distance from water and noise
		double moistureNoise = PerlinNoise::Noise(x * 0.005 + 75000, y * 0.005 + 75000, 4, 0.6);
		return glm::clamp(moistureNoise + 0.5, 0.0, 1.0) * settings.moistureScale;
	}

	static std::string DetermineBiome(double elevation, double temperature, double moisture) {
		// High elevation = mountains
		if (elevation > 0.7) {
			return "MOUNTAIN";
		}

		// Low elevation, high moisture = swamp
		if (elevation < 0.4 && moisture > 0.7) {
			return "SWAMP";
		}

		// Hot and dry = desert
		if (temperature > 0.7 && moisture < 0.3) {
			return "DESERT";
		}

		// Moderate temperature, high moisture = forest
		if (moisture > 0.5 && temperature > 0.3 && temperature < 0.8) {
			return "FOREST";
		}

		// Default to grassland
		return "GRASSLAND";
	}

	static std::string GetBiomeTile(const std::string &biome, int x, int y) {
		// Thread-safe biome lookup with read lock
		std::lock_guard<std::mutex> lock(biomes_mutex);
		
		auto it = biomes.find(biome);
		if (it == biomes.end()) {
			return "GRASS_TILE_1"; // Fallback
		}

		const BiomeInfo &info = it->second;

		// Use position-based deterministic randomness for tile variation
		// Create a new RNG instance for each call to avoid shared state
		std::mt19937 rng(x * 73856093 + y * 19349663);
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		if (dist(rng) < info.altTileChance) {
			return info.altTile;
		}

		return info.baseTile;
	}

	static std::vector<OrePatch> GenerateOreSpots(int chunkX, int chunkY, GeneratorSettings settings) {
		std::vector<OrePatch> patches;

		// Generate ore patches on a larger grid to avoid overlap and ensure proper spacing
		int regionSize = 64; // Ore patches spawn in 64x64 regions

		// Check surrounding regions for ore patches that might affect this chunk
		for (int regionY = (chunkY * CHUNK_SIZE - regionSize) / regionSize;
			 regionY <= (chunkY * CHUNK_SIZE + CHUNK_SIZE + regionSize) / regionSize; regionY++) {
			for (int regionX = (chunkX * CHUNK_SIZE - regionSize) / regionSize;
				 regionX <= (chunkX * CHUNK_SIZE + CHUNK_SIZE + regionSize) / regionSize; regionX++) {

				// Use region coordinates for deterministic seeding
				// Create a new RNG instance for each region to avoid shared state
				std::mt19937 rng((regionX * 73856093) ^ (regionY * 19349663));
				std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);

				// 30% chance for an ore patch to spawn in this region
				if (spawnChance(rng) > 0.3f)
					continue;

				// Generate patch center within the region
				std::uniform_int_distribution<int> regionPosDist(0, regionSize - 1);
				glm::ivec2 patchCenter = {
					regionX * regionSize + regionPosDist(rng),
					regionY * regionSize + regionPosDist(rng)};

				// Check if this patch is relevant to current chunk
				int maxRadius = 15; // Maximum possible patch radius
				if (patchCenter.x + maxRadius < chunkX * CHUNK_SIZE ||
					patchCenter.x - maxRadius > (chunkX + 1) * CHUNK_SIZE ||
					patchCenter.y + maxRadius < chunkY * CHUNK_SIZE ||
					patchCenter.y - maxRadius > (chunkY + 1) * CHUNK_SIZE) {
					continue;
				}

				double elevation = GetHeight(patchCenter.x, patchCenter.y, settings.terrainOctaves, settings.terrainPersistence, settings.terrainNoiseBias);
				double temperature = GetTemperature(patchCenter.x, patchCenter.y, settings);
				double moisture = GetMoisture(patchCenter.x, patchCenter.y, settings);

				if (elevation < 0.25)
					continue; // Skip water/shore areas

				std::string biome = DetermineBiome(elevation, temperature, moisture);
				
				// Thread-safe biome lookup
				BiomeInfo biomeInfo;
				{
					std::lock_guard<std::mutex> lock(biomes_mutex);
					auto biomeIt = biomes.find(biome);
					if (biomeIt == biomes.end())
						continue;
					biomeInfo = biomeIt->second; // Copy the BiomeInfo
				}

				if (biomeInfo.oreTypes.empty())
					continue;

				// Select ore type based on biome weights
				std::discrete_distribution<int> oreTypeDist(biomeInfo.oreWeights.begin(), biomeInfo.oreWeights.end());
				int oreTypeIndex = oreTypeDist(rng);

				// Generate patch size based on ore type
				std::uniform_int_distribution<int> radiusDist(6, 12);
				int radius = radiusDist(rng);

				patches.push_back({patchCenter,
								   biomeInfo.oreTypes[oreTypeIndex],
								   radius,
								   1.0f});
			}
		}

		return patches;
	}

	static void PlaceOrePatch(const OrePatch &patch, std::unordered_map<glm::ivec2, std::string> &tileMap) {
		// Use noise-based generation for more natural, solid ore patches
		// Create a new RNG instance for each patch to avoid shared state
		std::mt19937 rng(patch.center.x * 73856093 + patch.center.y * 19349663);

		// Generate shape distortion parameters
		float distortionScale = 0.15f; // How much to distort the shape
		float distortionFreq = 0.08f;  // Frequency of distortion noise

		// Create directional bias for more interesting shapes
		std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
		std::uniform_real_distribution<float> elongationDist(0.7f, 1.4f);
		float elongationAngle = angleDist(rng);
		float elongationFactor = elongationDist(rng);

		// Create a solid core with noise-based edges and shape distortion
		for (int dy = -patch.radius; dy <= patch.radius; ++dy) {
			for (int dx = -patch.radius; dx <= patch.radius; ++dx) {
				glm::ivec2 pos = patch.center + glm::ivec2(dx, dy);

				// Apply shape distortion using multiple noise layers
				float distortionX = PerlinNoise::Noise((pos.x + patch.center.x) * distortionFreq,
													   (pos.y + patch.center.y) * distortionFreq + 1000, 4, 0.6f);
				float distortionY = PerlinNoise::Noise((pos.x + patch.center.x) * distortionFreq + 2000,
													   (pos.y + patch.center.y) * distortionFreq, 4, 0.6f);

				// Apply distortion
				glm::vec2 distortedPos = glm::vec2(float(dx), float(dy)) + glm::vec2(distortionX, distortionY) * distortionScale * float(patch.radius);

				// Apply directional elongation
				float rotatedX = distortedPos.x * cos(elongationAngle) - distortedPos.y * sin(elongationAngle);
				float rotatedY = distortedPos.x * sin(elongationAngle) + distortedPos.y * cos(elongationAngle);

				// Scale one axis for elongation
				rotatedX /= elongationFactor;

				// Calculate distorted distance
				float distortedDistance = sqrt(rotatedX * rotatedX + rotatedY * rotatedY);

				if (distortedDistance > float(patch.radius))
					continue;

				bool shouldPlace = false;

				// Inner core is always solid (like Factorio) - but now distorted
				if (distortedDistance <= float(patch.radius) * 0.5f) {
					shouldPlace = true;
				} else {
					// Outer edge uses additional noise for natural boundaries
					float edgeNoise = PerlinNoise::Noise(pos.x * 0.12f, pos.y * 0.12f, 3, 0.5f);
					float secondaryNoise = PerlinNoise::Noise(pos.x * 0.25f + 5000, pos.y * 0.25f + 5000, 2, 0.4f);

					float edgeThreshold = 1.0f - ((distortedDistance - float(patch.radius) * 0.5f) / (float(patch.radius) * 0.5f));

					// Add multiple layers of noise for more organic shapes
					edgeThreshold += edgeNoise * 0.35f + secondaryNoise * 0.15f;

					if (edgeThreshold > 0.6f) {
						shouldPlace = true;
					}
				}

				if (shouldPlace) {
					auto it = tileMap.find(pos);
					if (it != tileMap.end() && it->second != "WATER_TILE") {
						tileMap[pos] = patch.type;
					}
				}
			}
		}

		// Post-process to remove isolated single tiles and fill small gaps
		CleanupOrePatch(patch, tileMap);
	}

	static void CleanupOrePatch(const OrePatch &patch, std::unordered_map<glm::ivec2, std::string> &tileMap) {
		// Remove isolated ore tiles (tiles with fewer than 2 ore neighbors)
		std::vector<glm::ivec2> tilesToRemove;
		std::vector<glm::ivec2> tilesToAdd;

		for (int dy = -patch.radius; dy <= patch.radius; ++dy) {
			for (int dx = -patch.radius; dx <= patch.radius; ++dx) {
				glm::ivec2 pos = patch.center + glm::ivec2(dx, dy);
				float distance = glm::length(glm::vec2(dx, dy));

				if (distance > patch.radius)
					continue;

				auto it = tileMap.find(pos);
				if (it == tileMap.end())
					continue;

				// Count ore neighbors
				int oreNeighbors = 0;
				int nonOreNeighbors = 0;

				for (int ndy = -1; ndy <= 1; ++ndy) {
					for (int ndx = -1; ndx <= 1; ++ndx) {
						if (ndx == 0 && ndy == 0)
							continue;

						glm::ivec2 neighborPos = pos + glm::ivec2(ndx, ndy);
						auto neighborIt = tileMap.find(neighborPos);

						if (neighborIt != tileMap.end()) {
							if (neighborIt->second == patch.type) {
								oreNeighbors++;
							} else if (neighborIt->second != "WATER_TILE") {
								nonOreNeighbors++;
							}
						}
					}
				}

				// Remove isolated ore tiles
				if (it->second == patch.type && oreNeighbors < 2) {
					tilesToRemove.push_back(pos);
				}

				// Fill small gaps (non-ore tiles completely surrounded by ore)
				if (it->second != patch.type && it->second != "WATER_TILE" &&
					oreNeighbors >= 6 && distance <= patch.radius * 0.8f) {
					tilesToAdd.push_back(pos);
				}
			}
		}

		// Apply cleanup changes
		for (const auto &pos : tilesToRemove) {
			// Restore original terrain type - you might want to store this info
			auto it = tileMap.find(pos);
			if (it != tileMap.end()) {
				// Simple restoration - in practice you might want to regenerate the original tile
				it->second = "GRASS_TILE_1";
			}
		}

		for (const auto &pos : tilesToAdd) {
			tileMap[pos] = patch.type;
		}
	}

	static void PostProcessTerrain(std::unordered_map<glm::ivec2, std::string> &tileMap, int startX, int startY, GeneratorSettings settings) {
		// Add small details like scattered rocks, flowers, etc.
		// Create a new RNG instance for each post-process call to avoid shared state
		std::mt19937 rng(startX * 73856093 + startY * 19349663);
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		for (int y = startY; y < startY + CHUNK_SIZE; y++) {
			for (int x = startX; x < startX + CHUNK_SIZE; x++) {
				glm::ivec2 pos(x, y);
				auto it = tileMap.find(pos);
				if (it == tileMap.end())
					continue;

				// Add variety to grass tiles
				if (it->second == "GRASS_TILE_1" && dist(rng) < 0.05f) {
					it->second = "GRASS_TILE_1";
				}

				// Add rocks to mountain areas
				if (it->second == "STONE_TILE" && dist(rng) < 0.1f) {
					it->second = "ROCK_TILE";
				}
			}
		}
	}
};


#endif // MAP_GENERATOR_H
