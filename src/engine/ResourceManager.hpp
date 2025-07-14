#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "utils/Shader.hpp"
#include "utils/Texture.hpp"
#include "utils/Font.h"
#include <map>
#include <string>

class ResourceManager {
  public:
	// Instantiate static variables
	static std::map<std::string, Texture2D> Textures;
	static std::map<std::string, Shader> Shaders;
	static std::map<std::string, Font> Fonts;

	static void Init();

	static Shader LoadShader(const char *vShaderFile,
							 const char *fShaderFile, std::string name);

	static Shader GetShader(std::string name);

	static Texture2D LoadTexture(std::string name, bool alpha, const char *file);

	static Texture2D GetTexture(std::string name);

	static void LoadFont(std::string name, std::string path);

	static Font GetFont(std::string name);

	static void Clear();
};

#endif
