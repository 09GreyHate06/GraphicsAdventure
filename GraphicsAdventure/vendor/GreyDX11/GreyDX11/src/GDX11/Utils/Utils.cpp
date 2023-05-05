#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <fstream>
#include "GDX11/Core/GDX11Assert.h"

namespace GDX11::Utils
{
	std::string LoadText(const std::string& filename)
	{
		std::string result;
		std::ifstream in(filename, std::ios::in | std::ios::binary);

		GDX11_CORE_ASSERT(in, "Failed to load text file {0}", filename);

		in.seekg(0, std::ios::end);
		result.resize(static_cast<uint32_t>(in.tellg()));
		in.seekg(0, std::ios::beg);
		in.read(result.data(), result.size());
		in.close();

		return result;
	}

	ImageData LoadImageFile(const std::string& filename, bool flipImageY, int reqComponents)
	{
		ImageData data = {};
		stbi_set_flip_vertically_on_load(flipImageY);
		data.pixels = stbi_load(filename.c_str(), &data.width, &data.height, &data.nrComponents, reqComponents);

		GDX11_CORE_ASSERT(data.pixels, "Failed to load image file: {0}", filename);

		return data;
	}

	void FreeImageData(ImageData* data)
	{
		stbi_image_free(data->pixels);
		data->pixels = nullptr;
	}

	std::wstring ToWideString(const std::string& str)
	{
		if (str.empty()) return std::wstring();

		// determine required length of new string
		size_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), 0, 0);

		// construct new string of required length
		std::wstring ret(reqLength, L'\0');

		// convert old string to new string
		::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &ret[0], (int)ret.length());

		return ret;
	}
}

