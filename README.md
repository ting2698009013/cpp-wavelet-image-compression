# C++ 小波图像压缩实验

[![build](https://github.com/ting2698009013/cpp-wavelet-image-compression/actions/workflows/build.yml/badge.svg)](https://github.com/ting2698009013/cpp-wavelet-image-compression/actions/workflows/build.yml)

一个从零实现的有损图像压缩课程项目。它先对 RGB 三个通道执行二维 Haar 小波变换，再通过高频裁剪、阈值过滤、量化、零游程编码和 Huffman 编码生成自定义 `.dat` 文件。

解压过程按相反顺序恢复数据，并输出 BMP 图像。

## 压缩流程

```text
BMP → RGB 通道 → Haar 小波变换 → 高频过滤 → 量化
    → 零游程编码 → Huffman 编码 → .dat
```

解压流程：

```text
.dat → Huffman 解码 → 游程解码 → 反量化
     → 逆 Haar 小波变换 → BMP
```

## 特点

- 二维、多层 Haar 小波正变换与逆变换
- RGB 三通道独立处理
- 高频系数阈值过滤与局部裁剪
- 8 位量化
- 针对连续零值的游程编码
- 自建 Huffman 树、码表序列化与二进制存储
- 不依赖第三方图像库
- 支持未压缩的 24 位和 32 位 BMP 输入

当前固定使用三级小波变换，因此输入图片的宽和高必须都能被 8 整除。

## 构建

```bash
cmake -S . -B build
cmake --build build --config Release
```

也可以直接使用支持 C++17 的编译器：

```bash
g++ -std=c++17 -O2 src/main.cpp src/Huffman.cpp src/ImageIO.cpp -o image_compression
```

## 使用

压缩：

```bash
image_compression -compress input.bmp output.dat
```

解压：

```bash
image_compression -decompress output.dat restored.bmp
```

省略输出路径时，程序会自动生成文件名。

## 验证

仓库包含一个不依赖外部图片的端到端冒烟测试。它会生成一张 16×16 BMP，完成压缩与解压，并检查输出格式和错误返回码：

```bash
python tests/smoke_test.py build/image_compression
```

每次推送和 Pull Request 都会在 GitHub Actions 的 Linux 环境中重新构建并运行该测试。

## 整理说明

原课程工程依赖老师提供的 Windows/EasyX 图片读写框架。公开版本没有复制该课程框架，而是重新实现了独立的 BMP 读写模块，因此可以仅使用标准 C++17 构建。原测试图片也未包含在仓库中。

该压缩格式用于算法学习，不追求与 JPEG、WebP 等成熟格式竞争，也不保证跨平台二进制格式长期兼容。
