#pragma once

#include "ll/api/base/StdInt.h"

#include "mc/world/level/GeneratorType.h"

#include <string>
#include <unordered_map>


namespace more_dimensions::CustomDimensionConfig {
struct Config {
    struct Info {
        int         dimId{};
        std::string base64Nbt;
    };
    int                                   version = 3;
    std::unordered_map<std::string, Info> dimensionList{};
};

static Config& getConfig() {
    static Config instance;
    return instance;
}

void setDimensionConfigPath();
bool loadConfigFile();
bool saveConfigFile();
}; // namespace more_dimensions::CustomDimensionConfig
