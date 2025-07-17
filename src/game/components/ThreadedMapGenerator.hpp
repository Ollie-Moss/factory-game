#ifndef THREADED_MAP_GENERATOR_H
#define THREADED_MAP_GENERATOR_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <future>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <unordered_set>
#include "Chunk.hpp"
#include "MapGenerator.hpp"
#include "../utils/GeneratorSettings.hpp"
#include "../entities/ChunkEntity.hpp"
#include "../../engine/ecs/components/Camera.hpp"

struct ChunkGenerationRequest {
	glm::ivec2 chunkCoord;
	GeneratorSettings settings;
	int priority; // Higher = more important
	std::chrono::steady_clock::time_point requestTime;

	bool operator<(const ChunkGenerationRequest &other) const {
		// Higher priority first, then by request time
		if (priority != other.priority) {
			return priority < other.priority;
		}
		return requestTime > other.requestTime;
	}
};

struct ChunkGenerationResult {
	glm::ivec2 chunkCoord;
	std::vector<TileEntity *> tiles;
	bool success;
	std::string errorMessage;
};

class ThreadedMapGenerator {
private:
	// Thread pool
	std::vector<std::thread> workers;
	std::atomic<bool> shouldStop{false};
	std::atomic<bool> isInitialized{false};

	// Work queue - using shared_ptr to avoid copy issues
	std::priority_queue<ChunkGenerationRequest> workQueue;
	mutable std::mutex queueMutex;
	std::condition_variable queueCondition;

	// Results
	std::queue<ChunkGenerationResult> completedChunks;
	mutable std::mutex resultsMutex;

	// Currently generating chunks (to avoid duplicates)
	std::unordered_set<glm::ivec2> generatingChunks;
	mutable std::mutex generatingMutex;

	// Thread count
	size_t threadCount;

	// Statistics
	std::atomic<size_t> chunksGenerated{0};
	std::atomic<size_t> chunksQueued{0};

	// Thread-safe initialization
	static std::once_flag initFlag;
	static void InitializeMapGenerator() {
		// This ensures thread-safe initialization of static members
		// Call this once before any threaded operations
	}

public:
	ThreadedMapGenerator(size_t numThreads = std::thread::hardware_concurrency())
		: threadCount(numThreads) {
		
		// Ensure thread-safe initialization
		std::call_once(initFlag, InitializeMapGenerator);
		
		// Create worker threads
		for (size_t i = 0; i < threadCount; ++i) {
			workers.emplace_back([this]() { WorkerThread(); });
		}
		
		isInitialized = true;
	}

	~ThreadedMapGenerator() {
		Stop();
	}

	void Stop() {
		if (!isInitialized) return;
		
		shouldStop = true;
		queueCondition.notify_all();

		for (auto &worker : workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}
		
		// Clean up any remaining results
		{
			std::lock_guard<std::mutex> lock(resultsMutex);
			while (!completedChunks.empty()) {
				auto& result = completedChunks.front();
				// Clean up tiles
				for (auto* tile : result.tiles) {
					delete tile;
				}
				completedChunks.pop();
			}
		}
	}

	// Request chunk generation with priority
	void RequestChunk(const glm::ivec2 &chunkCoord, const GeneratorSettings &settings, int priority = 0) {
		if (shouldStop || !isInitialized) return;
		
		{
			std::lock_guard<std::mutex> lock(generatingMutex);
			if (generatingChunks.find(chunkCoord) != generatingChunks.end()) {
				return; // Already generating
			}
			generatingChunks.insert(chunkCoord);
		}

		ChunkGenerationRequest request;
		request.chunkCoord = chunkCoord;
		request.settings = settings;
		request.priority = priority;
		request.requestTime = std::chrono::steady_clock::now();

		{
			std::lock_guard<std::mutex> lock(queueMutex);
			workQueue.push(request);
			chunksQueued++;
		}

		queueCondition.notify_one();
	}

	// Get completed chunks (call from main thread)
	std::vector<ChunkGenerationResult> GetCompletedChunks() {
		std::vector<ChunkGenerationResult> results;
		
		if (!isInitialized) return results;

		std::lock_guard<std::mutex> lock(resultsMutex);
		while (!completedChunks.empty()) {
			results.push_back(std::move(completedChunks.front()));
			completedChunks.pop();
		}

		return results;
	}

	// Cancel chunk generation request
	void CancelChunk(const glm::ivec2 &chunkCoord) {
		std::lock_guard<std::mutex> lock(generatingMutex);
		generatingChunks.erase(chunkCoord);
	}

	// Clear all pending requests
	void ClearQueue() {
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			std::priority_queue<ChunkGenerationRequest> empty;
			workQueue.swap(empty);
			chunksQueued = 0;
		}

		{
			std::lock_guard<std::mutex> genLock(generatingMutex);
			generatingChunks.clear();
		}
	}

	// Statistics
	size_t GetQueueSize() const {
		if (!isInitialized) return 0;
		std::lock_guard<std::mutex> lock(queueMutex);
		return workQueue.size();
	}

	size_t GetChunksGenerated() const { return chunksGenerated; }
	size_t GetChunksQueued() const { return chunksQueued; }

	bool IsGenerating(const glm::ivec2 &chunkCoord) const {
		if (!isInitialized) return false;
		std::lock_guard<std::mutex> lock(generatingMutex);
		return generatingChunks.find(chunkCoord) != generatingChunks.end();
	}

private:
	void WorkerThread() {
		while (!shouldStop) {
			ChunkGenerationRequest request;
			bool hasWork = false;

			// Get work from queue
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				queueCondition.wait(lock, [this] { return !workQueue.empty() || shouldStop; });

				if (shouldStop) break;

				if (!workQueue.empty()) {
					request = workQueue.top();
					workQueue.pop();
					hasWork = true;
				}
			}

			if (!hasWork) continue;

			// Check if chunk is still needed
			bool shouldGenerate = false;
			{
				std::lock_guard<std::mutex> lock(generatingMutex);
				shouldGenerate = generatingChunks.find(request.chunkCoord) != generatingChunks.end();
			}

			if (!shouldGenerate) continue;

			// Generate the chunk
			ChunkGenerationResult result;
			result.chunkCoord = request.chunkCoord;
			result.success = true;

			try {
				// CRITICAL: Make sure MapGenerator is thread-safe
				result.tiles = MapGenerator::Generate(
					request.chunkCoord.x,
					request.chunkCoord.y,
					request.settings);
				chunksGenerated++;
			} catch (const std::exception &e) {
				result.success = false;
				result.errorMessage = e.what();

				// Clean up any partial tiles
				for (auto *tile : result.tiles) {
					delete tile;
				}
				result.tiles.clear();
			} catch (...) {
				result.success = false;
				result.errorMessage = "Unknown error during chunk generation";
				
				// Clean up any partial tiles
				for (auto *tile : result.tiles) {
					delete tile;
				}
				result.tiles.clear();
			}

			// Remove from generating set
			{
				std::lock_guard<std::mutex> lock(generatingMutex);
				generatingChunks.erase(request.chunkCoord);
			}

			// Add to results
			{
				std::lock_guard<std::mutex> lock(resultsMutex);
				completedChunks.push(std::move(result));
			}
		}
	}
};

// Static member definition
std::once_flag ThreadedMapGenerator::initFlag;

// Updated Map component to use threaded generation
struct ThreadedMap : IComponent {
	std::unordered_map<glm::ivec2, Entity *> chunks;
	std::unique_ptr<ThreadedMapGenerator> generator;
	GeneratorSettings settings;

	// Chunks that are being generated
	std::unordered_set<glm::ivec2> pendingChunks;
	
	// Safety flags
	std::atomic<bool> isUpdating{false};
	std::atomic<bool> isDestroying{false};

	ThreadedMap() : generator(std::make_unique<ThreadedMapGenerator>()) {}

	~ThreadedMap() {
		isDestroying = true;
		
		// Wait for any ongoing updates
		while (isUpdating) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		
		generator->Stop();
		
		// Clean up chunks
		for (auto &[coord, entity] : chunks) {
			if (entity) {
				delete entity;
			}
		}
		chunks.clear();
	}

	void Start() override {
		// Initialize generator if needed
	}

	void Update() override {
		if (isDestroying) return;
		
		isUpdating = true;
		
		// Handle ImGui settings - make sure this is called from main thread
		if (ImGui::GetCurrentContext() != nullptr) {
			ImGui::Begin("Threaded Map Generation Settings");
			settings.DrawImGui();

			ImGui::Text("Generation Stats:");
			ImGui::Text("Queue Size: %zu", generator->GetQueueSize());
			ImGui::Text("Generated: %zu", generator->GetChunksGenerated());
			ImGui::Text("Pending: %zu", pendingChunks.size());
			ImGui::Text("Active Chunks: %zu", chunks.size());

			if (ImGui::Button("Clear Queue")) {
				generator->ClearQueue();
				pendingChunks.clear();
			}

			ImGui::Text("Presets");
			if (ImGui::Button("Balanced")) {
				settings = WorldPresets::Balanced();
			}
			ImGui::End();
		}

		// Handle map regeneration
		if (settings.regenerateMap) {
			generator->ClearQueue();

			// Clean up existing chunks
			for (auto &[coord, entity] : chunks) {
				if (entity) {
					delete entity;
				}
			}
			chunks.clear();
			pendingChunks.clear();

			settings.regenerateMap = false;
		}

		// Process completed chunks
		ProcessCompletedChunks();

		// Generate new chunks
		GenerateChunks();

		// Cull distant chunks
		CullChunks();

		// Update existing chunks
		for (auto &[coord, chunk] : chunks) {
			if (chunk && !isDestroying) {
				chunk->UpdateComponents();
			}
		}
		
		isUpdating = false;
	}

private:
	void ProcessCompletedChunks() {
		auto completedChunks = generator->GetCompletedChunks();

		for (auto &result : completedChunks) {
			if (isDestroying) {
				// Clean up tiles if we're destroying
				for (auto* tile : result.tiles) {
					delete tile;
				}
				continue;
			}
			
			pendingChunks.erase(result.chunkCoord);

			if (result.success) {
				// Create chunk entity
				Entity *chunkEntity = new ChunkEntity(result.chunkCoord);
				Chunk *chunkComponent = chunkEntity->GetComponent<Chunk>();

				if (chunkComponent) {
					// Set tiles directly instead of generating
					chunkComponent->tiles = std::move(result.tiles);
					chunkComponent->Generated = true;

					chunks[result.chunkCoord] = chunkEntity;
                    chunkEntity->GetComponent<ChunkRenderer>()->AddChunkToSSBO(*chunkComponent);
				} else {
					// Clean up if chunk creation failed
					delete chunkEntity;
					for (auto* tile : result.tiles) {
						delete tile;
					}
				}
			} else {
				// Handle generation error
				printf("Chunk generation failed for (%d, %d): %s\n",
					   result.chunkCoord.x, result.chunkCoord.y,
					   result.errorMessage.c_str());
				
				// Clean up tiles
				for (auto* tile : result.tiles) {
					delete tile;
				}
			}
		}
	}

	void GenerateChunks() {
		if (isDestroying) return;
		
		RectBounds<int> chunkCoords = CalculateChunksInView();

		// Expand bounds slightly for buffering
		int buffer = 2;
		chunkCoords.top += buffer;
		chunkCoords.bottom -= buffer;
		chunkCoords.left -= buffer;
		chunkCoords.right += buffer;

		// Calculate priorities based on distance from camera
		glm::ivec2 cameraChunk = GetCameraChunkCoord();

		std::vector<std::pair<glm::ivec2, int>> chunksToGenerate;

		for (int y = chunkCoords.bottom; y < chunkCoords.top; y++) {
			for (int x = chunkCoords.left; x < chunkCoords.right; x++) {
				glm::ivec2 chunkCoord(x, y);

				// Skip if already exists or is being generated
				if (chunks.find(chunkCoord) != chunks.end() ||
					pendingChunks.find(chunkCoord) != pendingChunks.end()) {
					continue;
				}

				// Calculate priority based on distance from camera
				int distance = abs(chunkCoord.x - cameraChunk.x) + abs(chunkCoord.y - cameraChunk.y);
				int priority = 100 - distance; // Higher priority for closer chunks

				chunksToGenerate.push_back({chunkCoord, priority});
			}
		}

		// Sort by priority and request generation
		std::sort(chunksToGenerate.begin(), chunksToGenerate.end(),
				  [](const auto &a, const auto &b) { return a.second > b.second; });

		for (const auto &[coord, priority] : chunksToGenerate) {
			generator->RequestChunk(coord, settings, priority);
			pendingChunks.insert(coord);
		}
	}

	void CullChunks() {
		if (isDestroying) return;
		
		RectBounds<int> chunkCoords = CalculateChunksInView();
		int cullBuffer = 5; // Larger buffer for culling

		for (auto it = chunks.begin(); it != chunks.end();) {
			glm::ivec2 chunkCoord = it->first;

			bool shouldCull = (chunkCoord.x < chunkCoords.left - cullBuffer ||
							   chunkCoord.x >= chunkCoords.right + cullBuffer ||
							   chunkCoord.y < chunkCoords.bottom - cullBuffer ||
							   chunkCoord.y >= chunkCoords.top + cullBuffer) &&
							  chunks.size() > 135;

			if (shouldCull) {
				if (it->second) {
					delete it->second;
				}
				it = chunks.erase(it);
			} else {
				++it;
			}
		}

		// Also cull pending chunks that are too far away
		for (auto it = pendingChunks.begin(); it != pendingChunks.end();) {
			glm::ivec2 chunkCoord = *it;

			bool shouldCull = chunkCoord.x < chunkCoords.left - cullBuffer ||
							  chunkCoord.x >= chunkCoords.right + cullBuffer ||
							  chunkCoord.y < chunkCoords.bottom - cullBuffer ||
							  chunkCoord.y >= chunkCoords.top + cullBuffer;

			if (shouldCull) {
				generator->CancelChunk(chunkCoord);
				it = pendingChunks.erase(it);
			} else {
				++it;
			}
		}
	}

	RectBounds<int> CalculateChunksInView() {
		// Add null checks for safety
		if (!Simplex::view.Camera) return {0, 0, 0, 0};
		
		Camera* camera = Simplex::view.Camera->GetComponent<Camera>();
		if (!camera) return {0, 0, 0, 0};
		
		RectBounds<float> cameraBounds = camera->GetCameraBounds();
		int chunkXStart = static_cast<int>(std::floor(cameraBounds.left / settings.chunkSize));
		int chunkXEnd = static_cast<int>(std::floor(cameraBounds.right / settings.chunkSize));
		int chunkYStart = static_cast<int>(std::floor(cameraBounds.top / settings.chunkSize));
		int chunkYEnd = static_cast<int>(std::floor(cameraBounds.bottom / settings.chunkSize));

		return {chunkYEnd, chunkXEnd, chunkYStart, chunkXStart};
	}

	glm::ivec2 GetCameraChunkCoord() {
		// Add null checks for safety
		if (!Simplex::view.Camera) return {0, 0};
		
		Camera* camera = Simplex::view.Camera->GetComponent<Camera>();
		if (!camera) return {0, 0};
		
		RectBounds<float> cameraBounds = camera->GetCameraBounds();
		float centerX = (cameraBounds.left + cameraBounds.right) * 0.5f;
		float centerY = (cameraBounds.top + cameraBounds.bottom) * 0.5f;

		return glm::ivec2(
			static_cast<int>(std::floor(centerX / settings.chunkSize)),
			static_cast<int>(std::floor(centerY / settings.chunkSize)));
	}
};

#endif // THREADED_MAP_GENERATOR_H
