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
	float terrainPersistence = 0.35;
	float terrainNoiseBias = 0.23;

    int oreOctaves = 7;
	float orePersistence = 0.66;

	int chunkSize = 32;
	bool regenerateMap = false;

	void DrawImGui() {
        ImGui::Text("Terrain Settings");
		IMGUI_FIELD_INT("Terrain Octaves", terrainOctaves);
		IMGUI_FIELD_FLOAT("Terrain Persistence", terrainPersistence);
		IMGUI_FIELD_FLOAT("Terrain Bias", terrainNoiseBias);
        ImGui::Text("Ore Settings");
		IMGUI_FIELD_INT("Ore Octaves", oreOctaves);
		IMGUI_FIELD_FLOAT("Ore Persistence", orePersistence);

		IMGUI_BUTTON("RegenerateMap", regenerateMap);
	}
};

#endif
