#ifndef MAP_H
#define MAP_H

#include "../../engine/ecs/IComponent.hpp"
#include "../../engine/ecs/Entity.hpp"
#include "Chunk.hpp"
#include "../entities/ChunkEntity.hpp"
#include "../utils/GeneratorSettings.hpp"
#include "../../engine/Simplex.hpp"
#include "../../engine/ecs/components/Camera.hpp"
#include <glm/ext/vector_float2.hpp>
#include <unordered_map>
#include <utility>
#include "../../engine/utils/glm_hash.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>

struct Map : IComponent {
	std::unordered_map<glm::ivec2, Entity *> chunks;
	GeneratorSettings settings;

	Map() {};

	void Start() override {
	}

	void GenerateLODS() {
		RectBounds<int> chunkCoords = CalculateChunksInView();
		return;

		for (int y = chunkCoords.bottom - 10; y < chunkCoords.top + 10; y++) {
			for (int x = chunkCoords.left - 10; x < chunkCoords.right + 10; x++) {
				glm::ivec2 chunkCoord = glm::ivec2(x, y);
				Entity *chunk = chunks[chunkCoord];
				if (chunk != nullptr) {
					chunk->GetComponent<Chunk>()->Generate(settings);
				}
			}
		}
	};

	void GenerateChunks() {
		RectBounds<int> chunkCoords = CalculateChunksInView();

		int yIgnore = -1;
		int xIgnore = -1;
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

				Entity *chunk = chunks[chunkCoord];
				if (chunkCoords.InBounds(chunkCoord.x, chunkCoord.y) && !chunk->GetComponent<Chunk>()->Generated) {
					chunk->GetComponent<Chunk>()->Generate(settings);
				}
			}
		}
	}

	void CullChunks() {
		RectBounds<int> chunkCoords = CalculateChunksInView();
		for (auto it = chunks.begin(); it != chunks.end();) {
			glm::ivec2 chunkCoord = it->first;

			if ((chunkCoord.x < chunkCoords.left - 10 || chunkCoord.x >= chunkCoords.right + 10 ||
				 chunkCoord.y < chunkCoords.bottom - 10 || chunkCoord.y >= chunkCoords.top + 10) &&
				chunks.size() > 1000) {

				delete it->second;
				it = chunks.erase(it);
			} else {
				++it;
			}
		}
	}

	RectBounds<int> CalculateChunksInView() {
		RectBounds<float> cameraBounds = Simplex::view.Camera->GetComponent<Camera>()->GetCameraBounds();
		int chunkXStart = static_cast<int>(std::floor(cameraBounds.left / settings.chunkSize));
		int chunkXEnd = static_cast<int>(std::floor(cameraBounds.right / settings.chunkSize));
		int chunkYStart = static_cast<int>(std::floor(cameraBounds.top / settings.chunkSize));
		int chunkYEnd = static_cast<int>(std::floor(cameraBounds.bottom / settings.chunkSize));

		return {chunkYEnd, chunkXEnd, chunkYStart, chunkXStart};
	}

	void Update() override {
		ImGui::Begin("Map Generation Settings");
		settings.DrawImGui();

        ImGui::Text("Presets");
		if (ImGui::Button("Balanced")) {
			settings = WorldPresets::Balanced();
		}
        ImGui::End();

		if (settings.regenerateMap) {
			chunks.clear();
			settings.regenerateMap = false;
		}
		GenerateChunks();
		CullChunks();
		for (auto chunk : chunks) {
			chunk.second->UpdateComponents();
		}
	};
};

#endif
