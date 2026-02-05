#pragma once

#include "FieldTraits.h"

namespace shine::reflection {

// Extension point for list-like containers.
template <typename List>
struct ListThunks {
    static size_t GetSize(const void *ptr) { return static_cast<const List *>(ptr)->size(); }
    static void  *GetElement(void *ptr, size_t index) {
        auto it = static_cast<List *>(ptr)->begin();
        std::advance(it, index);
        return &(*it);
    }
    static const void *GetElementConst(const void *ptr, size_t index) {
        auto it = static_cast<const List *>(ptr)->begin();
        std::advance(it, index);
        return &(*it);
    }
    static void             Resize(void *ptr, size_t size) { static_cast<List *>(ptr)->resize(size); }
    static const ArrayTrait trait;
};

template <typename List>
const ArrayTrait ListThunks<List>::trait = {
    GetTypeId<typename List::value_type>(),
    FunctionTag<size_t(const void *)>(0),
    FunctionTag<void *(void *, size_t)>(0),
    FunctionTag<const void *(const void *, size_t)>(0),
    FunctionTag<void(void *, size_t)>(0),
    FunctionTableCT<size_t(const void *), 1>{{&GetSize}, 1},
    FunctionTableCT<void *(void *, size_t), 1>{{&GetElement}, 1},
    FunctionTableCT<const void *(const void *, size_t), 1>{{&GetElementConst}, 1},
    FunctionTableCT<void(void *, size_t), 1>{{&Resize}, 1}
};


} // namespace shine::reflection
