#include "BasicMesh.h"

using namespace DirectX;

namespace GA::Utils
{
	struct TangentSpace
	{
		DirectX::XMFLOAT3 t, b, n;
	};

	TangentSpace CalcTangentSpace(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2, 
		const DirectX::XMFLOAT2& uv0, const DirectX::XMFLOAT2& uv1, const DirectX::XMFLOAT2& uv2)
	{
		// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
		// [edge] = [deltaUV][tb]
		// [tb] = [deltaUV]^-1 [edge]

		XMVECTOR xmP0 = XMLoadFloat3(&p0);
		XMVECTOR xmP1 = XMLoadFloat3(&p1);
		XMVECTOR xmP2 = XMLoadFloat3(&p2);

		XMVECTOR xmUV0 = XMLoadFloat2(&uv0);
		XMVECTOR xmUV1 = XMLoadFloat2(&uv1);
		XMVECTOR xmUV2 = XMLoadFloat2(&uv2);

		XMVECTOR xmEdge0 = xmP1 - xmP0;
		XMVECTOR xmEdge1 = xmP2 - xmP0;

		XMVECTOR xmDeltaUV0 = xmUV1 - xmUV0;
		XMVECTOR xmDeltaUV1 = xmUV2 - xmUV0;

		XMFLOAT3 edge0;
		XMFLOAT3 edge1;
		XMFLOAT2 deltaUV0;
		XMFLOAT2 deltaUV1;
		XMStoreFloat3(&edge0, xmEdge0);
		XMStoreFloat3(&edge1, xmEdge1);
		XMStoreFloat2(&deltaUV0, xmDeltaUV0);
		XMStoreFloat2(&deltaUV1, xmDeltaUV1);
		
		float deltaUVInvDet = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);

		XMFLOAT3 tangent = {};
		tangent.x = deltaUVInvDet * (deltaUV1.y * edge0.x - deltaUV0.y * edge1.x);
		tangent.y = deltaUVInvDet * (deltaUV1.y * edge0.y - deltaUV0.y * edge1.y);
		tangent.z = deltaUVInvDet * (deltaUV1.y * edge0.z - deltaUV0.y * edge1.z);

		XMFLOAT3 bitangent = {};
		bitangent.x = deltaUVInvDet * (-deltaUV1.x * edge0.x + deltaUV0.x * edge1.x);
		bitangent.y = deltaUVInvDet * (-deltaUV1.x * edge0.y + deltaUV0.x * edge1.y);
		bitangent.z = deltaUVInvDet * (-deltaUV1.x * edge0.z + deltaUV0.x * edge1.z);

		XMFLOAT3 normal;
		XMStoreFloat3(&normal, XMVector3Cross(XMLoadFloat3(&tangent), XMLoadFloat3(&bitangent)));

		return { tangent, bitangent, normal };
	}


	std::vector<float> CreateCubeVertices(bool hasTexCoords, bool hasNormals, float min, float max)
	{
		std::vector<float> vertices;
		if (hasTexCoords && hasNormals)
		{
			vertices =
			{
				//  back		             						  
				max, max, max,    0.0f,	 0.0f,           0.0f, 0.0f, 1.0f,
				min, max, max,    1.0f,  0.0f,           0.0f, 0.0f, 1.0f,
				min, min, max,    1.0f,  1.0f,	     0.0f, 0.0f, 1.0f,
				max, min, max,    0.0f,	 1.0f,	     0.0f, 0.0f, 1.0f,

				// front
				min, max, min,    0.0f,	 0.0f,          0.0f, 0.0f, -1.0f,
				max, max, min,    1.0f,  0.0f,          0.0f, 0.0f, -1.0f,
				max, min, min,    1.0f,  1.0f,     0.0f, 0.0f, -1.0f,
				min, min, min,    0.0f,	 1.0f,     0.0f, 0.0f, -1.0f,

				// bottom			     	      
				min, min, min,    0.0f,	 0.0f,          0.0f, -1.0f,  0.0,
				max, min, min,    1.0f,  0.0f,          0.0f, -1.0f,  0.0,
				max, min, max,    1.0f,  1.0f,     0.0f, -1.0f,  0.0,
				min, min, max,    0.0f,	 1.0f,     0.0f, -1.0f,  0.0,

				// top		      	     					  
				min, max, max,    0.0f,	 0.0f,           0.0f, 1.0f, 0.0f,
				max, max, max,    1.0f,  0.0f,           0.0f, 1.0f, 0.0f,
				max, max, min,    1.0f,  1.0f,      0.0f, 1.0f, 0.0f,
				min, max, min,    0.0f,	 1.0f,      0.0f, 1.0f, 0.0f,

				// left				     	     
				min, max, max,    0.0f,	 0.0f,     	    -1.0f, 0.0f,  0.0,
				min, max, min,    1.0f,  0.0f,     	    -1.0f, 0.0f,  0.0,
				min, min, min,    1.0f,  1.0f,     -1.0f, 0.0f,  0.0,
				min, min, max,    0.0f,	 1.0f,     -1.0f, 0.0f,  0.0,

				// right			             
				max, max, min,    0.0f,	 0.0f,           1.0f, 0.0f, 0.0f,
				max, max, max,    1.0f,  0.0f,           1.0f, 0.0f, 0.0f,
				max, min, max,    1.0f,  1.0f,      1.0f, 0.0f, 0.0f,
				max, min, min,    0.0f,	 1.0f,      1.0f, 0.0f, 0.0f,
			};
		}
		else if (hasTexCoords && !hasNormals)
		{
			vertices =
			{
				//  back		             						  
				max, max, max,    0.0f,	     0.0f,
				min, max, max,    1.0f,  0.0f,
				min, min, max,    1.0f,  1.0f,
				max, min, max,    0.0f,	     1.0f,

				// front
				min, max, min,    0.0f,	     0.0f,
				max, max, min,    1.0f,  0.0f,
				max, min, min,    1.0f,  1.0f,
				min, min, min,    0.0f,	     1.0f,

				// bottom			     	      					 
				min, min, min,   0.0f,	     0.0f,
				max, min, min,   1.0f,  0.0f,
				max, min, max,   1.0f,  1.0f,
				min, min, max,   0.0f,	     1.0f,

				// top		      	     					  
				min, max, max,    0.0f,	     0.0f,
				max, max, max,    1.0f,  0.0f,
				max, max, min,    1.0f,  1.0f,
				min, max, min,    0.0f,	     1.0f,

				// left				     	     
				min, max, max,    0.0f,	     0.0f,
				min, max, min,    1.0f,  0.0f,
				min, min, min,    1.0f,  1.0f,
				min, min, max,    0.0f,	     1.0f,

				// right			             
				max, max, min,    0.0f,	     0.0f,
				max, max, max,    1.0f,  0.0f,
				max, min, max,    1.0f,  1.0f,
				max, min, min,    0.0f,	     1.0f,
			};
		}
		else if (!hasTexCoords && hasNormals)
		{
			vertices =
			{
				//  back		             						  
				max, max, max,    0.0f, 0.0f, 1.0f,
				min, max, max,    0.0f, 0.0f, 1.0f,
				min, min, max,    0.0f, 0.0f, 1.0f,
				max, min, max,    0.0f, 0.0f, 1.0f,

				// front
				min, max, min,    0.0f, 0.0f, -1.0f,
				max, max, min,    0.0f, 0.0f, -1.0f,
				max, min, min,    0.0f, 0.0f, -1.0f,
				min, min, min,    0.0f, 0.0f, -1.0f,

				// bottom			     	      
				min, min, min,    0.0f, -1.0f,  0.0,
				max, min, min,    0.0f, -1.0f,  0.0,
				max, min, max,    0.0f, -1.0f,  0.0,
				min, min, max,    0.0f, -1.0f,  0.0,

				// top		      	     					  
				min, max, max,    0.0f, 1.0f, 0.0f,
				max, max, max,    0.0f, 1.0f, 0.0f,
				max, max, min,    0.0f, 1.0f, 0.0f,
				min, max, min,    0.0f, 1.0f, 0.0f,

				// left				     	     
				min, max, max,   -1.0f, 0.0f,  0.0,
				min, max, min,   -1.0f, 0.0f,  0.0,
				min, min, min,   -1.0f, 0.0f,  0.0,
				min, min, max,   -1.0f, 0.0f,  0.0,

				// right			             
				max, max, min,    1.0f, 0.0f, 0.0f,
				max, max, max,    1.0f, 0.0f, 0.0f,
				max, min, max,    1.0f, 0.0f, 0.0f,
				max, min, min,    1.0f, 0.0f, 0.0f,
			};

		}
		else
		{
			vertices =
			{
				//  back		             						  
				max, max, max,
				min, max, max,
				min, min, max,
				max, min, max,

				// front
				min, max, min,
				max, max, min,
				max, min, min,
				min, min, min,

				// bottom			     	      
				min, min, min,
				max, min, min,
				max, min, max,
				min, min, max,

				// top		      	     					  
				min, max, max,
				max, max, max,
				max, max, min,
				min, max, min,

				// left				     	     
				min, max, max,
				min, max, min,
				min, min, min,
				min, min, max,

				// right			             
				max, max, min,
				max, max, max,
				max, min, max,
				max, min, min,
			};
		}

		return vertices;
	}

	std::array<float, 24> CreateCubeVerticesCompact(float min, float max)
	{
		return std::array<float, 24>{
			// front plane
			min, max, min,
			max, max, min,
			max, min, min,
			min, min, min,

			// back plane
			max, max, max,
			min, max, max,
			min, min, max,
			max, min, max
		};
	}

	std::vector<float> CreatePlaneVertices(bool hasTexCoords, bool hasNormals, float min, float max)
	{
		std::vector<float> vertices;

		if (hasTexCoords && hasNormals)
		{
			vertices =
			{
				min, 0.0f, max,   0.0f, 0.0f,       0.0f, 1.0f, 0.0f,
				max, 0.0f, max,   1.0f, 0.0f,       0.0f, 1.0f, 0.0f,
				max, 0.0f, min,   1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
				min, 0.0f, min,   0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
			};
		}
		else if (hasTexCoords && !hasNormals)
		{
			vertices =
			{
				min, 0.0f, max,   0.0f,      0.0f,
				max, 0.0f, max,   1.0f, 0.0f,
				max, 0.0f, min,   1.0f, 1.0f,
				min, 0.0f, min,   0.0f,      1.0f,
			};
		}
		else if (!hasTexCoords && hasNormals)
		{
			vertices =
			{
				min, 0.0f, max,   0.0f, 1.0f, 0.0f,
				max, 0.0f, max,   0.0f, 1.0f, 0.0f,
				max, 0.0f, min,   0.0f, 1.0f, 0.0f,
				min, 0.0f, min,   0.0f, 1.0f, 0.0f,
			};
		}
		else
		{
			vertices =
			{
				min, 0.0f, max,
				max, 0.0f, max,
				max, 0.0f, min,
				min, 0.0f, min,
			};
		}

		return vertices;
	}

	std::array<uint32_t, 36> CreateCubeIndices()
	{
		return std::move(std::array<uint32_t, 36>
		{
			// back
			0, 1, 2,
			2, 3, 0,

			// front
			4, 5, 6,
			6, 7, 4,

			// left
			8, 9, 10,
			10, 11, 8,

			// right
			12, 13, 14,
			14, 15, 12,

			// bottom
			16, 17, 18,
			18, 19, 16,

			// top
			20, 21, 22,
			22, 23, 20
		});
	}

	std::array<uint32_t, 36> CreateCubeIndicesCompact()
	{
		return std::array<uint32_t, 36>{
			// front
			0, 1, 2,
			2, 3, 0,

			// back
			4, 5, 6,
			6, 7, 4,

			// right
			1, 4, 7,
			7, 2, 1,

			// left
			5, 0, 3,
			3, 6, 5,

			// top
			5, 4, 1,
			1, 0, 5,

			// bottom
			3, 2, 7,
			7, 6, 3
		};
	}

	std::array<uint32_t, 6> CreatePlaneIndices()
	{
		return std::move(std::array<uint32_t, 6>
		{
			0, 1, 2,
			2, 3, 0,
		});
	}

	std::array<Vertex, 36> CreateCubeVerticesEx(float min, float max)
	{
		XMFLOAT2 uv0 = { 0.0f, 0.0f };
		XMFLOAT2 uv1 = { 1.0f, 0.0f };
		XMFLOAT2 uv2 = { 1.0f, 1.0f };
		XMFLOAT2 uv3 = { 0.0f, 1.0f };

		// front pos
		XMFLOAT3 frontP0 = { min, max, min };
		XMFLOAT3 frontP1 = { max, max, min };
		XMFLOAT3 frontP2 = { max, min, min };
		XMFLOAT3 frontP3 = { min, min, min };

		TangentSpace frontTS012 = CalcTangentSpace(frontP0, frontP1, frontP2, uv0, uv1, uv2);
		TangentSpace frontTS230 = CalcTangentSpace(frontP2, frontP3, frontP0, uv2, uv3, uv0);

		// back pos
		XMFLOAT3 backP0 = { max, max, max };
		XMFLOAT3 backP1 = { min, max, max };
		XMFLOAT3 backP2 = { min, min, max };
		XMFLOAT3 backP3 = { max, min, max };

		TangentSpace backTS012 = CalcTangentSpace(backP0, backP1, backP2, uv0, uv1, uv2);
		TangentSpace backTS230 = CalcTangentSpace(backP2, backP3, backP0, uv2, uv3, uv0);

		// top pos
		XMFLOAT3 topP0 = { min, max, max };
		XMFLOAT3 topP1 = { max, max, max };
		XMFLOAT3 topP2 = { max, max, min };
		XMFLOAT3 topP3 = { min, max, min };

		TangentSpace topTS012 = CalcTangentSpace(topP0, topP1, topP2, uv0, uv1, uv2);
		TangentSpace topTS230 = CalcTangentSpace(topP2, topP3, topP0, uv2, uv3, uv0);

		// bottom pos
		XMFLOAT3 bottomP0 = { min, min, min };
		XMFLOAT3 bottomP1 = { max, min, min };
		XMFLOAT3 bottomP2 = { max, min, max };
		XMFLOAT3 bottomP3 = { min, min, max };

		TangentSpace bottomTS012 = CalcTangentSpace(bottomP0, bottomP1, bottomP2, uv0, uv1, uv2);
		TangentSpace bottomTS230 = CalcTangentSpace(bottomP2, bottomP3, bottomP0, uv2, uv3, uv0);

		// right pos
		XMFLOAT3 rightP0 = { max, max, min };
		XMFLOAT3 rightP1 = { max, max, max };
		XMFLOAT3 rightP2 = { max, min, max };
		XMFLOAT3 rightP3 = { max, min, min };

		TangentSpace rightTS012 = CalcTangentSpace(rightP0, rightP1, rightP2, uv0, uv1, uv2);
		TangentSpace rightTS230 = CalcTangentSpace(rightP2, rightP3, rightP0, uv2, uv3, uv0);

		// left pos
		XMFLOAT3 leftP0 = { min, max, max };
		XMFLOAT3 leftP1 = { min, max, min };
		XMFLOAT3 leftP2 = { min, min, min };
		XMFLOAT3 leftP3 = { min, min, max };

		TangentSpace leftTS012 = CalcTangentSpace(leftP0, leftP1, leftP2, uv0, uv1, uv2);
		TangentSpace leftTS230 = CalcTangentSpace(leftP2, leftP3, leftP0, uv2, uv3, uv0);

		return std::array<Vertex, 36>
		{
			// front 
			frontP0, uv0, frontTS012.t, frontTS012.b, frontTS012.n,
			frontP1, uv1, frontTS012.t, frontTS012.b, frontTS012.n,
			frontP2, uv2, frontTS012.t, frontTS012.b, frontTS012.n,
			frontP2, uv2, frontTS230.t, frontTS230.b, frontTS230.n,
			frontP3, uv3, frontTS230.t, frontTS230.b, frontTS230.n,
			frontP0, uv0, frontTS230.t, frontTS230.b, frontTS230.n,

			// back
			backP0, uv0, backTS012.t, backTS012.b, backTS012.n,
			backP1, uv1, backTS012.t, backTS012.b, backTS012.n,
			backP2, uv2, backTS012.t, backTS012.b, backTS012.n,
			backP2, uv2, backTS230.t, backTS230.b, backTS230.n,
			backP3, uv3, backTS230.t, backTS230.b, backTS230.n,
			backP0, uv0, backTS230.t, backTS230.b, backTS230.n,

			// top 
			topP0, uv0, topTS012.t, topTS012.b, topTS012.n,
			topP1, uv1, topTS012.t, topTS012.b, topTS012.n,
			topP2, uv2, topTS012.t, topTS012.b, topTS012.n,
			topP2, uv2, topTS230.t, topTS230.b, topTS230.n,
			topP3, uv3, topTS230.t, topTS230.b, topTS230.n,
			topP0, uv0, topTS230.t, topTS230.b, topTS230.n,

			// bottom
			bottomP0, uv0, bottomTS012.t, bottomTS012.b, bottomTS012.n,
			bottomP1, uv1, bottomTS012.t, bottomTS012.b, bottomTS012.n,
			bottomP2, uv2, bottomTS012.t, bottomTS012.b, bottomTS012.n,
			bottomP2, uv2, bottomTS230.t, bottomTS230.b, bottomTS230.n,
			bottomP3, uv3, bottomTS230.t, bottomTS230.b, bottomTS230.n,
			bottomP0, uv0, bottomTS230.t, bottomTS230.b, bottomTS230.n,

			// right
			rightP0, uv0, rightTS012.t, rightTS012.b, rightTS012.n,
			rightP1, uv1, rightTS012.t, rightTS012.b, rightTS012.n,
			rightP2, uv2, rightTS012.t, rightTS012.b, rightTS012.n,
			rightP2, uv2, rightTS230.t, rightTS230.b, rightTS230.n,
			rightP3, uv3, rightTS230.t, rightTS230.b, rightTS230.n,
			rightP0, uv0, rightTS230.t, rightTS230.b, rightTS230.n,

			// left
			leftP0, uv0, leftTS012.t, leftTS012.b, leftTS012.n,
			leftP1, uv1, leftTS012.t, leftTS012.b, leftTS012.n,
			leftP2, uv2, leftTS012.t, leftTS012.b, leftTS012.n,
			leftP2, uv2, leftTS230.t, leftTS230.b, leftTS230.n,
			leftP3, uv3, leftTS230.t, leftTS230.b, leftTS230.n,
			leftP0, uv0, leftTS230.t, leftTS230.b, leftTS230.n
		};
	}

	std::array<uint32_t, 36> CreateCubeIndicesEx()
	{
		std::array<uint32_t, 36> ind = {};
		for (int i = 0; i < 36; i++)
			ind[i] = i;

		return ind;
	}

	std::array<Vertex, 6> CreatePlaneVerticesEx(float min, float max)
	{
		XMFLOAT2 uv0 = { 0.0f, 0.0f };
		XMFLOAT2 uv1 = { 1.0f, 0.0f };
		XMFLOAT2 uv2 = { 1.0f, 1.0f };
		XMFLOAT2 uv3 = { 0.0f, 1.0f };

		XMFLOAT3 p0 = { min, 0.0f, max };
		XMFLOAT3 p1 = { max, 0.0f, max };
		XMFLOAT3 p2 = { max, 0.0f, min };
		XMFLOAT3 p3 = { min, 0.0f, min };

		auto ts0 = CalcTangentSpace(p0, p1, p2, uv0, uv1, uv2);
		auto ts1 = CalcTangentSpace(p2, p3, p0, uv2, uv3, uv0);

		return std::array<Vertex, 6> {
			p0, uv0, ts0.t, ts0.b, ts0.n,
			p1, uv1, ts0.t, ts0.b, ts0.n,
			p2, uv2, ts0.t, ts0.b, ts0.n,
			p2, uv2, ts1.t, ts1.b, ts1.n,
			p3, uv3, ts1.t, ts1.b, ts1.n,
			p0, uv0, ts1.t, ts1.b, ts1.n,
		};
	}

	std::array<uint32_t, 6> CreatePlaneIndicesEx()
	{
		std::array<uint32_t, 6> ind = {};
		for (int i = 0; i < 6; i++)
			ind[i] = i;

		return ind;
	}
}
