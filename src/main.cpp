#include "amped/net.h"
#include "amped/version.h"

#if AMPED_MOCK

int main(int argc, char** argv) { return amped::native_main(argc, argv); }

#else

void setup() { amped::esp32_setup(); }
void loop() { amped::esp32_loop(); }

#endif
