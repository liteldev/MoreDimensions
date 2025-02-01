
#include "SimpleCustomDimension.h"

#include "more_dimensions/MoreDimenison.h"

#include "magic_enum.hpp"

#include "ll/api/memory/Memory.h"
#include "ll/api/service/Bedrock.h"

#include "mc/common/Brightness.h"
#include "mc/common/BrightnessPair.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/DimensionConversionData.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/LevelSeed64.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"
#include "mc/world/level/biome/registry/VanillaBiomeNames.h"
#include "mc/world/level/biome/source/BiomeSource.h"
#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/block/BlockVolume.h"
#include "mc/world/level/chunk/vanilla_level_chunk_upgrade/VanillaLevelChunkUpgrade.h"
#include "mc/world/level/dimension/DimensionHeightRange.h"
#include "mc/world/level/dimension/NetherBrightnessRamp.h"
#include "mc/world/level/dimension/OverworldBrightnessRamp.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"
#include "mc/world/level/levelgen/structure/EndCityFeature.h"
#include "mc/world/level/levelgen/structure/StructureFeatureRegistry.h"
#include "mc/world/level/levelgen/structure/registry//StructureSetRegistry.h"
#include "mc/world/level/levelgen/synth/PerlinNoise.h"
#include "mc/world/level/levelgen/synth/PerlinSimplexNoise.h"
#include "mc/world/level/levelgen/synth/SimplexNoise.h"
#include "mc/world/level/levelgen/v1/NetherGenerator.h"
#include "mc/world/level/levelgen/v1/OverworldGeneratorMultinoise.h"
#include "mc/world/level/levelgen/v1/TheEndGenerator.h"
#include "mc/world/level/levelgen/v1/VoidGenerator.h"
#include "mc/world/level/levelgen/v2/ChunkGeneratorStructureState.h"
#include "mc/world/level/storage/LevelData.h"


namespace more_dimensions {


namespace {
using namespace ll::memory_literals;
static auto* overworldAddress =
    "`anonymous namespace'::unity_5c986e6b9d6571cc96912b0bfa0329e2::addStructureFeatures"_symp;
static auto* netherAddress = "`anonymous namespace'::unity_3da1d4c9fa90b4b1becbca96840255a5::addStructureFeatures"_symp;

void overworldAddStructureFeatures(
    StructureFeatureRegistry& registry,
    uint                      seed,
    bool                      isLegacy,
    BaseGameVersion const&    baseGameVersion
) {
    ll::memory::addressCall<void*, StructureFeatureRegistry&, uint, bool, BaseGameVersion const&>(
        overworldAddress,
        registry,
        seed,
        isLegacy,
        baseGameVersion
    );
};

void netherAddStructureFeatures(
    StructureFeatureRegistry& registry,
    uint                      seed,
    BaseGameVersion const&    baseGameVersion,
    Experiments const&        experiments
) {
    ll::memory::addressCall<void*, StructureFeatureRegistry&, uint, BaseGameVersion const&, Experiments const&>(
        netherAddress,
        registry,
        seed,
        baseGameVersion,
        experiments
    );
};
} // namespace

// static ll::Logger loggerMoreDim("SimpleCustomDim");
auto& loggerMoreDim = MoreDimenison::getInstance().getSelf().getLogger();

SimpleCustomDimension::SimpleCustomDimension(std::string const& name, DimensionFactoryInfo const& info)
: Dimension(info.level, info.dimId, {-64, 320}, info.scheduler, name) {
    loggerMoreDim.debug("{} dimension name:{}", __FUNCTION__, name);
    mDefaultBrightness->sky = Brightness::MAX();
    generatorType           = *magic_enum::enum_cast<GeneratorType>((std::string_view)info.data["generatorType"]);
    seed                    = info.data["seed"];
    switch (generatorType) {
    case GeneratorType::TheEnd: {
        mSeaLevel                = 63;
        mHasWeather              = false;
        mDimensionBrightnessRamp = std::make_unique<OverworldBrightnessRamp>();
    }
    case GeneratorType::Nether: {
        mSeaLevel                = 32;
        mHasWeather              = false;
        mDimensionBrightnessRamp = std::make_unique<NetherBrightnessRamp>();
    }
    default:
        mSeaLevel                = 63;
        mHasWeather              = true;
        mDimensionBrightnessRamp = std::make_unique<OverworldBrightnessRamp>();
    }
    mDimensionBrightnessRamp->buildBrightnessRamp();
}

CompoundTag SimpleCustomDimension::generateNewData(uint seed, GeneratorType generatorType) {
    CompoundTag result;
    result["seed"]          = seed;
    result["generatorType"] = magic_enum::enum_name(generatorType);
    return result;
}

void SimpleCustomDimension::init(br::worldgen::StructureSetRegistry const& structureSetRegistry) {
    loggerMoreDim.debug(__FUNCTION__);
    setSkylight(false);
    Dimension::init(structureSetRegistry);
}

std::unique_ptr<WorldGenerator>
SimpleCustomDimension::createGenerator(br::worldgen::StructureSetRegistry const& structureSetRegistry) {
    loggerMoreDim.debug(__FUNCTION__);
    auto& level     = getLevel();
    auto& levelData = level.getLevelData();
    auto  biome     = level.getBiomeRegistry().lookupByName(levelData.getBiomeOverride());

    std::unique_ptr<WorldGenerator> worldGenerator;

    switch (generatorType) {
    case GeneratorType::Overworld: {
        worldGenerator = std::make_unique<OverworldGeneratorMultinoise>(*this, LevelSeed64{seed}, biome);
        worldGenerator->getStructureFeatureRegistry().mGeneratorState =
            br::worldgen::ChunkGeneratorStructureState::createNormal(
                seed,
                worldGenerator->getBiomeSource(),
                structureSetRegistry
            );
        overworldAddStructureFeatures(
            worldGenerator->getStructureFeatureRegistry(),
            seed,
            false,
            levelData.getBaseGameVersion()
        );
        break;
    }
    case GeneratorType::Nether: {
        worldGenerator = std::make_unique<NetherGenerator>(*this, seed, biome);
        worldGenerator->getStructureFeatureRegistry().mGeneratorState =
            br::worldgen::ChunkGeneratorStructureState::createNormal(
                seed,
                worldGenerator->getBiomeSource(),
                structureSetRegistry
            );
        netherAddStructureFeatures(
            worldGenerator->getStructureFeatureRegistry(),
            seed,
            levelData.getBaseGameVersion(),
            levelData.getExperiments()
        );
        break;
    }
    case GeneratorType::TheEnd: {
        worldGenerator = std::make_unique<TheEndGenerator>(*this, seed, biome);
        worldGenerator->getStructureFeatureRegistry().mGeneratorState =
            br::worldgen::ChunkGeneratorStructureState::createNormal(
                seed,
                worldGenerator->getBiomeSource(),
                structureSetRegistry
            );
        worldGenerator->getStructureFeatureRegistry().mStructureFeatures->emplace_back(
            std::make_unique<EndCityFeature>(*this, seed)
        );
        break;
    }
    case GeneratorType::Flat: {
        worldGenerator = std::make_unique<FlatWorldGenerator>(*this, seed, levelData.getFlatWorldGeneratorOptions());
        worldGenerator->getStructureFeatureRegistry().mGeneratorState =
            br::worldgen::ChunkGeneratorStructureState::createFlat(seed, worldGenerator->getBiomeSource(), {});
        break;
    }
    default: {
        auto generator    = std::make_unique<VoidGenerator>(*this);
        generator->mBiome = level.getBiomeRegistry().lookupByHash(VanillaBiomeNames::Ocean());
        worldGenerator    = std::move(generator);
        worldGenerator->getStructureFeatureRegistry().mGeneratorState =
            br::worldgen::ChunkGeneratorStructureState::createVoid(seed);
    }
    }
    //    worldGenerator->init();
    return std::move(worldGenerator);
}

void SimpleCustomDimension::upgradeLevelChunk(ChunkSource& cs, LevelChunk& lc, LevelChunk& generatedChunk) {
    loggerMoreDim.debug(__FUNCTION__);
    auto blockSource = BlockSource(getLevel(), *this, cs, false, true, false);
    VanillaLevelChunkUpgrade::_upgradeLevelChunkViaMetaData(lc, generatedChunk, blockSource);
    VanillaLevelChunkUpgrade::_upgradeLevelChunkLegacy(lc, blockSource);
}

void SimpleCustomDimension::fixWallChunk(ChunkSource& cs, LevelChunk& lc) {
    loggerMoreDim.debug(__FUNCTION__);
    auto blockSource = BlockSource(getLevel(), *this, cs, false, true, false);
    VanillaLevelChunkUpgrade::fixWallChunk(lc, blockSource);
}

bool SimpleCustomDimension::levelChunkNeedsUpgrade(LevelChunk const& lc) const {
    loggerMoreDim.debug(__FUNCTION__);
    return VanillaLevelChunkUpgrade::levelChunkNeedsUpgrade(lc);
}
void SimpleCustomDimension::_upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers) {
    loggerMoreDim.debug(__FUNCTION__);
    auto isTemplate = getLevel().getLevelData().isFromWorldTemplate();
    return VanillaLevelChunkUpgrade::upgradeOldLimboEntity(tag, vers, isTemplate);
}

Vec3 SimpleCustomDimension::translatePosAcrossDimension(Vec3 const& fromPos, DimensionType fromId) const {
    loggerMoreDim.debug(__FUNCTION__);
    Vec3 topos;
    VanillaDimensions::convertPointBetweenDimensions(
        fromPos,
        topos,
        fromId,
        mId,
        getLevel().getDimensionConversionData()
    );
    constexpr auto clampVal = 32000000.0f - 128.0f;

    topos.x = std::clamp(topos.x, -clampVal, clampVal);
    topos.z = std::clamp(topos.z, -clampVal, clampVal);

    return topos;
}

short SimpleCustomDimension::getCloudHeight() const { return 192; }

bool SimpleCustomDimension::hasPrecipitationFog() const { return true; }

std::unique_ptr<ChunkSource>
SimpleCustomDimension::_wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion /*ver*/) {
    loggerMoreDim.debug(__FUNCTION__);
    return cs;
}

mce::Color SimpleCustomDimension::getBrightnessDependentFogColor(mce::Color const& color, float brightness) const {
    loggerMoreDim.debug(__FUNCTION__);
    float temp   = (brightness * 0.94f) + 0.06f;
    float temp2  = (brightness * 0.91f) + 0.09f;
    auto  result = color;
    result.r     = color.r * temp;
    result.g     = color.g * temp;
    result.b     = color.b * temp2;
    return result;
};

} // namespace more_dimensions
