
#include "CustomDimensionManager.h"

#include "ll/api/Logger.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/utils/Base64Utils.h"
#include "ll/api/utils/StringUtils.h"
#include "mc/math/Vec3.h"
#include "mc/server/common/PropertiesSettings.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/storage/LevelStorage.h"
#include "more_dimensions/core/dimension/CustomDimensionConfig.h"
#include "more_dimensions/core/dimension/FakeDimensionId.h"

class Scheduler;

namespace more_dimensions {

static ll::Logger loggerMoreDimMag("CustomDimensionManager");

namespace CustomDimensionHookList {
LL_TYPE_STATIC_HOOK(
    VanillaDimensionsConverHook,
    HookPriority::Normal,
    VanillaDimensions,
    VanillaDimensions::convertPointBetweenDimensions,
    bool,
    Vec3 const&                    oldPos,
    Vec3&                          toPos,
    DimensionType                  oldDim,
    DimensionType                  toDim,
    DimensionConversionData const& data
) {
    if (oldDim <= 2 && toDim <= 2) return origin(oldPos, toPos, oldDim, toDim, data);
    toPos = oldPos;
    return true;
};

LL_TYPE_STATIC_HOOK(
    VanillaDimensionsFromSerializedIntHook,
    HookPriority::Normal,
    VanillaDimensions,
    VanillaDimensions::fromSerializedInt,
    Bedrock::Result<DimensionType>,
    Bedrock::Result<int>&& dim
) {
    if (!dim || *dim <= 2) {
        return origin(std::move(dim));
    }
    if (!VanillaDimensions::DimensionMap.mLeft.contains(*dim)) {
        return VanillaDimensions::Undefined;
    }
    return *dim;
};

LL_TYPE_STATIC_HOOK(
    VanillaDimensionsFromSerializedIntHookI,
    HookPriority::Normal,
    VanillaDimensions,
    VanillaDimensions::fromSerializedInt,
    DimensionType,
    int dimId
) {
    if (dimId <= 2) return origin(dimId);
    if (!VanillaDimensions::DimensionMap.mLeft.contains(dimId)) {
        return VanillaDimensions::Undefined;
    }
    return {dimId};
}

LL_TYPE_STATIC_HOOK(
    VanillaDimensionsToSerializedIntHook,
    HookPriority::Normal,
    VanillaDimensions,
    VanillaDimensions::toSerializedInt,
    int,
    DimensionType const& dim
) {
    if (dim <= 2) return origin(dim);
    return dim.id;
}

LL_TYPE_STATIC_HOOK(
    VanillaDimensionsToStringHook,
    HookPriority::Normal,
    VanillaDimensions,
    VanillaDimensions::toString,
    std::string const,
    DimensionType const& dim
) {
    if (dim <= 2) return origin(dim);
    return VanillaDimensions::DimensionMap.mLeft.at(dim);
}

// 当玩家加入服务器时，生成时的维度不存在，并且维度id不是Undefined时，把玩家放到主世界
LL_TYPE_INSTANCE_HOOK(
    LevelStorageloadServerPlayerDataHook,
    HookPriority::Normal,
    LevelStorage,
    &LevelStorage::loadServerPlayerData,
    std::unique_ptr<class CompoundTag>,
    Player const& client,
    bool          isXboxLive
) {

    auto result = origin(client, isXboxLive);
    if (!result) return result;
    auto spawnDimension = result->at("DimensionId");
    if (!VanillaDimensions::DimensionMap.mLeft.contains(AutomaticID<Dimension, int>(spawnDimension))) {
        result->at("Pos")[1] = FloatTag{0x7fff};
    }
    return result;
}

// 由于这个的调用在维度注册之前，所以使用AUTO
LL_AUTO_TYPE_INSTANCE_HOOK(
    PropertiesSettingsisClientSideGenEnabledHook,
    HookPriority::Normal,
    PropertiesSettings,
    &PropertiesSettings::isClientSideGenEnabled,
    bool
) {
    return false;
}

// registry dimensoin when in ll, must reload Dimension::getWeakRef
LL_TYPE_INSTANCE_HOOK(DimensionGetWeakRefHook, HookPriority::Normal, Dimension, &Dimension::getWeakRef, WeakRef<Dimension>) {
    if (getDimensionId() > 2 && getDimensionId() != VanillaDimensions::Undefined.id) return weak_from_this();
    return origin();
};

using HookReg = ll::memory::HookRegistrar<
    VanillaDimensionsConverHook,
    VanillaDimensionsFromSerializedIntHook,
    VanillaDimensionsFromSerializedIntHookI,
    VanillaDimensionsToSerializedIntHook,
    VanillaDimensionsToStringHook,
    LevelStorageloadServerPlayerDataHook,
    PropertiesSettingsisClientSideGenEnabledHook,
    DimensionGetWeakRefHook>;

} // namespace CustomDimensionHookList

struct CustomDimensionManager::Impl {
    std::atomic<int> mNewDimensionId{3};
    std::mutex       mMapMutex;

    struct DimensionInfo {
        DimensionType id;
        CompoundTag   nbt;
    };
    std::unordered_map<std::string, DimensionInfo> customDimensionMap;
    std::unordered_set<std::string>                registeredDimension;
};

CustomDimensionManager::CustomDimensionManager() : impl(std::make_unique<Impl>()) {
    std::lock_guard lock{impl->mMapMutex};
    CustomDimensionConfig::setDimensionConfigPath();
    CustomDimensionConfig::loadConfigFile();
    if (!CustomDimensionConfig::dimConfig.dimensionList.empty()) {
        for (auto& [name, info] : CustomDimensionConfig::dimConfig.dimensionList) {
            impl->customDimensionMap.emplace(
                name,
                Impl::DimensionInfo{
                    info.dimId,
                    *CompoundTag::fromBinaryNbt(ll::string_utils::decompress(ll::base64_utils::decode(info.base64Nbt)))
                }
            );
        }
        impl->mNewDimensionId += static_cast<int>(impl->customDimensionMap.size());
    }
    FakeDimensionId::getInstance();
    CustomDimensionHookList::HookReg::hook();
};

CustomDimensionManager::~CustomDimensionManager() { CustomDimensionHookList::HookReg::unhook(); }

CustomDimensionManager& CustomDimensionManager::getInstance() {
    static CustomDimensionManager instance{};
    return instance;
}

DimensionType CustomDimensionManager::getDimensionIdFromName(std::string const& dimName) {
    return VanillaDimensions::fromString(dimName);
}

DimensionType CustomDimensionManager::addDimension(
    std::string const&                  dimName,
    std::function<DimensionFactoryT>    factory,
    std::function<CompoundTag()> const& data
) {
    std::lock_guard     lock{impl->mMapMutex};
    Impl::DimensionInfo info;
    bool                newDim{};
    if (impl->customDimensionMap.contains(dimName)) {
        info = impl->customDimensionMap.at(dimName);
        loggerMoreDimMag.info(
            "The dimension already registry. use old id, name: {}, id: {}, \ndata: {}",
            dimName,
            info.id.id,
            info.nbt.toSnbt()
        );
    } else {
        // Assign new id
        info.id  = impl->mNewDimensionId++;
        info.nbt = data();
        newDim   = true;
        loggerMoreDimMag
            .info("registry new dimension, name: {}, id: {}, \ndata: {}", dimName, info.id.id, info.nbt.toSnbt());
    };

    // registry create dimension function
    if (!ll::service::getLevel()) {
        throw std::runtime_error("Level is nullptr, cannot registry new dimension " + dimName);
    }

    ll::service::getLevel()->getDimensionFactory().registerFactory(
        dimName,
        [dimName, info, factory = std::move(factory)](ILevel& ilevel, Scheduler& scheduler) -> OwnerPtr<Dimension> {
            loggerMoreDimMag.debug("Create dimension, name: {}, id: {}", dimName, info.id.id);
            return factory(DimensionFactoryInfo{ilevel, scheduler, info.nbt, info.id});
        }
    );

    // modify default dimension map
    loggerMoreDimMag.debug("Add new dimension to DimensionMap");
    ll::memory::modify(VanillaDimensions::DimensionMap, [&](auto& dimMap) {
        loggerMoreDimMag.debug("Add new dimension: name->{}, id->{} to DimensionMap", dimName, info.id.id);
        dimMap.insert_or_assign(dimName, info.id);
    });

    // modify default Undefined dimension id
    ll::memory::modify(VanillaDimensions::Undefined, [&](auto& uid) {
        uid.id = impl->mNewDimensionId;
        loggerMoreDimMag.debug("Set VanillaDimensions::Undefined to {}", uid.id);
    });

    // config
    impl->registeredDimension.emplace(dimName);
    if (newDim) {
        impl->customDimensionMap.emplace(dimName, info);
        CustomDimensionConfig::dimConfig.dimensionList.emplace(
            dimName,
            CustomDimensionConfig::Config::Info{
                info.id,
                ll::base64_utils::encode(ll::string_utils::compress(info.nbt.toBinaryNbt()))
            }
        );
        CustomDimensionConfig::saveConfigFile();
    }

    // add to command enum

    ll::command::CommandRegistrar::getInstance().addEnumValues(
        "Dimension",
        {
            {dimName, info.id}
    },
        Bedrock::type_id<CommandRegistry, DimensionType>()
    );

    return info.id;
}
} // namespace more_dimensions
