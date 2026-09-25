#include "ImageIO.h"

#include <fstream>
#include <limits>

namespace {

std::uint16_t readU16(std::istream& input) {
    std::uint8_t bytes[2]{};
    input.read(reinterpret_cast<char*>(bytes), 2);
    return static_cast<std::uint16_t>(bytes[0]) |
        (static_cast<std::uint16_t>(bytes[1]) << 8);
}

std::uint32_t readU32(std::istream& input) {
    std::uint8_t bytes[4]{};
    input.read(reinterpret_cast<char*>(bytes), 4);
    return static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8) |
        (static_cast<std::uint32_t>(bytes[2]) << 16) |
        (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::int32_t readI32(std::istream& input) {
    return static_cast<std::int32_t>(readU32(input));
}

void writeU16(std::ostream& output, std::uint16_t value) {
    const std::uint8_t bytes[2] = {
        static_cast<std::uint8_t>(value),
        static_cast<std::uint8_t>(value >> 8)
    };
    output.write(reinterpret_cast<const char*>(bytes), 2);
}

void writeU32(std::ostream& output, std::uint32_t value) {
    const std::uint8_t bytes[4] = {
        static_cast<std::uint8_t>(value),
        static_cast<std::uint8_t>(value >> 8),
        static_cast<std::uint8_t>(value >> 16),
        static_cast<std::uint8_t>(value >> 24)
    };
    output.write(reinterpret_cast<const char*>(bytes), 4);
}

}  // namespace

bool loadBmp(const std::string& filename, Image& image, std::string& error) {
    std::ifstream input(filename, std::ios::binary);
    if (!input) {
        error = "无法打开输入文件";
        return false;
    }

    if (readU16(input) != 0x4D42) {
        error = "文件不是 BMP 图像";
        return false;
    }

    readU32(input);  // 文件大小
    readU16(input);
    readU16(input);
    const std::uint32_t pixelOffset = readU32(input);
    const std::uint32_t dibSize = readU32(input);
    if (dibSize < 40) {
        error = "不支持该 BMP 信息头";
        return false;
    }

    const std::int32_t width = readI32(input);
    const std::int32_t signedHeight = readI32(input);
    const std::uint16_t planes = readU16(input);
    const std::uint16_t bitsPerPixel = readU16(input);
    const std::uint32_t compression = readU32(input);

    if (width <= 0 || signedHeight == 0 || planes != 1 ||
        (bitsPerPixel != 24 && bitsPerPixel != 32) || compression != 0) {
        error = "仅支持未压缩的 24 位或 32 位 BMP";
        return false;
    }

    const int height = signedHeight < 0 ? -signedHeight : signedHeight;
    const bool topDown = signedHeight < 0;
    const std::size_t rowBytes =
        ((static_cast<std::size_t>(width) * bitsPerPixel + 31) / 32) * 4;
    const std::size_t pixelCount = static_cast<std::size_t>(width) * height;
    if (pixelCount > std::numeric_limits<std::size_t>::max() / 3) {
        error = "图像尺寸过大";
        return false;
    }

    image.width = width;
    image.height = height;
    image.rgb.assign(pixelCount * 3, 0);
    input.seekg(pixelOffset, std::ios::beg);

    std::vector<std::uint8_t> row(rowBytes);
    const int sourceBytesPerPixel = bitsPerPixel / 8;
    for (int sourceY = 0; sourceY < height; ++sourceY) {
        input.read(reinterpret_cast<char*>(row.data()),
            static_cast<std::streamsize>(row.size()));
        if (!input) {
            error = "BMP 像素数据不完整";
            return false;
        }

        const int targetY = topDown ? sourceY : height - 1 - sourceY;
        for (int x = 0; x < width; ++x) {
            const std::size_t source = static_cast<std::size_t>(x) * sourceBytesPerPixel;
            const std::size_t target =
                (static_cast<std::size_t>(targetY) * width + x) * 3;
            image.rgb[target] = row[source + 2];
            image.rgb[target + 1] = row[source + 1];
            image.rgb[target + 2] = row[source];
        }
    }

    return true;
}

bool saveBmp(const std::string& filename, const Image& image, std::string& error) {
    if (image.width <= 0 || image.height <= 0 ||
        image.rgb.size() != static_cast<std::size_t>(image.width) * image.height * 3) {
        error = "输出图像尺寸或像素数据无效";
        return false;
    }

    const std::size_t rowBytes =
        ((static_cast<std::size_t>(image.width) * 24 + 31) / 32) * 4;
    const std::size_t pixelBytes = rowBytes * image.height;
    if (pixelBytes > std::numeric_limits<std::uint32_t>::max() - 54) {
        error = "输出图像过大";
        return false;
    }

    std::ofstream output(filename, std::ios::binary);
    if (!output) {
        error = "无法创建输出文件";
        return false;
    }

    writeU16(output, 0x4D42);
    writeU32(output, static_cast<std::uint32_t>(54 + pixelBytes));
    writeU16(output, 0);
    writeU16(output, 0);
    writeU32(output, 54);
    writeU32(output, 40);
    writeU32(output, static_cast<std::uint32_t>(image.width));
    writeU32(output, static_cast<std::uint32_t>(image.height));
    writeU16(output, 1);
    writeU16(output, 24);
    writeU32(output, 0);
    writeU32(output, static_cast<std::uint32_t>(pixelBytes));
    writeU32(output, 2835);
    writeU32(output, 2835);
    writeU32(output, 0);
    writeU32(output, 0);

    std::vector<std::uint8_t> row(rowBytes, 0);
    for (int sourceY = image.height - 1; sourceY >= 0; --sourceY) {
        for (int x = 0; x < image.width; ++x) {
            const std::size_t source =
                (static_cast<std::size_t>(sourceY) * image.width + x) * 3;
            const std::size_t target = static_cast<std::size_t>(x) * 3;
            row[target] = image.rgb[source + 2];
            row[target + 1] = image.rgb[source + 1];
            row[target + 2] = image.rgb[source];
        }
        output.write(reinterpret_cast<const char*>(row.data()),
            static_cast<std::streamsize>(row.size()));
    }

    if (!output) {
        error = "写入 BMP 文件失败";
        return false;
    }
    return true;
}
