#include "engine/Simplex.hpp"
#include "game/scenes/MainScene.hpp"

int main() {
	Simplex::CreateWindow("Industria", 1280-190, 720-160);

	Simplex::SetScene(MainScene());

	Simplex::Loop();

	return 0;
}
