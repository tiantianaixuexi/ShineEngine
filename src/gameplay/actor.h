#pragma once
#include "object.h"

namespace shine::gameplay
{

    class SActor : public SObject
    {
    public:
        const char* getClassName() const noexcept override { return "SActor"; }
    };

    class EmptyActor : public SActor
    {
    public:
        const char* getClassName() const noexcept override { return "EmptyActor"; }
    };

    class StaticMeshActor : public SActor
    {
    public:
        const char* getClassName() const noexcept override { return "StaticMeshActor"; }
    };

    template <typename T, typename... TArgs>
    requires std::is_base_of_v<SActor, T>
    T* SpawnActor(TArgs&&... args) {
        return NewObject<T>(std::forward<TArgs>(args)...);
    }

    template <typename T>
    requires std::is_base_of_v<SActor, T>
    void DestroyActor(T* actor) {
        DestroyObject(actor);
    }
} 
