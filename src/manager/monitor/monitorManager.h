export module manager.mointorManager;

#include "fmt.h"
#include "util/timer_util.h"
#include "util/singleton.h"

namespace shine::manager
{
    //export struct EXPORT_MONITOR_MANAGER FMonitorInfo
    //{
    //    std::string name;
    //    

    //};

    //export struct EXPORT_MONITOR_MANAGER FMonitorItem
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
    //class EXPORT_MONITOR_MANAGER monitorManager : public shine::util::Singleton<monitorManager>
    //{
    //    public:
   

    //        std::unordered_map<std::string, std::string> monitorMap;
    //        

    //    private:



    //};

}
