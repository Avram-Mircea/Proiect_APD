#ifndef HUFFMAN_ALG_H
#define HUFFMAN_ALG_H

#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <fstream>
#include <bitset>
#include <unordered_map>

using namespace std;

class Node {
public:
    int data;
    unsigned char ch;
    Node* left;
    Node* right;

    Node(int data) {
        this->data = data;
        ch = 0;
        left = nullptr;
        right = nullptr;
    }
    ~Node() {}
};

class Compare {
public:
    bool operator()(Node* a, Node* b) {
        return a->data > b->data;
    }
};

vector<int> character_freq(const string& str);

void preOrder(Node* root, vector<string>& ans, string curr);
vector<string> huffmanCodes(const vector<int>& frequency);
Node* rebuildHuffmanTree(const vector<int>& frequency);

bool read_file(string& text, const string& fileName);
void write_compressed_file(const string& text, const string& filename);
void decode_file(const string& filename);

#endif