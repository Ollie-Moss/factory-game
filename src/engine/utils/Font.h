#ifndef CHARACTER_H
#define CHARACTER_H

#include <glm/ext/vector_int2.hpp>
#include <map>

struct Character {
	unsigned int TextureID; // ID handle of the glyph texture
	glm::ivec2 Size;		// Size of glyph
	glm::ivec2 Bearing;		// Offset from baseline to left/top of glyph
	unsigned int Advance;	// Offset to advance to next glyph
};

struct Font {
	std::map<char, Character> characters;
};

#endif
