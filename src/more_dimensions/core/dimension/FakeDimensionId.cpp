#include "FakeDimensionId.h"

#include "more_dimensions//api/dimension/CustomDimensionManager.h"
#include "more_dimensions/MoreDimenison.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "mc/common/ActorUniqueID.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/entity/components/MovementPackets.h"
#include "mc/entity/components/ServerPlayerMovementComponent.h"
#include "mc/network//LoopbackPacketSender.h"
#include "mc/network/NetworkBlockPosition.h"
#include "mc/network/NetworkIdentifierWithSubId.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/packet/AddVolumeEntityPacket.h"
#include "mc/network/packet/ChangeDimensionPacket.h"
#include "mc/network/packet/InteractPacket.h"
#include "mc/network/packet/InventoryTransactionPacket.h"
#include "mc/network/packet/LevelChunkPacket.h"
#include "mc/network/packet/PlayerActionPacket.h"
#include "mc/network/packet/PlayerAuthInputPacket.h"
#include "mc/network/packet/RemoveVolumeEntityPacket.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/network/packet/StartGamePacket.h"
#include "mc/network/packet/SubChunkPacket.h"
#include "mc/network/packet/SubChunkRequestPacket.h"
#include "mc/network/packet/UpdateBlockPacket.h"
#include "mc/server/ServerPlayer.h"
#include "mc/util/MolangVariableMap.h"
#include "mc/util/VarIntDataOutput.h"
#include "mc/world/actor/ActorDataIDs.h"
#include "mc/world/actor/SynchedActorDataEntityWrapper.h"
#include "mc/world/item/registry/ItemRegistryRef.h"
#include "mc/world/level/ChangeDimensionRequest.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/SpawnSettings.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/levelSettings.h"


// ChangeDimensionPacket.java
// ClientboundMapItemDataPacket.java
// RemoveVolumeEntityPacket.java
// StartGamePacket.java
// AddVolumeEntityPacket.java
// SpawnParticleEffectPacket.java
// SubChunkPacket.java
// SubChunkRequestPacket.java

namespace more_dimensions {

// static ll::Logger logger("FakeDimensionId");
auto& logger = MoreDimenison::getInstance().getSelf().getLogger();

static void sendEmptyChunk(const NetworkIdentifier& netId, int chunkX, int chunkZ, bool forceUpdate) {
    std::array<uchar, 4096> biome{};
    LevelChunkPacket        levelChunkPacket;
    BinaryStream            binaryStream{levelChunkPacket.mSerializedChunk, false};
    VarIntDataOutput        varIntDataOutput(binaryStream);

    varIntDataOutput.writeBytes(&biome, 4096); // write void biome
    for (int i = 1; i < 8; i++) {
        varIntDataOutput.writeByte(255ui8);
    }
    varIntDataOutput.mStream.writeUnsignedChar(0); // write border blocks

    levelChunkPacket.mPos->x         = chunkX;
    levelChunkPacket.mPos->z         = chunkZ;
    levelChunkPacket.mDimensionId    = FakeDimensionId::temporaryDimId;
    levelChunkPacket.mCacheEnabled   = false;
    levelChunkPacket.mSubChunksCount = 0;

    ll::service::getLevel()->getPacketSender()->sendToClient(netId, levelChunkPacket, SubClientId::PrimaryClient);

    if (forceUpdate) {
        NetworkBlockPosition pos{
            BlockPos{chunkX << 4, 80, chunkZ << 4}
        };
        UpdateBlockPacket blockPacket;
        blockPacket.mPos         = pos;
        blockPacket.mLayer       = 0;
        blockPacket.mUpdateFlags = 1;
        ll::service::getLevel()->getPacketSender()->sendToClient(netId, blockPacket, SubClientId::PrimaryClient);
    }
}

static void sendEmptyChunks(const NetworkIdentifier& netId, const Vec3& position, int radius, bool forceUpdate) {
    int chunkX = (int)(position.x) >> 4;
    int chunkZ = (int)(position.z) >> 4;
    for (int x = -radius; x <= radius; x++) {
        for (int z = -radius; z <= radius; z++) {
            sendEmptyChunk(netId, chunkX + x, chunkZ + z, forceUpdate);
        }
    }
}

static void fakeChangeDimension(
    const NetworkIdentifier& netId,
    ActorRuntimeID           runtimeId,
    DimensionType            fakeDimId,
    const Vec3&              pos
) {
    ChangeDimensionPacket changeDimensionPacket{fakeDimId, pos, true, {0}};
    ll::service::getLevel()->getPacketSender()->sendToClient(netId, changeDimensionPacket, SubClientId::PrimaryClient);
    PlayerActionPacket playerActionPacket{PlayerActionType::ChangeDimensionAck, runtimeId};
    ll::service::getLevel()->getPacketSender()->sendToClient(netId, playerActionPacket, SubClientId::PrimaryClient);
    sendEmptyChunks(netId, pos, 3, true);
}

namespace CustomDimensionHookList {

namespace sendpackethook {

LL_TYPE_INSTANCE_HOOK(
    LoopbackPacketSendersendToClientHandler1,
    HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClient,
    void,
    NetworkIdentifier const& netId,
    Packet const&            packet,
    ::SubClientId            subId
) {
    auto player = ll::service::getServerNetworkHandler()->_getServerPlayer(netId, subId);
    if (player && player->getDimensionId() >= 3 && packet.getId() != MinecraftPacketIds::ChangeDimension
        && packet.getId() != MinecraftPacketIds::PlayerAction
        && packet.getId() != MinecraftPacketIds::SpawnParticleEffect) {
        FakeDimensionId::changePacketDimension(const_cast<Packet&>(packet));
    }
    if (player && player->getDimensionId() >= 3 && packet.getId() == MinecraftPacketIds::FullChunkData) {
        auto& modifPacket = (LevelChunkPacket&)packet;
        if (modifPacket.mDimensionId->id >= 3) {
            modifPacket.mDimensionId = 0;
        }
        if (modifPacket.mDimensionId->id == VanillaDimensions::Undefined()) {
            modifPacket.mDimensionId->id = 3;
        }
        if (modifPacket.mClientRequestSubChunkLimit <= 8) {
            modifPacket.mClientRequestSubChunkLimit = 11;
        }
    }
    return origin(netId, packet, subId);
};

LL_TYPE_INSTANCE_HOOK(
    LoopbackPacketSendersendToClientHandler2,
    HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClient,
    void,
    UserEntityIdentifierComponent const* comp,
    Packet const&                        packet
) {
    auto player = ll::service::getServerNetworkHandler()->_getServerPlayer(comp->mNetworkId, comp->mClientSubId);
    if (!player) return origin(comp, packet);
    auto uuid = player->getUuid();
    if (player && player->getDimensionId() >= 3 && packet.getId() != MinecraftPacketIds::ChangeDimension
        && packet.getId() != MinecraftPacketIds::PlayerAction
        && packet.getId() != MinecraftPacketIds::SpawnParticleEffect) {
        FakeDimensionId::changePacketDimension(const_cast<Packet&>(packet));
    }

    // remove send changeDimensionPacket to client when player die
    if (FakeDimensionId::getInstance().isNeedRemove(uuid) && packet.getId() == MinecraftPacketIds::ChangeDimension) {
        return;
    }
    // remove level event packet, event id is 9801, LevelEvent is SleepingPlayers
    if (packet.getId() == MinecraftPacketIds::LevelEventGeneric && FakeDimensionId::getInstance().isNeedRemove(uuid)) {
        return;
    }
    // remove send changedimension success action packet to client when player die
    if (FakeDimensionId::getInstance().isNeedRemove(uuid) && packet.getId() == MinecraftPacketIds::PlayerAction) {
        auto& actionPacket = (PlayerActionPacket&)packet;
        if (actionPacket.mAction == PlayerActionType::ChangeDimensionAck) {
            return;
        }
    }
    return origin(comp, packet);
};

LL_TYPE_INSTANCE_HOOK(
    LoopbackPacketSendersendToClientsHandler,
    HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClients,
    void,
    std::vector<NetworkIdentifierWithSubId> const& subIds,
    Packet const&                                  packet
) {
    if (packet.getId() == MinecraftPacketIds::RemoveVolumeEntityPacket
        || packet.getId() == MinecraftPacketIds::AddVolumeEntityPacket) {
        for (auto& subId : subIds) {
            auto player =
                ll::service::getServerNetworkHandler()->_getServerPlayer(subId.id, SubClientId::PrimaryClient);
            if (player && player->getDimensionId() >= 3) {
                FakeDimensionId::changePacketDimension(const_cast<Packet&>(packet));
            }
            packet.sendToClient(subId);
        }
        return;
    }
    return origin(subIds, packet);
};

// StartGamePacket
LL_TYPE_INSTANCE_HOOK(
    StartGamePacketHandler,
    HookPriority::Normal,
    StartGamePacket,
    &StartGamePacket::$ctor,
    void*,
    ItemRegistryRef               itemRegistryRef,
    LevelSettings const&          levelSettings,
    ActorUniqueID                 uniqueId,
    ActorRuntimeID                runtimeId,
    ::GameType                    gameType,
    bool                          unk,
    Vec3 const&                   pos,
    Vec2 const&                   ros,
    std::string const&            levelId,
    std::string const&            levelName,
    ContentIdentity const&        contentIdentity,
    std::string const&            unk1,
    BlockDefinitionGroup const&   blockDefinitionGroup,
    bool                          unk2,
    CompoundTag                   compoundTag,
    PlayerMovementSettings const& moveSetting,
    std::string const&            unk3,
    mce::UUID const&              uuid,
    uint64                        unk4,
    int                           unk5,
    uint64                        unk6
) {
    if (levelSettings.getSpawnSettings().dimension->id >= 3) {
        SpawnSettings spawnSettings(levelSettings.getSpawnSettings());
        spawnSettings.dimension = FakeDimensionId::fakeDim;
        const_cast<LevelSettings&>(levelSettings).setSpawnSettings(spawnSettings);
    }
    return origin(
        itemRegistryRef,
        levelSettings,
        uniqueId,
        runtimeId,
        gameType,
        unk,
        pos,
        ros,
        levelId,
        levelName,
        contentIdentity,
        unk1,
        blockDefinitionGroup,
        unk2,
        std::move(compoundTag),
        moveSetting,
        unk3,
        uuid,
        unk4,
        unk5,
        unk6
    );
}

// ChangeDimensionPacket
LL_TYPE_INSTANCE_HOOK(
    ChangeDimensionPacketHandler,
    HookPriority::Normal,
    ChangeDimensionPacket,
    &ChangeDimensionPacket::$ctor,
    void*,
    DimensionType                  dimId,
    Vec3                           pos,
    bool                           respawn,
    NewType<::std::optional<uint>> loadingScreenId
) {
    if (dimId > 2) {
        dimId = FakeDimensionId::fakeDim;
    }
    return origin(dimId, pos, respawn, loadingScreenId);
}

// SubChunkPacket and SubChunkRequestPacket
LL_TYPE_INSTANCE_HOOK(
    SubChunkPacketHandler,
    HookPriority::Highest,
    ServerNetworkHandler,
    &ServerNetworkHandler::_buildSubChunkPacketData,
    void,
    NetworkIdentifier const&     networkIdentifier,
    ServerPlayer const*          serverPlayer,
    SubChunkRequestPacket const& subChunkRequestPacket,
    SubChunkPacket&              subChunkPacket,
    uint                         count,
    bool                         isCacheEnabled
) {
    auto inDim = serverPlayer->getDimensionId();
    if (inDim >= 3) {
        const_cast<SubChunkRequestPacket&>(subChunkRequestPacket).mDimensionType = inDim;
        // dimensionType value of subChunkPacket is not read in this function, so it is changed together.
        subChunkPacket.mDimensionType = FakeDimensionId::fakeDim;
    };
    return origin(networkIdentifier, serverPlayer, subChunkRequestPacket, subChunkPacket, count, isCacheEnabled);
}

// SpawnParticleEffectPacket
LL_TYPE_INSTANCE_HOOK(
    SpawnParticleEffectPacketHandler,
    HookPriority::Normal,
    SpawnParticleEffectPacket,
    &SpawnParticleEffectPacket::$ctor,
    void*,
    Vec3 const&                      pos,
    std::string const&               particle_name,
    uchar                            dimId,
    std::optional<MolangVariableMap> molang
) {
    if (dimId >= 3) {
        dimId = FakeDimensionId::fakeDim;
    }
    return origin(pos, particle_name, dimId, std::move(molang));
}

} // namespace sendpackethook

LL_TYPE_INSTANCE_HOOK(
    PlayerdieHandler,
    HookPriority::Normal,
    Player,
    &Player::$die,
    void,
    ActorDamageSource const& actorDamageSource
) {
    if (getDimensionId() >= 3) {
        logger.debug("Remove set true");
        FakeDimensionId::getInstance().setNeedRemove(getUuid(), true);
    }
    return origin(actorDamageSource);
}

namespace receivepackethook {

// when player in overworld and custom dimension will need
LL_TYPE_INSTANCE_HOOK(
    ServerNetworkHandlerPlayerActionPacketHandler,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&  netId,
    PlayerActionPacket const& packet
) {
    auto& handler         = ll::memory::dAccess<ServerNetworkHandler>(this, -16);
    auto  player          = handler._getServerPlayer(netId, packet.mClientSubId);
    auto  uuid            = player->getUuid();
    auto& fakeDimensionId = FakeDimensionId::getInstance();
    if (packet.mAction == PlayerActionType::Respawn) {
        // when player go overworld with die
        if (!fakeDimensionId.isNeedRemove(uuid)) {
            return origin(netId, packet);
        }
        fakeDimensionId.setNeedRemove(uuid, false);
        auto moveComp = player->getEntityContext().tryGetComponent<ServerPlayerMovementComponent>();
        if (moveComp) {
            moveComp->mServerHasMovementAuthority = false;
        }
        fakeDimensionId.onPlayerLeftCustomDimension(uuid, true);
        // flash player bounding box
        player->getEntityData().markDirty(fmt::underlying(ActorDataIDs::CollisionBox));
    }
    return origin(netId, packet);
};
} // namespace receivepackethook

LL_TYPE_INSTANCE_HOOK(
    LevelrequestPlayerChangeDimensionHandler,
    HookPriority::Normal,
    Level,
    &Level::$requestPlayerChangeDimension,
    void,
    Player&                  player,
    ChangeDimensionRequest&& changeRequest
) {
    auto inId = player.getDimensionId();
    if (changeRequest.mToDimensionId->id == 1 || changeRequest.mToDimensionId->id == 2 || inId == 1 || inId == 2
        || player.isDead()) {
        return origin(player, std::move(changeRequest));
    }
    fakeChangeDimension(
        player.getNetworkIdentifier(),
        player.getRuntimeID(),
        more_dimensions::FakeDimensionId::temporaryDimId,
        player.getPosition()
    );
    return origin(player, std::move(changeRequest));
}

using HookReg = ll::memory::HookRegistrar<
    sendpackethook::LoopbackPacketSendersendToClientHandler1,
    sendpackethook::LoopbackPacketSendersendToClientHandler2,
    sendpackethook::LoopbackPacketSendersendToClientsHandler,
    sendpackethook::SubChunkPacketHandler,
    sendpackethook::SpawnParticleEffectPacketHandler,
    sendpackethook::StartGamePacketHandler,
    sendpackethook::ChangeDimensionPacketHandler,
    PlayerdieHandler,
    LevelrequestPlayerChangeDimensionHandler,
    receivepackethook::ServerNetworkHandlerPlayerActionPacketHandler>;

} // namespace CustomDimensionHookList

FakeDimensionId::FakeDimensionId() { CustomDimensionHookList::HookReg::hook(); }

FakeDimensionId::~FakeDimensionId() { CustomDimensionHookList::HookReg::unhook(); }

FakeDimensionId& FakeDimensionId::getInstance() {
    static FakeDimensionId ins{};
    return ins;
}

void FakeDimensionId::changePacketDimension(Packet& packet) {
    auto packId = packet.getId();
    switch (packId) {
    case MinecraftPacketIds::RemoveVolumeEntityPacket: {
        auto& tempP          = (RemoveVolumeEntityPacket&)packet;
        tempP.mDimensionType = fakeDim;
        logger.debug("MinecraftPacketIds::RemoveVolumeEntityPacket: dimId change to {}", fakeDim);
    }
    case MinecraftPacketIds::AddVolumeEntityPacket: {
        auto& tempP          = (AddVolumeEntityPacket&)packet;
        tempP.mDimensionType = fakeDim;
        logger.debug("MinecraftPacketIds::AddVolumeEntityPacket: dimId change to {}", fakeDim);
    }
    default:
        return;
    }
}

void FakeDimensionId::setNeedRemove(mce::UUID uuid, bool needRemove) {
    if (mSettingMap.count(uuid)) {
        mSettingMap.at(uuid).needRemovePacket = needRemove;
    } else {
        mSettingMap.emplace(uuid, CustomDimensionIdSetting{needRemove});
    }
}

bool FakeDimensionId::isNeedRemove(mce::UUID uuid) {
    if (mSettingMap.count(uuid)) {
        return mSettingMap.at(uuid).needRemovePacket;
    };
    return false;
}

void FakeDimensionId::onPlayerGoCustomDimension(mce::UUID uuid) {
    if (!mSettingMap.count(uuid)) {
        std::lock_guard lockGuard{mMapMutex};
        mSettingMap.emplace(uuid, CustomDimensionIdSetting{false});
    }
}

void FakeDimensionId::onPlayerLeftCustomDimension(mce::UUID uuid, bool isRespawn) {
    std::lock_guard lockGuard{mMapMutex};
    if (mSettingMap.count(uuid)) {
        if (isRespawn) {
            mSettingMap.at(uuid).needRemovePacket = false;
        } else {
            mSettingMap.erase(uuid);
        }
    }
}

} // namespace more_dimensions
