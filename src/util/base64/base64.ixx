#ifdef SHINE_USE_MODULE

export module shine.base64;
import <string>;
import <string_view>;

#else

#pragma once
#include <vector>
#include <string>
#include <string_view>

#endif

#if SHINE_USE_MODULE

export namespace shine::base64 {
#else 
namespace shine::base64{

#endif

	std::string base64_encode(std::string_view data) noexcept;
	std::string base64_decode(std::string_view encoded_string) noexcept;

}

