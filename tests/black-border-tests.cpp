#include "../src/black-border.hpp"

#include <cstdint>
#include <vector>

static void fill_rect(std::vector<uint8_t> &pixels, uint32_t width, uint32_t left, uint32_t top, uint32_t right,
		      uint32_t bottom, uint8_t value)
{
	for (uint32_t y = top; y < bottom; ++y) {
		for (uint32_t x = left; x < right; ++x) {
			const size_t i = (static_cast<size_t>(y) * width + x) * 4;
			pixels[i] = pixels[i + 1] = pixels[i + 2] = value;
		}
	}
}

int main()
{
	constexpr uint32_t width = 200;
	constexpr uint32_t height = 100;
	std::vector<uint8_t> pixels(width * height * 4, 0);
	for (size_t i = 3; i < pixels.size(); i += 4)
		pixels[i] = 255;

	fill_rect(pixels, width, 20, 10, 185, 92, 90);
	auto crop = detect_black_borders(pixels.data(), width, height, width * 4);
	if (!crop.valid || crop.left != 20 || crop.top != 10 || crop.right != 15 || crop.bottom != 8)
		return 1;

	// Compression noise in a bar should not stop the edge scan.
	fill_rect(pixels, width, 50, 2, 51, 3, 40);
	crop = detect_black_borders(pixels.data(), width, height, width * 4);
	if (!crop.valid || crop.top != 10)
		return 2;

	fill_rect(pixels, width, 0, 0, width, height, 0);
	if (detect_black_borders(pixels.data(), width, height, width * 4).valid)
		return 3;

	fill_rect(pixels, width, 0, 0, width, height, 100);
	if (detect_black_borders(pixels.data(), width, height, width * 4).valid)
		return 4;
}
