#include "EditorAssetRegistryIndex.h"

#include <chrono>
#include <format>

#include "glaze/json.hpp"

namespace shine::editor::asset
{
    namespace
    {
        constexpr glz::opts kReadOpts  { .error_on_unknown_keys = false };
        constexpr glz::opts kWriteOpts { .prettify = false }; // compact — size matters

        std::int64_t GetFileModifiedTime(const std::filesystem::path& p)
        {
            std::error_code ec;
            auto lwt = std::filesystem::last_write_time(p, ec);
            if (ec)
                return 0;
            // Convert file_time_type to Unix seconds
            auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::clock_cast<std::chrono::system_clock>(lwt));
            return sctp.time_since_epoch().count();
        }

        std::string NowISO8601()
        {
            auto now = std::chrono::system_clock::now();
            return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(now));
        }
    }

    bool SaveRegistryIndex(const EditorAssetRegistry&   registry,
                           const std::filesystem::path& indexPath)
    {
        RegistryIndexDocument doc;
        doc.writtenAt = NowISO8601();

        registry.ForEach([&](const EditorAssetEntry& entry)
        {
            if (entry.isDangling)
                return; // skip dangling — they'll be rediscovered or gone

            RegistryIndexEntry ie;
            ie.uuid     = entry.uuid.to_string();
            ie.diskPath = entry.diskPath.to_string();
            ie.type     = entry.record.type;
            ie.dependencies.assign(
                entry.record.dependencies.begin(),
                entry.record.dependencies.end());
            ie.lastModified = GetFileModifiedTime(entry.diskPath.sv());
            doc.entries.push_back(std::move(ie));
        });

        auto jsonRes = glz::write<kWriteOpts>(doc);
        if (!jsonRes) return false;
        std::ofstream f(indexPath, std::ios::binary);
        if (!f) return false;
        f << jsonRes.value();
        return f.good();
    }

    bool LoadRegistryIndex(EditorAssetRegistry&              registry,
                           const std::filesystem::path&      indexPath,
                           std::vector<std::filesystem::path>& outStaleFiles)
    {
        RegistryIndexDocument doc;
        std::string buf;
        if (auto ec = glz::read_file_json(doc, indexPath.string(), buf); ec)
            return false;

        for (const auto& ie : doc.entries)
        {
            std::filesystem::path p = ie.diskPath;
            std::error_code fsEc;
            if (!std::filesystem::exists(p, fsEc) || fsEc)
            {
                outStaleFiles.push_back(p);
                continue;
            }
            const std::int64_t currentMtime = GetFileModifiedTime(p);
            if (currentMtime != ie.lastModified)
            {
                outStaleFiles.push_back(p);
                continue;
            }
            AssetRecord rec;
            rec.uuid         = ie.uuid;
            rec.type         = ie.type;
            rec.dependencies = ie.dependencies;
            registry.Register(p, std::move(rec));
        }

        return true;
    }

} // namespace shine::editor::asset
