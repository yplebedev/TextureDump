/*
	Thank you for contributing your code to the project!


	Hi there!
	If you are a shader developer, you likely found yourself in the wrong place. Get the actual addon from Releases!
	If you are an addon developer, please keep in mind that this project should not be taken as a perfect reference for how one should do any given thing.
	This does one specific thing, relatively well, and nothing more. Check in with ReShade docs and the #addon-programming chat in discord.

	For contributors:
		!!! IMPORTANT !!!
		Declare RESHADE_PATH env variable to the !root! of a cloned repo of ReShade. This is required to compile the project!
*/
#define TINYEXR_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define ENTIRE_RESOURCE nullptr
#define EXR_f32 0
#define abstract_class class

#if DEBUG
	static constexr bool do_debug_logs = true;
#else
	static constexpr bool do_debug_logs = false;
#endif

#define NOMINMAX
#include <imgui.h>
#include <vector>
#include <reshade.hpp>
#include <chrono>
#include <fstream>

#include "tinyexr.h"
#include "stb_image_write.h"

using namespace reshade;
using namespace log;
#define LEVELS 0
#define LAYERS 1
#define SAMPLES 1


class NamedTexture {
	public:
		bool used = false;
		api::effect_texture_variable tex_variable;
		std::string name;

		NamedTexture(bool used, const api::effect_texture_variable tex_variable, const std::string& name) {
			this->used = used;
			this->tex_variable = tex_variable;
			this->name = name;
		}

		api::resource_desc get_desc(api::effect_runtime *runtime) const {
			return runtime->get_device()->get_resource_desc(get_resource(runtime));
		}

		api::resource clone_resource(api::effect_runtime *runtime) const {
			api::resource intermediate;
			api::device *device = runtime->get_device();
			api::resource_desc desc = get_desc(runtime);
			api::resource_desc new_desc(desc.texture.width, desc.texture.height, 1, 1, desc.texture.format, SAMPLES, api::memory_heap::gpu_to_cpu, api::resource_usage::copy_dest);

			if (device->create_resource(new_desc, nullptr, api::resource_usage::copy_dest, &intermediate)) {
				return intermediate;
			}

			throw "Failed to copy resource!";
		}

		api::resource get_resource(api::effect_runtime *runtime) const {
			return runtime->get_device()->get_resource_from_view(get_resource_view(runtime));
		}

		api::format get_format(api::effect_runtime *runtime) const {
			return get_desc(runtime).texture.format;
		}

	private:
		api::resource_view get_resource_view(const api::effect_runtime *runtime) const {
			api::resource_view rtv { 0 };
			runtime->get_texture_binding(tex_variable, &rtv, nullptr);

			return rtv;
		}
};

static char buffer[32] = "SCGI.fx";
static std::vector<NamedTexture> textures;

enum class SaveStatus {
	ok = 0,
	skipped = -1,
	no_texture = 1,
	no_create = 2,
	no_map = 3,
	no_export = 4,
	unhandled_export_path = 5
};

class SaveData {
	public:
		SaveData(api::format format, const std::string& initial_name, void *data, uint32_t w, uint32_t h, uint32_t pitch) {
			this->format = format;
			this->filename = initial_name;
			this->raw_data = data;
			this->w = w; this->h = h;
			this->pitch = pitch;
		}

		uint32_t get_approx_channels() const {
			uint32_t bpc = api::format_bit_depth(format);
			if (!bpc) bpc = 8;
			uint32_t format_pitch = api::format_row_pitch(format, 1);
			return (format_pitch * 8u) / bpc;
		}

		std::string filename;
		api::format format;
		void *raw_data;
		uint32_t w, h;
		uint32_t pitch;
};

abstract_class ISavingStrategy {
	public:
		virtual ~ISavingStrategy() = default;
		virtual void save(SaveData data) = 0;
		virtual bool match(api::format format) = 0;
	private:
		virtual void finalize_filename(SaveData &data) = 0;
		std::set<api::format> supported;
};

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
		std::set<api::format> supported = { api::format::r32g32b32a32_float, api::format::r32g32b32_float, api::format::r32_float };
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
		/*
		uint8_t buffer[288000] {};
		for (int x = 0; x < 480; x++) {
			for (int y = 0; y < 300; y++) {
				int addr = y * 480 + x;
				buffer[2 * addr] = x % 256;
				buffer[2 * addr + 1] = (y / 16) % 2 == 0 ? 255 : 128;
			}
		}*/


		stbi_write_png(data.filename.c_str(), data.w, data.h, channels, data.raw_data, pitch);
	}
	bool match(api::format format) override {
		return supported.contains(api::format_to_typeless(format));
	}
private:
	std::set<api::format> supported = { api::format::r8_typeless, api::format::r8g8_typeless, api::format::r8g8b8a8_typeless };
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
	std::set<api::format> supported;
	void finalize_filename(SaveData &data) override {
		std::stringstream ss;
		ss << data.filename << "_" << data.w << "x" << data.h << ".bin";
		data.filename = ss.str();
	}
};

static std::vector<ISavingStrategy*> strategies;

void safe_copy_contents(const NamedTexture& source, api::resource dest, api::effect_runtime *runtime, api::command_list *cmd_list) {
	api::device *device = runtime->get_device();
	api::resource source_resource = source.get_resource(runtime);

	api::resource_usage temp_usage = source.get_desc(runtime).usage;
	cmd_list->barrier(source_resource, temp_usage, api::resource_usage::copy_source);
	cmd_list->copy_texture_region(source_resource, 0, nullptr, dest, 0, nullptr);
	cmd_list->barrier(source_resource, api::resource_usage::copy_source, temp_usage);

	api::fence copy_sync_fence;
	if (!device->create_fence(0, api::fence_flags::none, &copy_sync_fence) || runtime->get_command_queue()->signal(copy_sync_fence, 1) || device->wait(copy_sync_fence, 1)) {
		runtime->get_command_queue()->wait_idle();
	}
	device->destroy_fence(copy_sync_fence);
}

api::subresource_data map_data(api::device *device, api::resource source) {
	api::subresource_data data;
	device->map_texture_region(source, 0, ENTIRE_RESOURCE, api::map_access::read_only, &data);
	return data;
}

SaveStatus save_texture(api::effect_runtime *runtime, api::command_list *cmd_list, const NamedTexture& texture) {
	api::resource intermediate = texture.clone_resource(runtime);
	safe_copy_contents(texture, intermediate, runtime, cmd_list);
	api::subresource_data host_data = map_data(runtime->get_device(), intermediate);
	if (!host_data.data) return SaveStatus::no_map;

	SaveData save_data(texture.get_format(runtime), texture.name, host_data.data, texture.get_desc(runtime).texture.width, texture.get_desc(runtime).texture.height, host_data.row_pitch);

	assert(!strategies.empty());
	for (ISavingStrategy *strategy : strategies) {
		if (strategy->match(save_data.format)) {
			strategy->save(save_data);
			break;
		}
	}

	return SaveStatus::ok;
}

void on_finish_effects(api::effect_runtime *runtime, api::command_list *cmd_list, api::resource_view rtv, api::resource_view rtv_srgb) {
	if (runtime->is_key_pressed(VK_INSERT)) {
		for (const auto& tex : textures) {
			if (!tex.used) continue;
			save_texture(runtime, cmd_list, tex);
		}
	}
}

void initialize(api::effect_runtime* runtime) {
	runtime->set_preprocessor_definition("DEBUG_ADDON", "1");

	auto *exr_strategy = new SaveEXRStrategy();
	auto *png_strategy = new SavePNGStrategy();
	auto *raw_strategy = new SaveBinaryStrategy();
	strategies.push_back(exr_strategy);
	strategies.push_back(png_strategy);
	strategies.push_back(raw_strategy); // todo: pls reflect on this code.
}

void push_textures(api::effect_runtime* runtime, api::effect_texture_variable variable, void* user_data) {
	char tex_buffer[64]{};
	runtime->get_texture_variable_name(variable, tex_buffer);

	NamedTexture texture(false, variable, std::string(tex_buffer));

	textures.push_back(texture);
}

int update_effect(ImGuiInputTextCallbackData *cbd) {
	textures.clear();

	auto *runtime = reinterpret_cast<api::effect_runtime*>(cbd->UserData);
	runtime->enumerate_texture_variables(cbd->Buf, push_textures, nullptr);

	for (auto& texture : textures) texture.used = false;
	return 0;
}

void UI(api::effect_runtime *runtime) {
	ImGui::TextColored(ImVec4(0.8, 0.9, 1., 1.0), "Hey there! \nKeep in mind most formats are unsupported, so use 32F texture formats (1, 3 or 4 channels), or 8 for ints. \nIf the exporter doesn't know how to write a format it will do a binary dump that could be ingested byt the likes of ImageMagick \nMore file formats could be added and PRed, I tried my best to make it easy for contributors.");

	ImGui::InputText("Effect Name", buffer, IM_ARRAYSIZE(buffer), ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackEdit, update_effect, reinterpret_cast<void *>(runtime));
	if (textures.empty()) {
		ImGui::TextColored(ImVec4(1.0, 0.2, 0.2, 1.0), "No Effect Textures Found!");
	} else {
		if (ImGui::BeginChild("TexSelect", ImVec2(-FLT_MIN, ImGui::GetFontSize() * 20), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY)) {
			for (int n = 0; n < textures.size(); n++) {
				char label[32];
				sprintf_s(label, "Tex %s", textures.at(n).name.c_str());
				ImGui::SetNextItemSelectionUserData(n);
				ImGui::Checkbox(label, (bool*)&textures[n].used);
			}
		}
		ImGui::EndChild();
	}
}

extern "C" __declspec(dllexport) const char *NAME = "Texture Saver";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "Helper addon for debugging and analyzing shaders via external tools.";
BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		if (!reshade::register_addon(hModule))
			return FALSE;

		reshade::register_overlay(nullptr, UI);
		reshade::register_event<reshade::addon_event::init_effect_runtime>(initialize);
		reshade::register_event<reshade::addon_event::reshade_finish_effects>(on_finish_effects);
		break;
	case DLL_PROCESS_DETACH:
		reshade::unregister_addon(hModule);
		break;
	}

	return TRUE;
}