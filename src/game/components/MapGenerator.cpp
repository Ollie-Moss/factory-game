#include "MapGenerator.hpp"

// Thread-safe static member definitions
std::unordered_map<std::string, BiomeInfo> MapGenerator::biomes;
std::mutex MapGenerator::biomes_mutex;
std::atomic<bool> MapGenerator::biomes_initialized{false};
