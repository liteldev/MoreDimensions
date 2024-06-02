#include "ll/api/event/EventBus.h"
#include "ll/api/event/server/ServerStartedEvent.h"
#include "more_dimensions/api/dimension/CustomDimensionManager.h"
#include "more_dimensions/api/dimension/SimpleCustomDimension.h"
#include "test/generator/flat-gen-village/FlatVillageDimension.h"

static bool reg = [] {
    using namespace ll::event;
    EventBus::getInstance().emplaceListener<ServerStartedEvent>([](ServerStartedEvent&) {
        more_dimensions::CustomDimensionManager::getInstance().addDimension<more_dimensions::SimpleCustomDimension>(
            "testNewDimension"
        );
        more_dimensions::CustomDimensionManager::getInstance()
            .addDimension<more_dimensions::SimpleCustomDimension>("testNewFlatDimension", 345, GeneratorType::Flat);
        more_dimensions::CustomDimensionManager::getInstance()
            .addDimension<more_dimensions::SimpleCustomDimension>("testNewNetherDimension", 345, GeneratorType::Nether);
        more_dimensions::CustomDimensionManager::getInstance()
            .addDimension<more_dimensions::SimpleCustomDimension>("testNewTheEndDimension", 345, GeneratorType::TheEnd);
        more_dimensions::CustomDimensionManager::getInstance()
            .addDimension<more_dimensions::SimpleCustomDimension>("testNewVoidDimension", 345, GeneratorType::Void);
        more_dimensions::CustomDimensionManager::getInstance()
            .addDimension<flat_village_dimension::FlatVillageDimension>("testFlatVillage");
    });
    return true;
}();


// #include "ll/api/memory/Hook.h"
// #include "mc/world/level/dimension/DimensionManager.h"
// #include "mc/world/level/dimension/DimensionFactory.h"
//
// LL_AUTO_TYPE_INSTANCE_HOOK(
//     GetOrCreateDimension,
//     HookPriority::Normal,
//     DimensionManager,
//     "?getOrCreateDimension@DimensionManager@@QEAA?AV?$WeakRef@VDimension@@@@V?$AutomaticID@VDimension@@H@@@Z",
//     WeakRef<Dimension>,
//     DimensionType dim) {
//     std::cout<<"getOrCreateDimension->"<< dim.id <<std::endl;;
//     return origin(dim);
// };
//
// LL_AUTO_TYPE_INSTANCE_HOOK(
//     DimensonFactory123,
//     HookPriority::Normal,
//     DimensionFactory,
//     "?create@DimensionFactory@@UEBA?AV?$OwnerPtr@VDimension@@@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
//     OwnerPtr<Dimension>,
//     std::string const &name) {
//     std::cout<<"Dimension creative->"<<name<<std::endl;;
//     return origin(name);
// };
//
// #include "ll/api/command/CommandHandle.h"
// #include "ll/api/command/CommandRegistrar.h"
// #include "mc/server/commands/CommandOutput.h"
// #include "mc/server/commands/ServerCommands.h"
// #include "ll/api/service/Bedrock.h"
// #include "mc/world/level/Level.h"
// #include "mc/world/level/dimension/DimensionFactory.h"
// #include "mc/world/level/dimension/VanillaDimensions.h"
//
// #include "mc/world/events/ServerInstanceEventCoordinator.h"
//
// struct ParamTest {
//     int      p1;
// };
//
// LL_AUTO_TYPE_INSTANCE_HOOK(
//     registerBuiltinCommands,
//     ll::memory::HookPriority::Normal,
//     ServerInstanceEventCoordinator,
//     &ServerInstanceEventCoordinator::sendServerThreadStarted,
//     void,
//     ::ServerInstance& ins
//) {
//     origin(ins);
//
//     auto&       cmd    = ll::command::CommandRegistrar::getInstance().getOrCreateCommand("t", "test tttttt");
//     static auto lambda = [](CommandOrigin const&, CommandOutput& output, ParamTest const& param) {
//         output.success("p1: {}", param.p1);
//         auto dim = ll::service::getLevel()->getOrCreateDimension(param.p1);
//         if (dim.expired()) {
//             std::cout<<"他宝贝的，销毁了"<<std::endl;
//         } else {
//             auto dim_ptr = dim.lock();
//             std::cout<<"这是正常的:"<<dim_ptr->mName<<std::endl;
//         }
//         auto& dimM = ll::service::getLevel()->getDimensionManager();
//         for (auto& item: dimM.mDimensions) {
//             std::cout<<"Dimension have->" <<item.first.id<<std::endl;
//         };
//         for (auto item: VanillaDimensions::DimensionMap.mLeft) {
//             std::cout<< "Dimension Factory->"<<item.first<<",name:"<<item.second<<std::endl;
//             std::cout<< "Dimension Factory->"<<item.first<<",get
//             name:"<<VanillaDimensions::toString(item.first)<<std::endl;
//         }
//     };
//     cmd.overload<ParamTest>()
//         .required("p1")
//         .execute(lambda);
// }