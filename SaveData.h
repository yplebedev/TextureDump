#pragma once

#include <reshade.hpp>
#include <string>

using namespace reshade;
using namespace log;

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