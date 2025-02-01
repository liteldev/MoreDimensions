#pragma once

#include "mc/deps/core/utility/AutomaticID.h"
#include "mc/network/packet/Packet.h"
#include "mc/platform/UUID.h"


#include <mutex>
#include <unordered_map>

class ServerPlayer;

namespace more_dimensions {

class FakeDimensionId {
    struct CustomDimensionIdSetting {
        bool needRemovePacket{};
    };
    std::mutex mMapMutex;

    std::unordered_map<mce::UUID, CustomDimensionIdSetting> mSettingMap; // save in more dimension player
public:
    static constexpr AutomaticID<Dimension, int> fakeDim = 0; // Overworld, Make the client think of the dimension
    static constexpr AutomaticID<Dimension, int> temporaryDimId = 1; // Dimensions of transit

    static FakeDimensionId& getInstance();

    FakeDimensionId();
    ~FakeDimensionId();
    static void changePacketDimension(Packet& packet);
    void        setNeedRemove(mce::UUID uuid, bool needRemove);
    bool        isNeedRemove(mce::UUID uuid);
    void        onPlayerGoCustomDimension(mce::UUID uuid);
    void        onPlayerLeftCustomDimension(mce::UUID uuid, bool isRespawn);
};
} // namespace more_dimensions
