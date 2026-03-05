#include <type_traits>

#include "Engine/Macro/RuntimeEditorSplit.h"

struct SplitProbe
{
    RUNTIME_DATA(int runtimeValue = 1;)
    RUNTIME_DATA(int(*runtimeFn)(int) = nullptr;)
    EDITOR_DATA(int editorValue = 2;)
    EDITOR_DATA(struct NestedEditor
    {
        int thumbnailKey = 0;
    } nestedEditor;)
};

template<typename T>
concept HasRuntimeValue = requires(T v)
{
    v.runtimeValue;
};

template<typename T>
concept HasEditorValue = requires(T v)
{
    v.editorValue;
};

RUNTIME_DATA(int RuntimeOnlySymbol() { return 7; })
EDITOR_DATA(int EditorOnlySymbol() { return 9; })

int main()
{
#if defined(BUILD_RUNTIME) && BUILD_RUNTIME
    static_assert(HasRuntimeValue<SplitProbe>);
    static_assert(sizeof(SplitProbe) >= sizeof(int) + sizeof(void*));
#else
    static_assert(!HasRuntimeValue<SplitProbe>);
#endif

#if defined(BUILD_EDITOR) && BUILD_EDITOR
    static_assert(HasEditorValue<SplitProbe>);
    return EditorOnlySymbol() == 9 ? 0 : 1;
#else
    static_assert(!HasEditorValue<SplitProbe>);
    return RuntimeOnlySymbol() == 7 ? 0 : 1;
#endif
}
