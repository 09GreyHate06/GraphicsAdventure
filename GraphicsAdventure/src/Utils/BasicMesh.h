#pragma once
#include <vector>
#include <array>
#include <DirectXMath.h>

namespace GA::Utils
{
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 uv; 
		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT3 bitangent;
		DirectX::XMFLOAT3 normal;
	};


	std::vector<float> CreateCubeVertices(bool hasTexCoords, bool hasNormals, float min = -0.5f, float max = 0.5f);
	std::array<uint32_t, 36> CreateCubeIndices();

	std::array<float, 24> CreateCubeVerticesCompact(float min = -0.5f, float max = 0.5f);
	std::array<uint32_t, 36> CreateCubeIndicesCompact();

	std::vector<float> CreatePlaneVertices(bool hasTexCoords, bool hasNormals, float min = -0.5f, float max = 0.5f);
	std::array<uint32_t, 6> CreatePlaneIndices();

	// has tangent space
	std::array<Vertex, 36> CreateCubeVerticesEx(float min = -0.5f, float max = 0.5f);
	std::array<uint32_t, 36> CreateCubeIndicesEx();
	std::array<Vertex, 6> CreatePlaneVerticesEx(float min = -0.5f, float max = 0.5f);
	std::array<uint32_t, 6> CreatePlaneIndicesEx();

}