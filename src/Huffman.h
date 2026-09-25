#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <memory>

struct HuffmanNode {
    char data;
    int freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(char d, int f);
};

// 公共函数声明
std::unordered_map<char, std::string> buildHuffmanTree(const std::vector<char>& data);
std::string huffmanEncode(const std::vector<char>& data, const std::unordered_map<char, std::string>& codeMap);
std::vector<char> huffmanDecode(const std::string& binaryStr, HuffmanNode* root);
void saveCodeMap(const std::unordered_map<char, std::string>& codeMap, std::ofstream& file);
HuffmanNode* loadCodeMap(std::ifstream& file);
void saveBinaryData(const std::string& binaryStr, std::ofstream& file);
std::string loadBinaryData(std::ifstream& file);
void freeHuffmanTree(HuffmanNode* root);

#endif