#pragma once

#include <string>
#include <iostream>
#include <fstream>

#include "tinyexr.h"
#include "stb_image_write.h"
#include "ISavingStrategy.h"

class SaveEXRStrategy : public ISavingStrategy {
	public:
		void save(SaveData data) override {
			finalize_filename(data);

			const char *err;
			SaveEXR(reinterpret_cast<float*>(data.raw_data), data.w, data.h, data.get_approx_channels(), 0, data.filename.c_str(), &err);
		}
		bool match(api::format format) override {
			return supported.contains(format);
		}
	private:
		std::unordered_set<api::format> supported = { api::format::r32g32b32a32_float, api::format::r32g32b32_float, api::format::r32_float };
		void finalize_filename(SaveData &data) override {
			data.filename = data.filename + ".exr";
		}
};

class SavePNGStrategy : public ISavingStrategy {
public:
	void save(SaveData data) override {
		finalize_filename(data);
		uint32_t channels = data.get_approx_channels();
		uint32_t pitch = data.pitch;

		stbi_write_png(data.filename.c_str(), data.w, data.h, channels, data.raw_data, pitch);
	}
	bool match(api::format format) override {
		return supported.contains(api::format_to_typeless(format));
	}
private:
	std::unordered_set<api::format> supported = { api::format::r8_typeless, api::format::r8g8_typeless, api::format::r8g8b8a8_typeless };
	void finalize_filename(SaveData &data) override {
		data.filename = data.filename + ".png";
	}
};

class SaveBinaryStrategy : public ISavingStrategy {
public:
	void save(SaveData data) override {
		finalize_filename(data);

		std::ofstream file(data.filename, std::ios::binary);
		file.write(reinterpret_cast<const char*>(data.raw_data), data.pitch * data.h);
		file.close();
	}
	bool match(api::format format) override {
		return true;
	}
private:
	std::unordered_set<api::format> supported;
	void finalize_filename(SaveData &data) override {
		std::stringstream ss;
		ss << data.filename << "_" << data.w << "x" << data.h << ".bin";
		data.filename = ss.str();
	}
};