#ifndef GLM_HASH_HPP
#define GLM_HASH_HPP

#include <glm/glm.hpp>
#include <functional>

namespace std {

template <>
struct hash<glm::ivec2> {
	size_t operator()(const glm::ivec2 &v) const {
		// Hash each component (x, y) of the ivec2
		size_t h1 = hash<int>()(v.x);
		size_t h2 = hash<int>()(v.y);

		size_t combined = h1;
		combined ^= h2 + 0x9e3779b9 + (combined << 6) + (combined >> 2); // Mixing h2 into h1

		return combined;
	};
};

} // namespace std

#endif
