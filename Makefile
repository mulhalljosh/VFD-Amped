# Hardware-free mock build. Does not require PlatformIO or an ESP32 toolchain.
#   make            — build amped-mock
#   make test       — run API / failsafe self-tests
#   make run        — serve Web UI + REST on :8080
#   make pio-native — optional PlatformIO native env (if pio is installed)

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -DAMPED_MOCK=1 -Iinclude -Isrc -I.
LDFLAGS  ?=
BUILD    := build
BIN      := $(BUILD)/amped-mock

SRCS := \
	src/main.cpp \
	src/app/json.cpp \
	src/app/config.cpp \
	src/app/controller.cpp \
	src/hal/dac.cpp \
	src/hal/io.cpp \
	src/hal/temps.cpp \
	src/hal/rs485.cpp \
	src/hal/time.cpp \
	src/web/api.cpp \
	src/web/server.cpp \
	src/net/mqtt.cpp \
	src/net/ota.cpp \
	src/net/nvs_store.cpp

OBJS := $(SRCS:%.cpp=$(BUILD)/%.o)

.PHONY: all test run clean pio-native

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test: $(BIN)
	$(BIN) --self-test

run: $(BIN)
	$(BIN) --port 8080

clean:
	rm -rf $(BUILD)

pio-native:
	pio run -e native
