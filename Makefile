BUILD_DIR := build

.PHONY: build run test bench

build:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --target needle

run: build
	$(BUILD_DIR)/needle $(FILE) $(PATTERN)

test:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --target tests
	cd $(BUILD_DIR) && ctest --output-on-failure

bench:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --target benchmarks
	$(BUILD_DIR)/benchmarks
