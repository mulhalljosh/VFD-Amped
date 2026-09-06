#pragma once

#include <cstdint>
#include <string>

namespace amped {

bool json_get_string(const std::string& body, const char* key, std::string& out);
bool json_get_float(const std::string& body, const char* key, float& out);
bool json_get_bool(const std::string& body, const char* key, bool& out);
bool json_get_uint32(const std::string& body, const char* key, uint32_t& out);

std::string json_escape(const std::string& in);

}  // namespace amped
