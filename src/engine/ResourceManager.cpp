#include "ResourceManager.hpp"
#include "utils/Font.h"
#include "utils/Shader.hpp"
#include "utils/Texture.hpp"
#include <map>
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include <ft2build.h>
#include FT_FREETYPE_H

// Instantiate static variables
std::map<std::string, Texture2D> ResourceManager::Textures;
std::map<std::string, Shader> ResourceManager::Shaders;
std::map<std::string, Font> ResourceManager::Fonts;

Shader ResourceManager::LoadShader(const char *vShaderFile,
								   const char *fShaderFile, std::string name) {
	Shader shader;
	shader.Compile(vShaderFile, fShaderFile);
	Shaders[name] = shader;
	return Shaders[name];
}
void ResourceManager::Init() {
	stbi_set_flip_vertically_on_load(true);

	LoadShader("src/engine/utils/shaders/vSpriteShader.glsl",
			   "src/engine/utils/shaders/fSpriteShader.glsl",
			   "SpriteShader");

	LoadShader("src/engine/utils/shaders/vLineShader.glsl",
			   "src/engine/utils/shaders/fLineShader.glsl", "LineShader");

	LoadShader("src/engine/utils/shaders/vTextShader.glsl",
			   "src/engine/utils/shaders/fTextShader.glsl", "TextShader");

	LoadShader("src/engine/utils/shaders/vQuadShader.glsl",
			   "src/engine/utils/shaders/fQuadShader.glsl", "QuadShader");

	// Buildings
	LoadTexture("CONVEYOR_STRAIGHT", 1.0f, "sprites/conveyors/conveyor_straight_up.png");
	LoadTexture("FURNACE_SMALL", 1.0f, "sprites/buildings/furnace.png");
	LoadTexture("FURNACE", 1.0f, "sprites/buildings/furnace_large.png");
	LoadTexture("INSERTER", 1.0f, "sprites/buildings/inserter.png");

	// UI
	LoadTexture("INVENTORY_BACKGROUND", 1.0f, "sprites/UI/inventory_background.png");
	LoadTexture("INVENTORY_SLOT", 1.0f, "sprites/UI/inventory_slot.png");
	LoadTexture("TILE_SELCTOR", 1.0f, "sprites/UI/tile_selector.png");

	// Items
	LoadTexture("IRON_ORE_ITEM", 1.0f, "sprites/items/iron_ore.png");
	LoadTexture("COPPER_ORE_ITEM", 1.0f, "sprites/items/copper_ore.png");
	LoadTexture("COAL_ITEM", 1.0f, "sprites/items/coal.png");
	LoadTexture("LEAVES_ITEM", 1.0f, "sprites/items/leaves.png");
	LoadTexture("IRON_PLATE_ITEM", 1.0f, "sprites/items/iron_plate.png");
	LoadTexture("COPPER_PLATE_ITEM", 1.0f, "sprites/items/copper_plate.png");
	LoadTexture("GEARS_ITEM", 1.0f, "sprites/items/gears.png");
	LoadTexture("WIRE_ITEM", 1.0f, "sprites/items/wire.png");
	LoadTexture("CIRCUIT_BOARDS_ITEM", 1.0f, "sprites/items/circuit_board.png");

	// Environment
	LoadTexture("TREE", 1.0f, "sprites/environment/tree.png");
	LoadTexture("TILE_SELCTOR", 1.0f, "sprites/environment/grass.png");

	// Tiles
	LoadTexture("GRASS_TILE_1", 1.0f, "sprites/tiles/grass_tile_1.png");
	LoadTexture("GRASS_TILE_2", 1.0f, "sprites/tiles/grass_tile_2.png");

    // Fonts
    LoadFont("Arial", "fonts/arial.ttf");
};

Shader ResourceManager::GetShader(std::string name) {
	return Shaders[name];
}

Texture2D ResourceManager::LoadTexture(std::string name, bool alpha,
									   const char *file) {
	// create texture object
	Texture2D texture = Texture2D();
	if (alpha) {
		texture.Internal_Format = GL_RGBA;
		texture.Image_Format = GL_RGBA;
	}

	// load image
	int width, height, nrChannels;
	unsigned char *data = stbi_load(file, &width, &height, &nrChannels, STBI_rgb_alpha);
	// now generate texture
	texture.Generate(width, height, data);
	// and finally free image data
	stbi_image_free(data);
	Textures[name] = texture;
	return Textures[name];
}

Texture2D ResourceManager::GetTexture(std::string name) {
	return Textures[name];
}

void ResourceManager::LoadFont(std::string name, std::string path) {
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
				  << std::endl;
		return;
	}

	FT_Face face;
	if (FT_New_Face(ft, path.c_str(), 0, &face)) {
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return;
	}
	FT_Set_Pixel_Sizes(face, 0, 40);
	if (FT_Load_Char(face, 'X', FT_LOAD_RENDER)) {
		std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
		return;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

	for (unsigned char c = 0; c < 128; c++) {
		// load character glyph
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
					 face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
					 face->glyph->bitmap.buffer);
		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			(unsigned int)face->glyph->advance.x};
		Fonts[name].characters.insert(std::pair<char, Character>(c, character));
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

Font ResourceManager::GetFont(std::string name) {
	return Fonts[name];
};

void ResourceManager::Clear() {
	// (properly) delete all shaders
	for (auto iter : Shaders)
		glDeleteProgram(iter.second.ID);
	// (properly) delete all textures
	for (auto iter : Textures)
		glDeleteTextures(1, &iter.second.ID);
}
