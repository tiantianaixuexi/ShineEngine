#pragma once

#include "jobs.h"

namespace shine::util::job
{
    struct JobExecutor
    {
        void operator()(const JobPhysicsStep& job);
        void operator()(const JobUpdateLogic& job);
        void operator()(const JobRenderExtract& job);
        void operator()(const JobAssetLoad& job);
        void operator()(const JobCompileShader& job);
        void operator()(const JobShutdown& job);
        void operator()(const JobExecuteTaskNode& job);
        void operator()(const JobExecuteTick& job);
    };
}
