#ifndef GENERATOR_SETTINGS_H
#define GENERATOR_SETTINGS_H

#include <imgui.h>
#include <imgui_stdlib.h>

#define IMGUI_FIELD_FLOAT(label, var) ImGui::SliderFloat(label, &var, 0.0f, 1.0f)
#define IMGUI_FIELD_INT(label, var) ImGui::InputInt(label, &var)
#define IMGUI_FIELD_BOOL(label, var) ImGui::Checkbox(label, &var)
#define IMGUI_BUTTON(label, var) if (ImGui::Button(label)) { var = true; }

struct GeneratorSettings {
	int terrainOctaves = 6;
	double terrainPersistence = 0.4;
	double terrainNoiseBias = 0.2;

	// Optional: Add more control parameters
	double temperatureScale = 1.0;
	double moistureScale = 1.0;
	int seed = 12345;

	int chunkSize = 32;
	bool regenerateMap = false;

	void DrawImGui() {
		ImGui::Text("Terrain Settings");
		IMGUI_FIELD_INT("Terrain Octaves", terrainOctaves);
		float persistence = static_cast<float>(terrainPersistence);
		if (IMGUI_FIELD_FLOAT("Terrain Persistence", persistence)) {
			terrainPersistence = persistence;
		}
		float bias = static_cast<float>(terrainNoiseBias);
		if (IMGUI_FIELD_FLOAT("Terrain Bias", bias)) {
			terrainNoiseBias = bias;
		}
		float temp = static_cast<float>(temperatureScale);
		if (IMGUI_FIELD_FLOAT("Temperature Scale", temp)) {
			temperatureScale = temp;
		}
		float moist = static_cast<float>(moistureScale);
		if (IMGUI_FIELD_FLOAT("Moisture Scale", moist)) {
			moistureScale = moist;
		}

		IMGUI_BUTTON("RegenerateMap", regenerateMap);
	}
};

// Preset configurations for different world types
namespace WorldPresets {

// Balanced world - good for most gameplay
inline GeneratorSettings Balanced() {
	return {
		.terrainOctaves = 6,
		.terrainPersistence = 0.4,
		.terrainNoiseBias = 0.2,
		.temperatureScale = 1.0,
		.moistureScale = 1.0,
		.seed = 12345};
}

} // namespace WorldPresets

#endif

/*
PARAMETER EXPLANATIONS:

terrainOctaves (1-8):
- Controls terrain detail/complexity
- Lower = smoother, larger features
- Higher = more detailed, rougher terrain
- Recommended: 3-5 for most games

terrainPersistence (0.1-0.9):
- Controls how much each octave contributes
- Lower = smoother, less detail
- Higher = more chaotic, detailed
- Recommended: 0.3-0.6 for natural look

terrainNoiseBias (-0.5 to 0.5):
- Shifts overall elevation up/down
- Negative = more water/lower terrain
- Positive = more land/higher terrain
- Recommended: -0.2 to 0.2 for balance

temperatureScale (0.5-2.0):
- Multiplier for temperature variation
- Lower = more temperate climates
- Higher = more extreme hot/cold zones
- Recommended: 0.8-1.2 for variety

moistureScale (0.3-2.0):
- Multiplier for moisture variation
- Lower = more arid climates
- Higher = more wet/swampy areas
- Recommended: 0.8-1.3 for balance

USAGE EXAMPLES:

// Use a preset
auto settings = WorldPresets::Balanced();
auto tiles = MapGenerator::Generate(chunkX, chunkY, settings);

// Custom settings
GeneratorSettings custom = {
	.terrainOctaves = 4,
	.terrainPersistence = 0.5,
	.terrainNoiseBias = 0.1,
	.temperatureScale = 1.0,
	.moistureScale = 1.0,
	.seed = 54321
};
auto tiles = MapGenerator::Generate(chunkX, chunkY, custom);

// Quick balanced generation
auto tiles = MapGenerator::Generate(chunkX, chunkY, WorldPresets::Balanced());
*/
