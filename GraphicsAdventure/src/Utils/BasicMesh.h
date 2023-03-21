#pragma once
#include <vector>
#include <array>

namespace GA::Utils
{
	std::vector<float> CreateCubeVertices(bool hasTexCoords, bool hasNormals);
	std::vector<float> CreatePlaneVertices(bool hasTexCoords, bool hasNormals);

	std::array<uint32_t, 36> CreateCubeIndices();
	std::array<uint32_t, 6> CreatePlaneIndices();
}