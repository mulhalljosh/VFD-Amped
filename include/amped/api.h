#pragma once

#include <string>

namespace amped {

struct HttpResponse {
  int status = 200;
  const char* content_type = "application/json";
  std::string body;
};

bool api_authorized(const char* api_key_header);
HttpResponse api_handle(const char* method, const char* path, const char* body,
                        const char* api_key_header);

bool run_self_tests();

}  // namespace amped
