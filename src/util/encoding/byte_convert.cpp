#include "byte_convert.h"

#include "fmt/format.h"

#include <span>
#include <string>
#include <bit>


namespace shine::util
{

    constexpr double convert_size(std::uint64_t size, SizeUnit from, SizeUnit to) noexcept
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


    SizeUnitInfo get_appropriate_size(std::uint64_t size_in_bytes)
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

    std::string_view unit_to_string(SizeUnit unit)
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

    std::string format_file_size(std::uint64_t size_in_bytes, int precision)
    {
        auto [size, unit] = get_appropriate_size(size_in_bytes);
        return fmt::format("{} {} {}", size, precision, unit_to_string(unit));
    }

    template <std::integral T>
    constexpr T read_be(std::span<const std::byte> data, size_t offset) noexcept {
 
        if (offset > data.size() - sizeof(T)) {
            return {};
        }

        T value;

        memcpy(&value, data.data() + offset, sizeof(T));


        if constexpr (std::endian::native == std::endian::little) {
            return value;
        }
        else
        {
            return std::byteswap(value);
        }
    }

    constexpr std::string bytes_to_string(const std::span<const std::byte>& data, size_t offset, size_t length) {

        if (length == 0) {
            length = data.size() > offset ? data.size() - offset : 0;
        }

        if (offset >= data.size()) {
            return {};
        }

        std::string result;
        result.reserve(length);

        for (size_t i = 0; i < length; ++i) {
            result.push_back(static_cast<char>(data[offset + i]));
        }

        return result;
    }


    constexpr std::string_view bytes_to_string_view(const std::span<const std::byte>& data, size_t offset, size_t length) {
   
        if (length == 0) {
            length = data.size() > offset ? data.size() - offset : 0;
        }

   
        if (offset >= data.size()) {
            return {};
        }


        return std::string_view{ reinterpret_cast<const char*>(data.data() + offset), length };
    }


    constexpr std::string bytes_to_c_string(const std::span<const std::byte>& data, size_t offset) {
  
        if (offset >= data.size()) {
            return {};
        }

    
        std::size_t length = 0;
        while (offset + length < data.size() && data[offset + length] != std::byte{ 0 }) {
            ++length;
        }

        return bytes_to_string(data, offset, length);
    }
}

