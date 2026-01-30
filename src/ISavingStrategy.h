#pragma once
#define abstract_class class

#include <unordered_set>
#include "SaveData.h"

abstract_class ISavingStrategy {
    public:
        virtual ~ISavingStrategy() = default;
        virtual void save(SaveData data) = 0;
        virtual bool match(api::format format) = 0;
    private:
        virtual void finalize_filename(SaveData &data) = 0;
        std::unordered_set<api::format> supported;
};