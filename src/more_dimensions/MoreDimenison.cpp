#include "MoreDimenison.h"

#include "ll/api/Versions.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/service/ServerInfo.h"


#include <memory>

namespace more_dimensions {

MoreDimenison& MoreDimenison::getInstance() {
    static MoreDimenison instance;
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
    return true;
}

bool MoreDimenison::disable() {
    getSelf().getLogger().info("Disabling...");
    return true;
}

} // namespace more_dimensions

LL_REGISTER_MOD(more_dimensions::MoreDimenison, more_dimensions::MoreDimenison::getInstance());
