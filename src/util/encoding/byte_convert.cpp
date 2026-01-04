
#ifdef SHINE_USE_MODULE

module;

#include "shine_define.h"
#include "fmt/format.h"

module shine.byte_convert;

#else

#include "byte_convert.ixx"
#include "fmt/format.h"

#include <string>

#endif


namespace shine::util
{

	constexpr double convert_size(s64 size, SizeUnit from, SizeUnit to) noexcept
    {
        double bytes = static_cast<double>(size);
        switch (from)
        {
        case SizeUnit::PB:
            bytes *= 1024.0;
            [[fallthrough]];
        case SizeUnit::TB:
            bytes *= 1024.0;
            [[fallthrough]];
        case SizeUnit::GB:

            bytes *= 1024.0;
            [[fallthrough]];
        case SizeUnit::MB:
            bytes *= 1024.0;
            [[fallthrough]];
        case SizeUnit::KB:
            bytes *= 1024.0;
            [[fallthrough]];
        case SizeUnit::Byte:
            break;
        }


        switch (to)
        {
        case SizeUnit::Byte:
            return bytes;
        case SizeUnit::KB:
            return bytes / 1024.0;
        case SizeUnit::MB:
            return bytes / (1024.0 * 1024.0);
        case SizeUnit::GB:
            return bytes / (1024.0 * 1024.0 * 1024.0);
        case SizeUnit::TB:
            return bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0);
        case SizeUnit::PB:
            return bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0);
        default:
            return bytes;
        }

    }


    SizeUnitInfo get_appropriate_size(std::uint64_t size_in_bytes) noexcept
    {
        constexpr double threshold = 1024.0;
        double size = static_cast<double>(size_in_bytes);
        SizeUnit unit = SizeUnit::Byte;

        if (size >= threshold)
        {
            size /= 1024.0;
            unit = SizeUnit::KB;
        }

        if (size >= threshold)
        {
            size /= 1024.0;
            unit = SizeUnit::MB;
        }

        if (size >= threshold)
        {
            size /= 1024.0;
            unit = SizeUnit::GB;
        }

        if (size >= threshold)
        {
            size /= 1024.0;
            unit = SizeUnit::TB;
        }

        if (size >= threshold)
        {
            size /= 1024.0;
            unit = SizeUnit::PB;
        }

        return { size, unit };
    }

    std::string_view unit_to_string(SizeUnit unit) noexcept
    {
        switch (unit)
        {
        case SizeUnit::Byte:
            return "B";
        case SizeUnit::KB:
            return "KB";
        case SizeUnit::MB:
            return "MB";
        case SizeUnit::GB:
            return "GB";
        case SizeUnit::TB:
            return "TB";
        case SizeUnit::PB:
            return "PB";
        default:
			return "B";
        }
    }

    std::string format_file_size(u64 size_in_bytes, int precision) noexcept
    {
        auto [size, unit] = get_appropriate_size(size_in_bytes);
        return fmt::format("{} {} {}", size, precision, unit_to_string(unit));
    }

    constexpr std::string_view bytes_to_string(const unsigned char* data, size_t data_size, size_t offset, size_t length) {

        if (!data || offset >= data_size)
            return {};

        if (length == 0) [[unlikely]]
        {
            length = data_size - offset;
        }

        // 防止越界
        if (offset + length > data_size)
        {
            length = data_size - offset;
        }

        return std::string_view{
            reinterpret_cast<const char*>(data + offset),
            length
        };
    }

    constexpr std::string_view bytes_to_string(const char* data, size_t data_size, size_t offset, size_t length)
    {
        return bytes_to_string(
            reinterpret_cast<const unsigned char*>(data),
            data_size,
            offset,
            length
        );
    }
}

