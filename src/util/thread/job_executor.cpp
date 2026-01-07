#include "job_executor.h"
#include "task_scheduler.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace shine::util::job
{
    void JobExecutor::operator()(const JobPhysicsStep& job)
    {
        // Simulate physics
        // std::cout << "Running Physics Step: " << job.deltaTime << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void JobExecutor::operator()(const JobUpdateLogic& job)
    {
        // std::cout << "Updating Logic: " << job.deltaTime << std::endl;
    }

    void JobExecutor::operator()(const JobRenderExtract& job)
    {
        // std::cout << "Render Extract Frame: " << job.frameNumber << std::endl;
    }

    void JobExecutor::operator()(const JobAssetLoad& job)
    {
        // std::cout << "Loading Asset: " << job.path << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void JobExecutor::operator()(const JobCompileShader& job)
    {
        // std::cout << "Compiling Shader: " << job.shaderId << std::endl;
    }

    void JobExecutor::operator()(const JobShutdown& job)
    {
        // std::cout << "Shutdown Job" << std::endl;
    }

    void JobExecutor::operator()(const JobExecuteTaskNode& job)
    {
        // Call back into TaskScheduler to execute the managed task
        shine::util::TaskScheduler::Get().ExecuteTask(job.taskId);
    }

    void JobExecutor::operator()(const JobExecuteTick& job)
    {
        if (job.fn)
        {
            job.fn(job.userdata, job.deltaTime);
        }
    }
}
