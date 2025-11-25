#pragma once


#include <span>
#include <string>
#include <string_view>
#include <cstdint>
#include <bit>



namespace shine::util
{
    // 字节单位
    enum class  SizeUnit
    {
        Byte,
        KB,
        MB,
        GB,
        TB,
        PB
    };

    struct  SizeUnitInfo
    {

        double size;
        SizeUnit unit;


        SizeUnitInfo() = default;
        SizeUnitInfo(double _size, SizeUnit _unit) :size(_size), unit(_unit) {
        }
    };


    // 转换 SizeUnit 为字节
     constexpr double convert_size(std::uint64_t size, SizeUnit from, SizeUnit to) noexcept;

     std::string format_file_size(std::uint64_t size_in_bytes, int precision = 2);


     std::string_view unit_to_string(SizeUnit unit);


     constexpr std::string bytes_to_string(const std::span<const std::byte>& data, size_t offset = 0, size_t length = 0);


     constexpr std::string_view bytes_to_string_view(const std::span<const std::byte>& data, size_t offset = 0, size_t length = 0);
    

     constexpr std::string bytes_to_c_string(const std::span<const std::byte>& data, size_t offset = 0);


	template <typename T,std::size_t N = sizeof(T)>
        requires std::integral<T> || std::is_enum_v<T>
     constexpr void read_be_ref(std::span<const std::byte> data,T & value,size_t offset = 0) noexcept
    {
    
        if (offset > data.size() - sizeof(T)) {
            return;
        }

        memcpy(&value, data.data() + offset, sizeof(T));
        

        if constexpr (N == 1){
            return;
        }

        if constexpr (std::endian::native == std::endian::little)
        {
#ifdef _MSC_VER
            if constexpr (N==2) value = _byteswap_ushort(value);
            else if constexpr (N==4) value = _byteswap_ulong(value);
            else if constexpr (N==8) value = _byteswap_uint64(value);
#elif  defined(__GNUC__) || defined(__clang__)
            if constexpr (N==2) value = __builtin_bswap16(value);
            else if constexpr (N==4) value = __builtin_bswap32(value);
            else if constexpr (N==8) value = __builtin_bswap64(value);
#endif
        }
    }

	template <typename T,std::size_t N = sizeof(T)>
        requires std::integral<T> || std::is_enum_v<T>
	constexpr T read_be(std::span<const std::byte> data, size_t offset = 0) noexcept
    {
        T value  = {};
        read_be_ref<T,N>(data, value, offset);
        return value;
    }



    constexpr uint8_t read_u8(std::span<const std::byte> data, size_t offset = 0) noexcept
    {
        return read_be<uint8_t>(data,offset);
    }

    constexpr  uint16_t read_u16(std::span<const std::byte> data,size_t offset = 0) noexcept
    {
        return read_be<uint16_t>(data, offset);
    }

    constexpr uint32_t read_u32(std::span<const std::byte> data,size_t offset = 0) noexcept
    {
        return read_be<uint32_t>(data, offset);
    }

    // 小端序读取函数（Little Endian）
    template <typename T,std::size_t N = sizeof(T)>
        requires std::integral<T> || std::is_enum_v<T>
    constexpr void read_le_ref(std::span<const std::byte> data,T & value,size_t offset = 0) noexcept
    {
        if (offset > data.size() - sizeof(T)) {
            return;
        }

        memcpy(&value, data.data() + offset, sizeof(T));

        if constexpr (N == 1){
            return;
        }

        if constexpr (std::endian::native == std::endian::big)
        {
#ifdef _MSC_VER
            if constexpr (N==2) value = _byteswap_ushort(value);
            else if constexpr (N==4) value = _byteswap_ulong(value);
            else if constexpr (N==8) value = _byteswap_uint64(value);
#elif  defined(__GNUC__) || defined(__clang__)
            if constexpr (N==2) value = __builtin_bswap16(value);
            else if constexpr (N==4) value = __builtin_bswap32(value);
            else if constexpr (N==8) value = __builtin_bswap64(value);
#endif
        }
    }

    template <typename T,std::size_t N = sizeof(T)>
        requires std::integral<T> || std::is_enum_v<T>
    constexpr T read_le(std::span<const std::byte> data, size_t offset = 0) noexcept
    {
        T value  = {};
        read_le_ref<T,N>(data, value, offset);
        return value;
    }

    constexpr uint16_t read_le16(std::span<const std::byte> data, size_t offset = 0) noexcept
    {
        return read_le<uint16_t>(data, offset);
    }

    constexpr uint32_t read_le24(std::span<const std::byte> data, size_t offset = 0) noexcept
    {
        if (offset + 3 > data.size()) {
            return 0;
        }
        uint32_t value = static_cast<uint32_t>(data[offset + 0]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16);
        return value;
    }

    constexpr uint32_t read_le32(std::span<const std::byte> data, size_t offset = 0) noexcept
    {
        return read_le<uint32_t>(data, offset);
    }



    template<typename T,  std::size_t N = sizeof(T)>
        requires std::integral<T>
	constexpr void  byte_convert_ref(std::span<const std::byte> bytes,T & Value) noexcept
    {

        if constexpr (N == 1) {
            Value = static_cast<T>(bytes[0]);
            return;
        }

        if constexpr (std::endian::native == std::endian::little)
        {
#ifdef _MSC_VER
            if constexpr (N==2) Value = _byteswap_ushort(*reinterpret_cast<const uint16_t*>(bytes.data()));
            else if constexpr (N==4) Value = _byteswap_ulong(*reinterpret_cast<const uint32_t*>(bytes.data()));
            else if constexpr (N==8) Value = _byteswap_uint64(*reinterpret_cast<const uint64_t*>(bytes.data()));
#elif  defined(__GNUC__) || defined(__clang__)
            if constexpr (N==2) Value = __builtin_bswap16(*reinterpret_cast<const uint16_t*>(bytes.data()));
            else if constexpr (N==4) Value = __builtin_bswap32(*reinterpret_cast<const uint32_t*>(bytes.data()));
            else if constexpr (N==8) Value = __builtin_bswap64(*reinterpret_cast<const uint64_t*>(bytes.data()));
#endif
        }
        else
        {
            Value = static_cast<T>(bytes.data());
        }
    };

    template<typename T, std::size_t N = sizeof(T)>
        requires std::integral<T> || std::is_enum_v<T>
	constexpr T byte_convert(std::span<const std::byte> bytes) noexcept
    {

        T value = {};
        byte_convert_ref<T,N>(bytes, value);
        return value;
    }

}

