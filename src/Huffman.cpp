#include "Huffman.h"
#include <queue>
#include <bitset>
#include <iostream>
#include <limits>
#include <memory>

using namespace std;

// HuffmanNode 构造函数
HuffmanNode::HuffmanNode(char d, int f) : data(d), freq(f), left(nullptr), right(nullptr) {}

// 比较器用于优先队列
struct CompareNode {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};

// 生成 Huffman 编码的辅助函数
void generateCodes(HuffmanNode* root, const string& code, unordered_map<char, string>& codeMap) {
    if (!root) return;

    if (!root->left && !root->right) {
        codeMap[root->data] = code.empty() ? "0" : code;
        return;
    }

    generateCodes(root->left, code + "0", codeMap);
    generateCodes(root->right, code + "1", codeMap);
}

// 构建 Huffman 树
unordered_map<char, string> buildHuffmanTree(const vector<char>& data) {
    if (data.empty()) return {};

    // 统计频率
    unordered_map<char, int> freqMap;
    for (char c : data) {
        freqMap[c]++;
    }

    // 创建优先队列
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, CompareNode> pq;

    for (const auto& pair : freqMap) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    // 构建 Huffman 树
    while (pq.size() > 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();

        HuffmanNode* parent = new HuffmanNode('\0', left->freq + right->freq);
        parent->left = left;
        parent->right = right;

        pq.push(parent);
    }

    // 生成编码表
    unordered_map<char, string> codeMap;
    if (!pq.empty()) {
        HuffmanNode* root = pq.top();
        generateCodes(root, "", codeMap);
        freeHuffmanTree(root);
    }

    return codeMap;
}

// Huffman 编码
string huffmanEncode(const vector<char>& data, const unordered_map<char, string>& codeMap) {
    if (data.empty() || codeMap.empty()) return "";

    string encoded;
    for (char c : data) {
        auto it = codeMap.find(c);
        if (it != codeMap.end()) {
            encoded += it->second;
        }
    }
    return encoded;
}

// Huffman 解码
vector<char> huffmanDecode(const string& binaryStr, HuffmanNode* root) {
    vector<char> decoded;
    if (!root || binaryStr.empty()) return decoded;

    HuffmanNode* current = root;

    for (char bit : binaryStr) {
        if (bit == '0') {
            current = current->left;
        }
        else {
            current = current->right;
        }

        if (!current) break;

        // 到达叶子节点
        if (!current->left && !current->right) {
            decoded.push_back(current->data);
            current = root;
        }
    }

    return decoded;
}

// 保存编码表
void saveCodeMap(const unordered_map<char, string>& codeMap, ofstream& file) {
    int size = static_cast<int>(codeMap.size());
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (const auto& pair : codeMap) {
        char c = pair.first;
        const string& code = pair.second;
        int codeLen = static_cast<int>(code.length());

        file.write(&c, sizeof(c));
        file.write(reinterpret_cast<const char*>(&codeLen), sizeof(codeLen));
        file.write(code.c_str(), codeLen);
    }
}

// 加载编码表
HuffmanNode* loadCodeMap(ifstream& file) {
    int size;
    if (!file.read(reinterpret_cast<char*>(&size), sizeof(size)) || size <= 0 || size > 256) {
        return nullptr; // 读取size失败或无效
    }

    unordered_map<char, string> codeMap;
    for (int i = 0; i < size; i++) {
        char c;
        int codeLen;

        if (!file.read(&c, sizeof(c))) return nullptr;
        if (!file.read(reinterpret_cast<char*>(&codeLen), sizeof(codeLen)) ||
            codeLen <= 0 || codeLen > 256) return nullptr;

        string code(codeLen, ' ');
        if (codeLen > 0 && !file.read(&code[0], codeLen)) {
            return nullptr;
        }

        codeMap[c] = code;
    }
    // 重建 Huffman 树
    HuffmanNode* root = new HuffmanNode('\0', 0);

    for (const auto& pair : codeMap) {
        HuffmanNode* current = root;
        for (char bit : pair.second) {
            if (bit == '0') {
                if (!current->left) {
                    current->left = new HuffmanNode('\0', 0);
                }
                current = current->left;
            }
            else {
                if (!current->right) {
                    current->right = new HuffmanNode('\0', 0);
                }
                current = current->right;
            }
        }

        current->data = pair.first;
    }

    return root;
}

void saveBinaryData(const string& binaryStr, ofstream& file) {
    int padding = (8 - (binaryStr.length() % 8)) % 8;
    size_t byteCount = (binaryStr.length() + padding) / 8; // 总字节数

    // 先写入字节数（关键：标记当前通道二进制数据的长度）
    file.write(reinterpret_cast<const char*>(&byteCount), sizeof(byteCount));
    file.write(reinterpret_cast<const char*>(&padding), sizeof(padding));

    // 写入字节数据
    for (size_t i = 0; i < binaryStr.length(); i += 8) {
        bitset<8> bits;
        for (int j = 0; j < 8; j++) {
            if (i + j < binaryStr.length()) {
                bits[7 - j] = (binaryStr[i + j] == '1');
            }
        }
        char byte = static_cast<char>(bits.to_ulong());
        file.write(&byte, sizeof(byte));
    }
}

string loadBinaryData(ifstream& file) {
    size_t byteCount;
    int padding;

    // 先读取字节数和填充位（关键：控制读取边界）
    if (!file.read(reinterpret_cast<char*>(&byteCount), sizeof(byteCount))) {
        return ""; // 读取失败
    }
    if (!file.read(reinterpret_cast<char*>(&padding), sizeof(padding))) {
        return "";
    }
    if (padding < 0 || padding > 7 ||
        byteCount > numeric_limits<size_t>::max() / 8 ||
        static_cast<size_t>(padding) > byteCount * 8) {
        return "";
    }

    string binaryStr;
    binaryStr.reserve(byteCount * 8 - padding); // 预留空间

    // 只读取指定数量的字节（当前通道的二进制数据）
    for (size_t i = 0; i < byteCount; i++) {
        char byte;
        if (!file.read(&byte, sizeof(byte))) {
            return ""; // 读取失败
        }
        bitset<8> bits(static_cast<unsigned char>(byte));
        for (int j = 7; j >= 0; j--) {
            binaryStr += bits[j] ? '1' : '0';
        }
    }

    // 移除填充位
    const size_t paddingSize = static_cast<size_t>(padding);
    if (padding > 0 && paddingSize <= binaryStr.length()) {
        binaryStr.resize(binaryStr.length() - paddingSize);
    }

    return binaryStr;
}
// 释放 Huffman 树
void freeHuffmanTree(HuffmanNode* root) {
    if (!root) return;

    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    delete root;
}
