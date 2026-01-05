export module manager.mointorManager;

#include "fmt.h"
#include "util/timer_util.h"
#include "../../EngineCore/subsystem.h"
#include "../../EngineCore/engine_context.h"

namespace shine::manager
{
    //export struct MONITOR_MANAGER_API FMonitorInfo
    //{
    //    std::string name;
    //    

    //};

    //export struct MONITOR_MANAGER_API FMonitorItem
    //{

    //    float now;

    //    FMonitorItem() = default;
    //    FMonitorItem(const std::string& _name) : name(_name) {
    //        now = util::get_now_ms_platform<float>();
    //    }

    //    ~FMonitorItem()
    //    {
    //        const float last = util::get_now_ms_platform<float>();
    //        const float time = last - now;
    //    }

    //    

    //};

    //// 鐩戞帶 Manager
    //class MONITOR_MANAGER_API monitorManager : public shine::Subsystem
    //{
    //    public:
    //        static constexpr size_t GetStaticID() { return shine::HashString("monitorManager"); }

    //        std::unordered_map<std::string, std::string> monitorMap;
    //        

    //    private:



    //};
}
