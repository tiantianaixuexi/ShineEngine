#include "AssetUuidHelper.h"

namespace shine::editor::asset
{
    SString GenerateUUIDString()
    {
        shine::algorithm::UUID id = shine::algorithm::UUID::GenerateV4();
        return SString(id.ToString());
    }

    SString GenerateV7UUIDString()
    {
        shine::algorithm::UUID id = shine::algorithm::UUID::GenerateV7();
        return SString(id.ToString());
    }

    std::optional<shine::algorithm::UUID> ParseUUIDString(STextView uuidStr)
    {
        auto result = shine::algorithm::UUID::FromString(std::string_view(uuidStr.sv()));
        if (!result.has_value())
            return std::nullopt;
        return result.value();
    }

    SString UUIDToString(const shine::algorithm::UUID& id)
    {
        return SString(id.ToString());
    }

    bool IsValidUUIDString(STextView s) noexcept
    {
        return shine::algorithm::UUID::FromString(std::string_view(s.sv())).has_value();
    }

    SString NilUUIDString()
    {
        return SString("00000000-0000-0000-0000-000000000000");
    }

} // namespace shine::editor::asset
