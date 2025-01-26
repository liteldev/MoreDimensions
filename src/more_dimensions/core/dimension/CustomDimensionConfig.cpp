
#include "CustomDimensionConfig.h"

#include "more_dimensions/MoreDimenison.h"

#include "ll/api/Config.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/utils/ErrorUtils.h"
#include "mc/server/PropertiesSettings.h"

namespace more_dimensions::CustomDimensionConfig {

//static ll::Logger            logger("CustomDimensionConfig");
auto& logger = MoreDimenison::getInstance().getSelf().getLogger();

static std::filesystem::path dimensionConfigPath{u8"./worlds"};

void setDimensionConfigPath() {
    if (!ll::service::getLevel()) {
        throw std::runtime_error("Level nullptr");
    }
    dimensionConfigPath /= ll::string_utils::str2u8str(ll::service::getPropertiesSettings()->getLevelName());
    dimensionConfigPath /= u8"dimension_config.json";
}

bool loadConfigFile() {
    if (std::ifstream(dimensionConfigPath).good()) {
        try {
            if (ll::config::loadConfig(getConfig(), dimensionConfigPath)) {
                logger.info("Config file load success!");
                return true;
            }
        } catch (...) {
            logger.error("Config file load fail, will rewrite!");
            ll::error_utils::printCurrentException(logger);
        }
    }
    try {
        if (ll::config::saveConfig(getConfig(), dimensionConfigPath)) {
            logger.warn("Config file rewrite success!");
            return true;
        } else {
            logger.error("Config rewrite failed!");
        }
    } catch (...) {
        logger.error("Config rewrite failed!");
        ll::error_utils::printCurrentException(logger);
    }
    return false;
}

bool saveConfigFile() {
    bool result = false;
    try {
        result = ll::config::saveConfig(getConfig(), dimensionConfigPath);
    } catch (...) {
        result = false;
        ll::error_utils::printCurrentException(logger);
    }
    if (!result) {
        logger.error("Config file save fail!");
        return false;
    }
    return true;
}
} // namespace more_dimensions::CustomDimensionConfig
