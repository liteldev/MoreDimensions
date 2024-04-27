#pragma once

#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/NetherBrightnessRamp.h"
#include "mc/world/level/dimension/OverworldBrightnessRamp.h"
#include "mc/world/level/levelgen/structure/StructureFeatureRegistry.h"
#include "more_dimensions/api/dimension/CustomDimensionManager.h"
#include "more_dimensions/core/Macros.h"

class BaseGameVersion;
class Experiments;
class ChunkSource;
class LevelChunk;

namespace unity_5c986e6b9d6571cc96912b0bfa0329e2 {
MCAPI void addStructureFeatures(
    StructureFeatureRegistry& registry,
    uint                      seed,
    bool                      isLegacy,
    BaseGameVersion const&    baseGameVersion
);
}
namespace unity_3da1d4c9fa90b4b1becbca96840255a5 {
MCAPI void addStructureFeatures(
    StructureFeatureRegistry& registry,
    uint                      seed,
    BaseGameVersion const&    baseGameVersion,
    Experiments const&        experiments
);
}

namespace more_dimensions {

class SimpleCustomDimension : public Dimension {
    uint          seed;
    GeneratorType generatorType;

public:
    MORE_DIMENSIONS_API SimpleCustomDimension(std::string const& name, DimensionFactoryInfo const& info);

    MORE_DIMENSIONS_API static CompoundTag generateNewData(uint seed = 123, GeneratorType generatorType = GeneratorType::Overworld);

    MORE_DIMENSIONS_API void init(br::worldgen::StructureSetRegistry const&) override;

    MORE_DIMENSIONS_API std::unique_ptr<WorldGenerator> createGenerator(br::worldgen::StructureSetRegistry const&) override;

    MORE_DIMENSIONS_API void upgradeLevelChunk(ChunkSource& chunkSource, LevelChunk& oldLc, LevelChunk& newLc) override;

    MORE_DIMENSIONS_API void fixWallChunk(ChunkSource& cs, LevelChunk& lc) override;

    MORE_DIMENSIONS_API bool levelChunkNeedsUpgrade(LevelChunk const& lc) const override;

    MORE_DIMENSIONS_API void _upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers) override;

    MORE_DIMENSIONS_API Vec3 translatePosAcrossDimension(Vec3 const& pos, DimensionType did) const override;

    MORE_DIMENSIONS_API std::unique_ptr<ChunkSource>
    _wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion ver) override;

    MORE_DIMENSIONS_API mce::Color getBrightnessDependentFogColor(mce::Color const& color, float brightness) const override;

    MORE_DIMENSIONS_API short getCloudHeight() const override;

    MORE_DIMENSIONS_API bool hasPrecipitationFog() const override;
};
} // namespace more_dimensions
