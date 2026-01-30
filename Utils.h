#pragma once

#include "NamedTexture.h"
#include "SaveData.h"
#include "StrategyManager.h"

#include <reshade.hpp>
using namespace reshade;
using namespace api;

inline subresource_data map_data(device *device, resource source) {
    subresource_data data;
    device->map_texture_region(source, 0, ENTIRE_RESOURCE, api::map_access::read_only, &data);
    return data;
}

inline void save_texture(effect_runtime *runtime, command_list *cmd_list, const NamedTexture& texture) {
    resource intermediate = texture.clone_resource(runtime);
    texture.safe_copy_contents(intermediate, runtime, cmd_list);
    subresource_data host_data = map_data(runtime->get_device(), intermediate);
    if (!host_data.data) return;

    StrategyManager manager({new SaveEXRStrategy, new SavePNGStrategy, new SaveBinaryStrategy});

    SaveData save_data(texture.get_format(runtime), texture.name, host_data.data, texture.get_desc(runtime).texture.width, texture.get_desc(runtime).texture.height, host_data.row_pitch);
    manager.iterate_pick_top(save_data);
}