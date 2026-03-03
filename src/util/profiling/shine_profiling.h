#pragma once


#include "tracy/tracy/Tracy.hpp"

#define SHINE_PROFILE_CPU_SCOPE() ZoneScoped
#define SHINE_PROFILE_CPU_SCOPE_N(name) ZoneScopedN(name)
#define SHINE_PROFILE_CPU_SCOPE_C(color) ZoneScopedC(color)
#define SHINE_PROFILE_CPU_TEXT(text,size) ZoneText(text,size)
#define SHINE_PROFILE_CPU_NAME(text,size) ZoneName(text,size)
#define SHINE_PROFILE_CPU_VALUE(value) ZoneValue(value)
#define SHINE_PROFILE_CPU_MESSAGE(text,size) TracyMessage(text,size)
#define SHINE_PROFILE_CPU_MESSAGE_L(text) TracyMessageL(text)
#define SHINE_PROFILE_CPU_PLOT(name,value) TracyPlot(name,value)
#define SHINE_PROFILE_CPU_FRAME() FrameMark
#define SHINE_PROFILE_CPU_FRAME_N(name) FrameMarkNamed(name)

#define SHINE_PROFILE_GPU_CONTEXT() TracyGpuContext
#define SHINE_PROFILE_GPU_CONTEXT_NAME(name,size) TracyGpuContextName(name,size)
#define SHINE_PROFILE_GPU_ZONE(name) TracyGpuZone(name)
#define SHINE_PROFILE_GPU_ZONE_C(name,color) TracyGpuZoneC(name,color)
#define SHINE_PROFILE_GPU_COLLECT() TracyGpuCollect

#define SHINE_PROFILE_MEM_ALLOC(ptr,size) TracyAlloc(ptr,size)
#define SHINE_PROFILE_MEM_FREE(ptr) TracyFree(ptr)
#define SHINE_PROFILE_MEM_ALLOC_N(ptr,size,name) TracyAllocN(ptr,size,name)
#define SHINE_PROFILE_MEM_FREE_N(ptr,name) TracyFreeN(ptr,name)

#define SHINE_PROFILE_SCOPE() SHINE_PROFILE_CPU_SCOPE()
#define SHINE_PROFILE_SCOPE_N(name) SHINE_PROFILE_CPU_SCOPE_N(name)
#define SHINE_PROFILE_SCOPE_C(color) SHINE_PROFILE_CPU_SCOPE_C(color)
#define SHINE_PROFILE_TEXT(text,size) SHINE_PROFILE_CPU_TEXT(text,size)
#define SHINE_PROFILE_NAME(text,size) SHINE_PROFILE_CPU_NAME(text,size)
#define SHINE_PROFILE_VALUE(value) SHINE_PROFILE_CPU_VALUE(value)
#define SHINE_PROFILE_MESSAGE(text,size) SHINE_PROFILE_CPU_MESSAGE(text,size)
#define SHINE_PROFILE_MESSAGE_L(text) SHINE_PROFILE_CPU_MESSAGE_L(text)
#define SHINE_PROFILE_PLOT(name,value) SHINE_PROFILE_CPU_PLOT(name,value)
#define SHINE_PROFILE_ALLOC(ptr,size) SHINE_PROFILE_MEM_ALLOC(ptr,size)
#define SHINE_PROFILE_FREE(ptr) SHINE_PROFILE_MEM_FREE(ptr)
#define SHINE_PROFILE_ALLOC_N(ptr,size,name) SHINE_PROFILE_MEM_ALLOC_N(ptr,size,name)
#define SHINE_PROFILE_FREE_N(ptr,name) SHINE_PROFILE_MEM_FREE_N(ptr,name)
#define SHINE_PROFILE_FRAME() SHINE_PROFILE_CPU_FRAME()
#define SHINE_PROFILE_FRAME_N(name) SHINE_PROFILE_CPU_FRAME_N(name)