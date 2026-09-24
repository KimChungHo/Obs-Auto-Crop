#pragma once

#include <cstdint>

struct BlackBorderCrop {
	uint32_t left = 0;
	uint32_t top = 0;
	uint32_t right = 0;
	uint32_t bottom = 0;
	bool valid = false;
};

// Pixels are BGRA8. A valid result contains the number of dark rows/columns
// measured inward from each edge of the captured frame.
BlackBorderCrop detect_black_borders(const uint8_t *pixels, uint32_t width, uint32_t height, uint32_t stride);
