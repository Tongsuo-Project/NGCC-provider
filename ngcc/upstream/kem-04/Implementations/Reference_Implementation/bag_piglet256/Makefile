BUILD_DIR ?= build
TARGET := $(notdir $(CURDIR))

.PHONY: all configure functional benchmark kat clean

all: kat

configure:
	cmake -S . -B $(BUILD_DIR)

functional: configure
	cmake --build $(BUILD_DIR) --target $(TARGET) -j

benchmark: configure
	cmake --build $(BUILD_DIR) --target $(TARGET)-bench -j

kat: configure
	cmake --build $(BUILD_DIR) --target $(TARGET)-kat -j

clean:
	cmake --build $(BUILD_DIR) --target clean
