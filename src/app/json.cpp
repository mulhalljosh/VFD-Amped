#include "amped/json.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>

namespace amped {
namespace {

const char* find_key(const std::string& body, const char* key) {
  std::string quoted = std::string("\"") + key + "\"";
  size_t pos = 0;
  while ((pos = body.find(quoted, pos)) != std::string::npos) {
    size_t colon = body.find(':', pos + quoted.size());
    if (colon == std::string::npos) return nullptr;
    const char* p = body.c_str() + colon + 1;
    while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
    return p;
  }
  return nullptr;
}

}  // namespace

bool json_get_string(const std::string& body, const char* key, std::string& out) {
  const char* p = find_key(body, key);
  if (!p || *p != '"') return false;
  ++p;
  out.clear();
  while (*p && *p != '"') {
    if (*p == '\\' && p[1]) {
      ++p;
    }
    out.push_back(*p++);
  }
  return true;
}

bool json_get_float(const std::string& body, const char* key, float& out) {
  const char* p = find_key(body, key);
  if (!p || *p == '"' || *p == 't' || *p == 'f' || *p == 'n') return false;
  char* end = nullptr;
  float v = std::strtof(p, &end);
  if (end == p) return false;
  out = v;
  return true;
}

bool json_get_bool(const std::string& body, const char* key, bool& out) {
  const char* p = find_key(body, key);
  if (!p) return false;
  if (p[0] == 't') {
    out = true;
    return true;
  }
  if (p[0] == 'f') {
    out = false;
    return true;
  }
  if (p[0] == '1' || p[0] == '0') {
    out = (p[0] == '1');
    return true;
  }
  return false;
}

bool json_get_uint32(const std::string& body, const char* key, uint32_t& out) {
  float tmp = 0;
  if (!json_get_float(body, key, tmp) || tmp < 0) return false;
  out = static_cast<uint32_t>(tmp);
  return true;
}

std::string json_escape(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (char c : in) {
    if (c == '"' || c == '\\') out.push_back('\\');
    out.push_back(c);
  }
  return out;
}

}  // namespace amped
