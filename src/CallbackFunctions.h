#pragma once

#include "Utils.h"
#include "SavingStrategiesImpl.h"
#include <reshade.hpp>
#include <imgui.h>


static char buffer[32] = "SCGI.fx";
static std::vector<NamedTexture> textures;

inline void on_finish_effects(api::effect_runtime *runtime, api::command_list *cmd_list, api::resource_view rtv, api::resource_view rtv_srgb) {
    if (runtime->is_key_pressed(VK_INSERT)) {
        for (const auto& tex : textures) {
            if (!tex.used) continue;
            save_texture(runtime, cmd_list, tex);
        }
    }
}

inline void initialize(api::effect_runtime* runtime) {
    runtime->set_preprocessor_definition("DEBUG_ADDON", "1");
}

inline void push_textures(api::effect_runtime* runtime, api::effect_texture_variable variable, void* user_data) {
    char tex_buffer[64]{};
    runtime->get_texture_variable_name(variable, tex_buffer);

    NamedTexture texture(false, variable, std::string(tex_buffer));

    textures.push_back(texture);
}

inline int update_effect(ImGuiInputTextCallbackData *cbd) {
    textures.clear();

    auto *runtime = reinterpret_cast<api::effect_runtime*>(cbd->UserData);
    runtime->enumerate_texture_variables(cbd->Buf, push_textures, nullptr);

    for (auto& texture : textures) texture.used = false;
    return 0;
}

inline void UI(api::effect_runtime *runtime) {
    ImGui::TextColored(ImVec4(0.8, 0.9, 1., 1.0), "Hey there! \nKeep in mind most formats are unsupported, so use 32F texture formats (1, 3 or 4 channels), or 8 for ints. \n"
                                                                                "If the exporter doesn't know how to write a format it will do a binary dump that could be ingested byt the likes of ImageMagick. \n"
                                                                                "More file formats could be added and PRed, I tried my best to make it easy for contributors.");

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