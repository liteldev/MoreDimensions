#pragma once

#include "mc/world/level/dimension/Dimension.h"                   // 新维度类需要继承的类
#include "more_dimensions/api/dimension/CustomDimensionManager.h" // 引入DimensionFactoryInfo的声明

// 建议是加一个命名空间，避免与其他插件同类名的情况
namespace flat_village_dimension {

class FlatVillageDimension : public Dimension {
public:
    // 建议固定这样写，DimensionFactoryInfo类里面提供了Dimension实例化的基本数据，name就是维度名，多维度是维度名区分不同维度
    FlatVillageDimension(std::string const& name, more_dimensions::DimensionFactoryInfo const& info);

    // 多维度需要的一个方法，参数是你需要处理的数据，比如种子，这里不没有这样的需要，后面说原因
    static CompoundTag generateNewData();

    // 以下六个是必须重写的函数
    // 维度地形的生成器，是本教程主要更改的地方
    std::unique_ptr<WorldGenerator> createGenerator(br::worldgen::StructureSetRegistry const&) override;

    // 与本教程无关，按照本教程写的就行，无需留意
    void upgradeLevelChunk(ChunkSource& chunkSource, LevelChunk& oldLc, LevelChunk& newLc) override;

    // 与本教程无关，按照本教程写的就行，无需留意
    void fixWallChunk(ChunkSource& cs, LevelChunk& lc) override;

    // 与本教程无关，按照本教程写的就行，无需留意
    bool levelChunkNeedsUpgrade(LevelChunk const& lc) const override;

    // 与本教程无关，按照本教程写的就行，无需留意
    void _upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers) override;

    // 与本教程无关，按照本教程写的就行，无需留意
    std::unique_ptr<ChunkSource>
    _wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion ver) override;

    // 当你转到这个维度时，坐标怎么转换，比如主世界与地狱的
    Vec3 translatePosAcrossDimension(Vec3 const& pos, DimensionType did) const override;

    // 云高度，默认是y128，但多维度高度范围是在y-64~320，与主世界相同，重写它，放高些
    short getCloudHeight() const override;

    // 非必要。下雨时，可视范围的更改
    bool hasPrecipitationFog() const override;
};

} // namespace flat_village_dimension