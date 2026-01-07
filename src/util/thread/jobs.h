#pragma once

#include "util/shine_define.h"
#include <variant>
#include <string>

namespace shine::util::job
{
    // Define concrete Job types
    
    struct JobPhysicsStep {
        float deltaTime;
    };

    struct JobUpdateLogic {
        float deltaTime;
    };

    struct JobRenderExtract {
        u64 frameNumber;
    };

    struct JobAssetLoad {
        std::string path;
        u32 assetId;
    };

    struct JobCompileShader {
        std::string source;
        u32 shaderId;
    };

    struct JobShutdown {
        bool force;
    };

    struct JobExecuteTick {
        void(*fn)(void*, float);
        void* userdata;
        float deltaTime;
    };

    // Internal Job to execute a managed task from Scheduler
    struct JobExecuteTaskNode {
        u32 taskId;
    };

    // The Algebraic Data Type for all possible jobs
    using Job = std::variant<
        JobPhysicsStep,
        JobUpdateLogic,
        JobRenderExtract,
        JobAssetLoad,
        JobCompileShader,
        JobShutdown,
        JobExecuteTaskNode,
        JobExecuteTick
    >;
}
