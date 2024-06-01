#include "MoreDimenison.h"

#include "ll/api/plugin/NativePlugin.h"
#include "ll/api/plugin/RegisterHelper.h"
#include "ll/api/service/ServerInfo.h"

#include <memory>

namespace more_dimensions {

std::unique_ptr<MoreDimenison>& MoreDimenison::getInstance() {
    static std::unique_ptr<MoreDimenison> instance;
    return instance;
}

bool MoreDimenison::load() {
    getSelf().getLogger().info("Loading...");
    if (ll::getLoaderVersion() < ll::data::Version{0, 8, 3}) {
        getSelf().getLogger().error(
            "The LeviLamina version requires 0.8.3 or higher, now is {}",
            ll::getLoaderVersion().to_string()
        );
        return false;
    }
    getSelf().getLogger().info("More dimension is loaded");
    getSelf().getLogger().info("Version: {}, Developer: LiteLDev", getSelf().getManifest().version->to_string());
    return true;
}

bool MoreDimenison::enable() {
    getSelf().getLogger().info("Enabling...");
    // Code for enabling the plugin goes here.
    return true;
}

bool MoreDimenison::disable() {
    getSelf().getLogger().info("Disabling...");
    // Code for disabling the plugin goes here.
    return true;
}

} // namespace more_dimensions

LL_REGISTER_PLUGIN(more_dimensions::MoreDimenison, more_dimensions::MoreDimenison::getInstance());
