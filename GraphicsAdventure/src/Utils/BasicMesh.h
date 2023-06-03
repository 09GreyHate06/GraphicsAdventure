#pragma once
#include <vector>
#include <array>

namespace GA::Utils
{
	std::vector<float> CreateCubeVertices(bool hasTexCoords, bool hasNormals);
	std::array<float, 24> CreateCubeVerticesCompact(float min = -0.5f, float max = 0.5f);
	std::vector<float> CreatePlaneVertices(bool hasTexCoords, bool hasNormals);

	std::array<uint32_t, 36> CreateCubeIndices();
	std::array<uint32_t, 36> CreateCubeIndicesCompact();
	std::array<uint32_t, 6> CreatePlaneIndices();
}