#include "../../engine/Scene.hpp"
#include "../../engine/ecs/components/Camera.hpp"
//#include "../components/Map.hpp"
#include "../components/ThreadedMapGenerator.hpp"
#include "../../engine/Userinterface.hpp"


struct MainScene : public Scene {
	MainScene() {
		ENTITY(map, new ThreadedMap);
		ENTITY(cameraEntity, new Transform, new Camera);
        UI* testUi = testUserinterface();
	}
};
