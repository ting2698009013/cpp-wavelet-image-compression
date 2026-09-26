#include <iostream>
#include <string>
#include "ImageIO.h"
#include "Huffman.h"
#include <cstdint>
#include <vector>
#include <cmath>
#undef max
#include <algorithm>
#include <fstream>
using namespace std;

template <typename T>
T clampValue(T value, T minVal, T maxVal) {
    return (value < minVal) ? minVal : (value > maxVal) ? maxVal : value;
}

class HaarWavelet {
public:
    //Haar小波变换
    static void forwardTransform(vector<double>& data, int width, int height, int levels) {
        vector<double> temp(max(width, height));
        for (int level = 0; level < levels; level++) {
            int step = 1 << level;
            int currentWidth = width / step;
            int currentHeight = height / step;

            for (int i = 0; i < currentHeight; i++) {
                for (int j = 0; j < currentWidth / 2; j++) {
                    double a = data[i * width + 2 * j];
                    double b = data[i * width + 2 * j + 1];
                    temp[j] = (a + b) / 2.0;
                    temp[currentWidth / 2 + j] = (a - b) / 2.0;
                }
                for (int j = 0; j < currentWidth; j++) {
                    data[i * width + j] = temp[j];
                }
            }

            for (int j = 0; j < currentWidth; j++) {
                for (int i = 0; i < currentHeight / 2; i++) {
                    double a = data[2 * i * width + j];
                    double b = data[(2 * i + 1) * width + j];
                    temp[i] = (a + b) / 2.0;
                    temp[currentHeight / 2 + i] = (a - b) / 2.0;
                }
                for (int i = 0; i < currentHeight; i++) {
                    data[i * width + j] = temp[i];
                }
            }
        }
    }
	// 逆Haar小波变换
    static void inverseTransform(vector<double>& data, int width, int height, int levels) {
        vector<double> temp(max(width, height));
        for (int level = levels - 1; level >= 0; level--) {
            int step = 1 << level;
            int currentWidth = width / step;
            int currentHeight = height / step;

            for (int j = 0; j < currentWidth; j++) {
                for (int i = 0; i < currentHeight / 2; i++) {
                    double avg = data[i * width + j];
                    double diff = data[(currentHeight / 2 + i) * width + j];
                    temp[2 * i] = avg + diff;
                    temp[2 * i + 1] = avg - diff;
                }
                for (int i = 0; i < currentHeight; i++) {
                    data[i * width + j] = temp[i];
                }
            }

            for (int i = 0; i < currentHeight; i++) {
                for (int j = 0; j < currentWidth / 2; j++) {
                    double avg = data[i * width + j];
                    double diff = data[i * width + currentWidth / 2 + j];
                    temp[2 * j] = avg + diff;
                    temp[2 * j + 1] = avg - diff;
                }
                for (int j = 0; j < currentWidth; j++) {
                    data[i * width + j] = temp[j];
                }
            }
        }
    }
};

// 获取数据的最大绝对值
double getMaxAbsValue(const vector<double>& data) {
    double maxVal = 0.0;
    for (double val : data) maxVal = max(maxVal, abs(val));
    return maxVal < 1e-9 ? 1e-9 : maxVal;
}

// 量化函数
vector<char> quantize(const vector<double>& data, double& scale) {
    double maxVal = getMaxAbsValue(data);
    scale = 127.0 / maxVal;
    vector<char> quantized(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        const double scaled = clampValue(round(data[i] * scale), -127.0, 127.0);
        quantized[i] = static_cast<char>(scaled);
    }
    return quantized;
}

// 反量化函数
vector<double> dequantize(const vector<char>& quantized, double scale) {
    vector<double> data(quantized.size());
    for (size_t i = 0; i < quantized.size(); ++i) {
        data[i] = static_cast<double>(quantized[i]) / scale;
    }
    return data;
}

// 直接丢弃部分高频
void discardHighFreq(vector<double>& data, int width, int height) {
    int blockSizeW = width / 2, blockSizeH = height / 2;
    int startX = width - blockSizeW, startY = height - blockSizeH;
    for (int y = startY; y < height; ++y) {
        for (int x = startX; x < width; ++x) {
            data[y * width + x] = 0.0;
        }
    }
}

//设定阈值，过滤高频
void applyThreshold(vector<double>& data, double threshold) {
    for (double& val : data) {
        if (fabs(val) < threshold) {
            val = 0.0;
        }
    }
}

// 游程编码（压缩连续零值）
vector<char> runLengthEncode(const vector<char>& data) {
    vector<char> encoded;
    size_t i = 0;
    size_t n = data.size();
    while (i < n) {
        if (data[i] != 0) {
            // 非零值直接写入
            encoded.push_back(data[i]);
            i++;
        }
        else {
            // 统计连续零的个数
            size_t runLength = 0;
            while (i < n && data[i] == 0 && runLength < 65535) { // 上限65535（避免溢出）
                runLength++;
                i++;
            }
            // 拆分长于255的连续零（按255分段）
            while (runLength > 0) {
                uint8_t segment = static_cast<uint8_t>(min(runLength, 255ULL));
                encoded.push_back(0);           // 标记：接下来是零的个数
                encoded.push_back(static_cast<char>(segment)); // 零的个数（1字节）
                runLength -= segment;
            }
        }
    }
    return encoded;
}

// 保存压缩数据（0值游程编码+Huffman编码）
bool saveCompressedData(const vector<char>& red, const vector<char>& green,
    const vector<char>& blue, int width, int height,
    double redScale, double greenScale, double blueScale,
    const char* filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) return false;

    // 写入宽、高、缩放因子
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    file.write(reinterpret_cast<const char*>(&redScale), sizeof(redScale));
    file.write(reinterpret_cast<const char*>(&greenScale), sizeof(greenScale));
    file.write(reinterpret_cast<const char*>(&blueScale), sizeof(blueScale));

    size_t totalSize = static_cast<size_t>(width) * static_cast<size_t>(height);
    file.write(reinterpret_cast<const char*>(&totalSize), sizeof(totalSize));

    // 对每个通道进行游程编码
    auto redEncoded = runLengthEncode(red);
    auto greenEncoded = runLengthEncode(green);
    auto blueEncoded = runLengthEncode(blue);


    //  Huffman编码（对RLE结果进一步压缩）
    auto redCodeMap = buildHuffmanTree(redEncoded);
    auto greenCodeMap = buildHuffmanTree(greenEncoded);
    auto blueCodeMap = buildHuffmanTree(blueEncoded);

    string redHuff = huffmanEncode(redEncoded, redCodeMap);
    string greenHuff = huffmanEncode(greenEncoded, greenCodeMap);
    string blueHuff = huffmanEncode(blueEncoded, blueCodeMap);

    // 写入每个通道的Huffman编码表和数据
    auto saveChannel = [&](const unordered_map<char, string>& codeMap,
        string& huffData) {
            saveCodeMap(codeMap, file);          // 保存编码表
            saveBinaryData(huffData, file);      // 保存二进制编码
        };

    saveChannel(redCodeMap, redHuff);
    saveChannel(greenCodeMap, greenHuff);
    saveChannel(blueCodeMap, blueHuff);

    return true;
}

// 游程解码
vector<char> runLengthDecode(const vector<char>& encoded, size_t totalSize) {
    vector<char> decoded;
    decoded.reserve(totalSize);
    size_t i = 0;
    size_t n = encoded.size();
    while (i < n) {
        if (encoded[i] != 0) {
            // 非零值直接存入
            decoded.push_back(encoded[i]);
            i++;
        }
        else {
            // 遇到0，读取后续的零个数
            if (i + 1 >= n) break; // 防止越界
            uint8_t runLength = static_cast<uint8_t>(encoded[i + 1]);
            // 填充对应数量的零
            for (uint8_t j = 0; j < runLength; j++) {
                decoded.push_back(0);
                if (decoded.size() >= totalSize) break; // 避免超过总长度
            }
            i += 2; // 跳过标记和长度
        }
    }
    // 确保长度与原始一致（补零到总长度）
    decoded.resize(totalSize, 0);
    return decoded;
}

// 加载压缩数据并解码
bool loadCompressedData(vector<char>& red, vector<char>& green,
    vector<char>& blue, int& width, int& height,
    double& redScale, double& greenScale, double& blueScale,
    const char* filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "无法打开文件: " << filename << endl;
        return false;
    }

    // 读取文件头信息
    if (!file.read(reinterpret_cast<char*>(&width), sizeof(width)) ||
        !file.read(reinterpret_cast<char*>(&height), sizeof(height)) ||
        !file.read(reinterpret_cast<char*>(&redScale), sizeof(redScale)) ||
        !file.read(reinterpret_cast<char*>(&greenScale), sizeof(greenScale)) ||
        !file.read(reinterpret_cast<char*>(&blueScale), sizeof(blueScale))) {
        cout << "错误：压缩文件头不完整" << endl;
        return false;
    }

    size_t totalSize;
    if (!file.read(reinterpret_cast<char*>(&totalSize), sizeof(totalSize)) ||
        width <= 0 || height <= 0 ||
        totalSize != static_cast<size_t>(width) * static_cast<size_t>(height)) {
        cout << "错误：压缩文件中的图像尺寸无效" << endl;
        return false;
    }

    // 读取每个通道的Huffman数据
    auto loadChannel = [&]() -> vector<char> {
        HuffmanNode* root = loadCodeMap(file);
        if (!root) {
            cout << "错误：无法加载Huffman编码表" << endl;
            return {};
        }
        string binaryStr = loadBinaryData(file);
        if (binaryStr.empty()) {
            cout << "错误：无法加载二进制数据" << endl;
            freeHuffmanTree(root);
            return {};
        }

        vector<char> rleData = huffmanDecode(binaryStr, root);
        freeHuffmanTree(root);
        if (rleData.empty()) {
            cout << "错误：Huffman解码失败" << endl;
            return {};
        }
		cout << "Huffman解码成功，大小: " << rleData.size() << endl;
        return runLengthDecode(rleData, totalSize);
        };

    red = loadChannel();
    green = loadChannel();
    blue = loadChannel();

    file.close();
    return red.size() == totalSize && green.size() == totalSize && blue.size() == totalSize;
}

// 图像压缩主流程
bool compressImage(const char* inputFile, const char* outputFile) {
    Image image;
    string error;
    if (!loadBmp(inputFile, image, error)) {
        cerr << "无法读取输入文件: " << inputFile << "（" << error << "）" << endl;
        return false;
    }

    const int width = image.width;
    const int height = image.height;
    if (width % 8 != 0 || height % 8 != 0) {
        cerr << "图像宽高必须能被 8 整除，以执行三级 Haar 小波变换。" << endl;
        return false;
    }


    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    vector<double> redChannel(pixelCount), greenChannel(pixelCount), blueChannel(pixelCount);
    for (size_t idx = 0; idx < redChannel.size(); ++idx) {
        redChannel[idx] = image.rgb[idx * 3];
        greenChannel[idx] = image.rgb[idx * 3 + 1];
        blueChannel[idx] = image.rgb[idx * 3 + 2];
    }

    int waveletLevels = 3;
    HaarWavelet::forwardTransform(redChannel, width, height, waveletLevels);
    HaarWavelet::forwardTransform(greenChannel, width, height, waveletLevels);
    HaarWavelet::forwardTransform(blueChannel, width, height, waveletLevels);

    double threshold = 1.5;
    applyThreshold(redChannel, threshold);
    applyThreshold(greenChannel, threshold);
    applyThreshold(blueChannel, threshold);

    discardHighFreq(redChannel, width, height);
    discardHighFreq(greenChannel, width, height);
    discardHighFreq(blueChannel, width, height);

    double redScale, greenScale, blueScale;
    auto redQuantized = quantize(redChannel, redScale);
    auto greenQuantized = quantize(greenChannel, greenScale);
    auto blueQuantized = quantize(blueChannel, blueScale);

    if (saveCompressedData(redQuantized, greenQuantized, blueQuantized, width, height,
        redScale, greenScale, blueScale, outputFile)) {
        printf("压缩成功！保存为: %s\n", outputFile);
        return true;
    }
    else {
        printf("压缩失败！\n");
        return false;
    }
}

// 图像解压主流程
bool readImage(const char* compressedFile, const char* outputFile) {
    vector<char> redQuantized, greenQuantized, blueQuantized;
    int width, height;
    double redScale, greenScale, blueScale;

    if (!loadCompressedData(redQuantized, greenQuantized, blueQuantized, width, height,
        redScale, greenScale, blueScale, compressedFile)) {
        printf("无法读取文件: %s\n", compressedFile);
        return false;
    }

    vector<double> redChannel = dequantize(redQuantized, redScale);
    vector<double> greenChannel = dequantize(greenQuantized, greenScale);
    vector<double> blueChannel = dequantize(blueQuantized, blueScale);

    int waveletLevels = 3;
    HaarWavelet::inverseTransform(redChannel, width, height, waveletLevels);
    HaarWavelet::inverseTransform(greenChannel, width, height, waveletLevels);
    HaarWavelet::inverseTransform(blueChannel, width, height, waveletLevels);

    Image output;
    output.width = width;
    output.height = height;
    output.rgb.resize(static_cast<size_t>(width) * height * 3);
    for (int i = 0; i < width * height; i++) {
        output.rgb[i * 3] = static_cast<uint8_t>(clampValue(redChannel[i], 0.0, 255.0));
        output.rgb[i * 3 + 1] = static_cast<uint8_t>(clampValue(greenChannel[i], 0.0, 255.0));
        output.rgb[i * 3 + 2] = static_cast<uint8_t>(clampValue(blueChannel[i], 0.0, 255.0));
    }

    string error;
    if (!saveBmp(outputFile, output, error)) {
        cerr << "无法写入解压图像: " << outputFile << "（" << error << "）" << endl;
        return false;
    }
    cout << "解压成功！保存为: " << outputFile << endl;
    return true;
}
string replaceExtension(const string& inputFile, const string& extension) {
    size_t dotPos = inputFile.find_last_of('.');
    return (dotPos != string::npos) ? inputFile.substr(0, dotPos) + extension : inputFile + extension;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        printf("用法:\n");
        printf("  %s -compress <输入.bmp> [输出.dat]\n", argv[0]);
        printf("  %s -decompress <输入.dat> [输出.bmp]\n", argv[0]);
        return 1;
    }

    string cmd = argv[1], path = argv[2];
    if (cmd == "-compress") {
        const string output = argc == 4 ? argv[3] : replaceExtension(path, ".dat");
        return compressImage(path.c_str(), output.c_str()) ? 0 : 1;
    }
    else if (cmd == "-decompress") {
        const string output = argc == 4 ? argv[3] : replaceExtension(path, "_decoded.bmp");
        return readImage(path.c_str(), output.c_str()) ? 0 : 1;
    }
    else {
        printf("未知命令\n");
        return 1;
    }
}
