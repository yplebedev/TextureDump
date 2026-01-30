#pragma once

#include <reshade.hpp>
#include <string>

using namespace reshade;
using namespace log;

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

    void safe_copy_contents(api::resource dest, api::effect_runtime *runtime, api::command_list *cmd_list) const {
        api::device *device = runtime->get_device();
        api::resource source_resource = this->get_resource(runtime);

        api::resource_usage temp_usage = this->get_desc(runtime).usage;
        cmd_list->barrier(source_resource, temp_usage, api::resource_usage::copy_source);
        cmd_list->copy_texture_region(source_resource, 0, nullptr, dest, 0, nullptr);
        cmd_list->barrier(source_resource, api::resource_usage::copy_source, temp_usage);

        api::fence copy_sync_fence;
        if (!device->create_fence(0, api::fence_flags::none, &copy_sync_fence) || runtime->get_command_queue()->signal(copy_sync_fence, 1) || device->wait(copy_sync_fence, 1)) {
            runtime->get_command_queue()->wait_idle();
        }
        device->destroy_fence(copy_sync_fence);
    }

private:
    api::resource_view get_resource_view(const api::effect_runtime *runtime) const {
        api::resource_view rtv { 0 };
        runtime->get_texture_binding(tex_variable, &rtv, nullptr);

        return rtv;
    }
};