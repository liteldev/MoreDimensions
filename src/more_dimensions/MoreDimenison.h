#pragma once

#include "ll/api/mod/NativeMod.h"

namespace more_dimensions {

class MoreDimenison {

public:
    static MoreDimenison& getInstance();

    MoreDimenison() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the plugin is loaded successfully.
    bool load();

    /// @return True if the plugin is enabled successfully.
    bool enable();

    /// @return True if the plugin is disabled successfully.
    bool disable();

    // TODO: Implement this method if you need to unload the plugin.
    // /// @return True if the plugin is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace more_dimensions
