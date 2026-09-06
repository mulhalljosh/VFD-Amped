#pragma once

#include <string>

namespace amped {

bool nvs_load();
bool nvs_save();

void mqtt_begin();
void mqtt_publish_status();

void ota_begin();
void ota_loop();

bool http_serve(int port, const char* www_root);
int native_main(int argc, char** argv);

#if !AMPED_MOCK
void esp32_setup();
void esp32_loop();
#endif

}  // namespace amped
