#include "black-border.hpp"

#include <algorithm>
#include <cstddef>

namespace {

constexpr uint8_t black_threshold = 20;

bool is_content(const uint8_t *pixel)
{
	if (pixel[3] <= 16)
		return false;
	return std::max({pixel[0], pixel[1], pixel[2]}) > black_threshold;
}

bool row_has_content(const uint8_t *pixels, uint32_t width, uint32_t stride, uint32_t y)
{
	const uint8_t *row = pixels + static_cast<size_t>(y) * stride;
	uint32_t count = 0;
	const uint32_t required = std::max(2u, width / 100u);
	for (uint32_t x = 0; x < width; ++x) {
		count += is_content(row + static_cast<size_t>(x) * 4);
		if (count >= required)
			return true;
	}
	return false;
}

bool column_has_content(const uint8_t *pixels, uint32_t stride, uint32_t x, uint32_t top, uint32_t bottom)
{
	uint32_t count = 0;
	const uint32_t required = std::max(2u, (bottom - top) / 100u);
	for (uint32_t y = top; y < bottom; ++y) {
		const uint8_t *pixel = pixels + static_cast<size_t>(y) * stride + static_cast<size_t>(x) * 4;
		count += is_content(pixel);
		if (count >= required)
			return true;
	}
	return false;
}

} // namespace

BlackBorderCrop detect_black_borders(const uint8_t *pixels, uint32_t width, uint32_t height, uint32_t stride)
{
	BlackBorderCrop result;
	if (!pixels || width < 4 || height < 4 || stride < static_cast<uint64_t>(width) * 4)
		return result;

	uint32_t top = 0;
	while (top < height && !row_has_content(pixels, width, stride, top))
		++top;
	if (top == height)
		return result;

	uint32_t bottom = height;
	while (bottom > top && !row_has_content(pixels, width, stride, bottom - 1))
		--bottom;

	uint32_t left = 0;
	while (left < width && !column_has_content(pixels, stride, left, top, bottom))
		++left;
	uint32_t right = width;
	while (right > left && !column_has_content(pixels, stride, right - 1, top, bottom))
		--right;

	// A nearly empty frame is usually a fade or a black screen. Leave it alone.
	if (right - left < width / 10 || bottom - top < height / 10)
		return result;

	result.left = left;
	result.top = top;
	result.right = width - right;
	result.bottom = height - bottom;
	result.valid = left >= 2 || top >= 2 || result.right >= 2 || result.bottom >= 2;
	return result;
}
