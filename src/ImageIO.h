#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include <cstdint>
#include <string>
#include <vector>

struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgb;
};

bool loadBmp(const std::string& filename, Image& image, std::string& error);
bool saveBmp(const std::string& filename, const Image& image, std::string& error);

#endif
