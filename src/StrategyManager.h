#pragma once

#include "ISavingStrategy.h"
#include "SavingStrategiesImpl.h"


class StrategyManager {
    public:
        void iterate_pick_top(const SaveData& data) const {
            for (ISavingStrategy *strategy : strategies) {
                if (strategy->match(data.format)) {
                    try {
                        strategy->save(data);
                    } catch (...) {
                        message(level::error, "Exception occurred while a strategy was executing!");
                        MessageBoxA(nullptr, "Runtime error!", "Exception occurred while saving the texture(s).\nPlease report to developer!", MB_ICONEXCLAMATION);
                    }
                    break;
                }
            }
        }

        StrategyManager(std::initializer_list<ISavingStrategy*> initial) {
            for (auto s : initial) strategies.push_back(s);
        }

        ~StrategyManager() {
            for (auto s : strategies) delete s;
            strategies.clear();
        }
    private:
        std::vector<ISavingStrategy*> strategies;
};