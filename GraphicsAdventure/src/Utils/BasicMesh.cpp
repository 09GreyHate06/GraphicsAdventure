#include "BasicMesh.h"

namespace GA::Utils
{
	std::vector<float> CreateCubeVertices(bool hasTexCoords, bool hasNormals)
	{
		std::vector<float> vertices;
		if (hasTexCoords && hasNormals)
		{
			vertices =
			{
				//  back		             						  
				0.5f, 0.5f, 0.5f,    0.0f,	     0.0f,           0.0f, 0.0f, 1.0f,
				-0.5f, 0.5f, 0.5f,    1.0f,  0.0f,           0.0f, 0.0f, 1.0f,
				-0.5f, -0.5f, 0.5f,    1.0f,  1.0f,	     0.0f, 0.0f, 1.0f,
				0.5f, -0.5f, 0.5f,    0.0f,	     1.0f,	     0.0f, 0.0f, 1.0f,

				// front
				-0.5f, 0.5f, -0.5f,    0.0f,	     0.0f,          0.0f, 0.0f, -1.0f,
				0.5f, 0.5f, -0.5f,    1.0f,  0.0f,          0.0f, 0.0f, -1.0f,
				0.5f, -0.5f, -0.5f,    1.0f,  1.0f,     0.0f, 0.0f, -1.0f,
				-0.5f, -0.5f, -0.5f,    0.0f,	     1.0f,     0.0f, 0.0f, -1.0f,

				// bottom			     	      
				0.5f, -0.5f, 0.5f,    0.0f,	     0.0f,          0.0f, -1.0f,  0.0,
				-0.5f, -0.5f, 0.5f,    1.0f,  0.0f,          0.0f, -1.0f,  0.0,
				-0.5f, -0.5f, -0.5f,    1.0f,  1.0f,     0.0f, -1.0f,  0.0,
				0.5f, -0.5f, -0.5f,    0.0f,	     1.0f,     0.0f, -1.0f,  0.0,

				// top		      	     					  
				-0.5f, 0.5f, 0.5f,    0.0f,	     0.0f,           0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, 0.5f,    1.0f,  0.0f,           0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, -0.5f,    1.0f,  1.0f,      0.0f, 1.0f, 0.0f,
				-0.5f, 0.5f, -0.5f,    0.0f,	     1.0f,      0.0f, 1.0f, 0.0f,

				// left				     	     
				-0.5f, 0.5f, 0.5f,    0.0f,	     0.0f,     	    -1.0f, 0.0f,  0.0,
				-0.5f, 0.5f, -0.5f,    1.0f,  0.0f,     	    -1.0f, 0.0f,  0.0,
				-0.5f, -0.5f, -0.5f,    1.0f,  1.0f,     -1.0f, 0.0f,  0.0,
				-0.5f, -0.5f, 0.5f,    0.0f,	     1.0f,     -1.0f, 0.0f,  0.0,

				// right			             
				0.5f, 0.5f, -0.5f,    0.0f,	     0.0f,           1.0f, 0.0f, 0.0f,
				0.5f, 0.5f, 0.5f,    1.0f,  0.0f,           1.0f, 0.0f, 0.0f,
				0.5f, -0.5f, 0.5f,    1.0f,  1.0f,      1.0f, 0.0f, 0.0f,
				0.5f, -0.5f, -0.5f,    0.0f,	     1.0f,      1.0f, 0.0f, 0.0f,
			};
		}
		else if (hasTexCoords && !hasNormals)
		{
			vertices =
			{
				//  back		             						  
				0.5f, 0.5f, 0.5f,    0.0f,	     0.0f,
				-0.5f, 0.5f, 0.5f,    1.0f,  0.0f,
				-0.5f, -0.5f, 0.5f,    1.0f,  1.0f,
				0.5f, -0.5f, 0.5f,    0.0f,	     1.0f,

				// front
				-0.5f, 0.5f, -0.5f,    0.0f,	     0.0f,
				0.5f, 0.5f, -0.5f,    1.0f,  0.0f,
				0.5f, -0.5f, -0.5f,    1.0f,  1.0f,
				-0.5f, -0.5f, -0.5f,    0.0f,	     1.0f,

				// bottom			     	      					 
				0.5f, -0.5f, 0.5f,    0.0f,	     0.0f,
				-0.5f, -0.5f, 0.5f,    1.0f,  0.0f,
				-0.5f, -0.5f, -0.5f,    1.0f,  1.0f,
				0.5f, -0.5f, -0.5f,    0.0f,	     1.0f,

				// top		      	     					  
				-0.5f, 0.5f, 0.5f,    0.0f,	     0.0f,
				0.5f, 0.5f, 0.5f,    1.0f,  0.0f,
				0.5f, 0.5f, -0.5f,    1.0f,  1.0f,
				-0.5f, 0.5f, -0.5f,    0.0f,	     1.0f,

				// left				     	     
				-0.5f, 0.5f, 0.5f,    0.0f,	     0.0f,
				-0.5f, 0.5f, -0.5f,    1.0f,  0.0f,
				-0.5f, -0.5f, -0.5f,    1.0f,  1.0f,
				-0.5f, -0.5f, 0.5f,    0.0f,	     1.0f,

				// right			             
				0.5f, 0.5f, -0.5f,    0.0f,	     0.0f,
				0.5f, 0.5f, 0.5f,    1.0f,  0.0f,
				0.5f, -0.5f, 0.5f,    1.0f,  1.0f,
				0.5f, -0.5f, -0.5f,    0.0f,	     1.0f,
			};
		}
		else if (!hasTexCoords && hasNormals)
		{
			vertices =
			{
				//  back		             						  
				0.5f, 0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
				-0.5f, 0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
				-0.5f, -0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
				0.5f, -0.5f, 0.5f,    0.0f, 0.0f, 1.0f,

				// front
				-0.5f, 0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
				0.5f, 0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
				0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
				-0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,

				// bottom			     	      
				0.5f, -0.5f, 0.5f,    0.0f, -1.0f,  0.0,
				-0.5f, -0.5f, 0.5f,    0.0f, -1.0f,  0.0,
				-0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0,
				0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0,

				// top		      	     					  
				-0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, 0.5f,    0.0f, 1.0f, 0.0f,
				0.5f, 0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
				-0.5f, 0.5f, -0.5f,    0.0f, 1.0f, 0.0f,

				// left				     	     
				-0.5f, 0.5f, 0.5f,   -1.0f, 0.0f,  0.0,
				-0.5f, 0.5f, -0.5f,   -1.0f, 0.0f,  0.0,
				-0.5f, -0.5f, -0.5f,   -1.0f, 0.0f,  0.0,
				-0.5f, -0.5f, 0.5f,   -1.0f, 0.0f,  0.0,

				// right			             
				0.5f, 0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
				0.5f, 0.5f, 0.5f,    1.0f, 0.0f, 0.0f,
				0.5f, -0.5f, 0.5f,    1.0f, 0.0f, 0.0f,
				0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
			};

		}
		else
		{
			vertices =
			{
				//  back		             						  
				0.5f, 0.5f, 0.5f,
				-0.5f, 0.5f, 0.5f,
				-0.5f, -0.5f, 0.5f,
				0.5f, -0.5f, 0.5f,

				// front
				-0.5f, 0.5f, -0.5f,
				0.5f, 0.5f, -0.5f,
				0.5f, -0.5f, -0.5f,
				-0.5f, -0.5f, -0.5f,

				// bottom			     	      
				0.5f, -0.5f, 0.5f,
				-0.5f, -0.5f, 0.5f,
				-0.5f, -0.5f, -0.5f,
				0.5f, -0.5f, -0.5f,

				// top		      	     					  
				-0.5f, 0.5f, 0.5f,
				0.5f, 0.5f, 0.5f,
				0.5f, 0.5f, -0.5f,
				-0.5f, 0.5f, -0.5f,

				// left				     	     
				-0.5f, 0.5f, 0.5f,
				-0.5f, 0.5f, -0.5f,
				-0.5f, -0.5f, -0.5f,
				-0.5f, -0.5f, 0.5f,

				// right			             
				0.5f, 0.5f, -0.5f,
				0.5f, 0.5f, 0.5f,
				0.5f, -0.5f, 0.5f,
				0.5f, -0.5f, -0.5f,
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

	std::vector<float> CreatePlaneVertices(bool hasTexCoords, bool hasNormals)
	{
		std::vector<float> vertices;

		if (hasTexCoords && hasNormals)
		{
			vertices =
			{
				-0.5f, 0.0f, 0.5f,   0.0f,      0.0f,       0.0f, 1.0f, 0.0f,
				0.5f, 0.0f, 0.5f,   1.0f, 0.0f,       0.0f, 1.0f, 0.0f,
				0.5f, 0.0f, -0.5f,   1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
				-0.5f, 0.0f, -0.5f,   0.0f,      1.0f,  0.0f, 1.0f, 0.0f,
			};
		}
		else if (hasTexCoords && !hasNormals)
		{
			vertices =
			{
				-0.5f, 0.0f, 0.5f,   0.0f,      0.0f,
				0.5f, 0.0f, 0.5f,   1.0f, 0.0f,
				0.5f, 0.0f, -0.5f,   1.0f, 1.0f,
				-0.5f, 0.0f, -0.5f,   0.0f,      1.0f,
			};
		}
		else if (!hasTexCoords && hasNormals)
		{
			vertices =
			{
				-0.5f, 0.0f, 0.5f,   0.0f, 1.0f, 0.0f,
				0.5f, 0.0f, 0.5f,   0.0f, 1.0f, 0.0f,
				0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,
				-0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,
			};
		}
		else
		{
			vertices =
			{
				-0.5f, 0.0f, 0.5f,
				0.5f, 0.0f, 0.5f,
				0.5f, 0.0f, -0.5f,
				-0.5f, 0.0f, -0.5f,
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

}
