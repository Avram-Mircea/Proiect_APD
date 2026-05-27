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

vector<int> character_freq(const string& str) {
    vector<int> freq(256, 0);
    for (unsigned char ch : str)
        freq[ch]++;
    return freq;
}

void preOrder(Node* root, vector<string>& ans, string curr) {
    if (!root) return;

    if (!root->left && !root->right) {
        ans.push_back(string(1, root->ch) + " " + curr);
        return;
    }

    preOrder(root->left, ans, curr + '0');
    preOrder(root->right, ans, curr + '1');
}

vector<string> huffmanCodes(const vector<int>& frequency) {
    priority_queue<Node*, vector<Node*>, Compare> pq;

    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            Node* tmp = new Node(frequency[i]);
            tmp->ch = (unsigned char)i;
            pq.push(tmp);
        }
    }

    while (pq.size() >= 2) {
        Node* l = pq.top(); pq.pop();
        Node* r = pq.top(); pq.pop();

        Node* newNode = new Node(l->data + r->data);
        newNode->left = l;
        newNode->right = r;

        pq.push(newNode);
    }

    Node* root = pq.top();
    vector<string> ans;
    preOrder(root, ans, "");
    return ans;
}

Node* rebuildHuffmanTree(const vector<int>& frequency) {
    priority_queue<Node*, vector<Node*>, Compare> pq;

    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            Node* tmp = new Node(frequency[i]);
            tmp->ch = (unsigned char)i;
            pq.push(tmp);
        }
    }

    while (pq.size() >= 2) {
        Node* l = pq.top(); pq.pop();
        Node* r = pq.top(); pq.pop();

        Node* newNode = new Node(l->data + r->data);
        newNode->left = l;
        newNode->right = r;
        pq.push(newNode);
    }

    return pq.top();
}

bool read_file(string& text, const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        text += line + '\n';
    }

    file.close();
    return true;
}

vector<char> encode_chunk(const string& text, const unordered_map<unsigned char, string>& map) {
    vector<char> bits;
    for (unsigned char c : text) {
        for (char b : map.at(c))
            bits.push_back(b == '1' ? 1 : 0);
    }
    return bits;
}

void write_compressed_file(const string& text, const string& filename) {
    ofstream out(filename, ios::binary);
    if (!out.is_open()) return;

    vector<int> frequency = character_freq(text);

    unsigned char count = 0;
    for (int f : frequency)
        if (f > 0) count++;
    out.write((char*)&count, 1);

    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            unsigned char ch = i;
            out.write((char*)&ch, 1);
            out.write((char*)&frequency[i], sizeof(int));
        }
    }

    vector<string> codes = huffmanCodes(frequency);
    unordered_map<unsigned char, string> map;
    for (auto& c : codes)
        map[(unsigned char)c[0]] = c.substr(2);

    vector<bool> bits;
    for (unsigned char c : text) {
        for (char b : map[c])
            bits.push_back(b == '1');
    }

    int textSize = text.size();
    out.write((char*)&textSize, sizeof(textSize));

    while (bits.size() % 8 != 0)
        bits.push_back(0);

    for (size_t i = 0; i < bits.size(); i += 8) {
        bitset<8> b;
        for (int j = 0; j < 8; j++)
            b[7 - j] = bits[i + j];

        unsigned char byte = b.to_ulong();
        out.write((char*)&byte, 1);
    }

    out.close();
}

void decode_file(const string& filename) {
    ifstream in(filename, ios::binary);

    unsigned char count;
    in.read((char*)&count, 1);
    vector<int> freq(256, 0);

    for (int i = 0; i < count; i++) {
        unsigned char ch;
        int f;
        in.read((char*)&ch, 1);
        in.read((char*)&f, sizeof(int));
        freq[ch] = f;
    }

    Node* root = rebuildHuffmanTree(freq);

    int textSize;
    in.read((char*)&textSize, sizeof(textSize));

    vector<bool> bits;
    unsigned char byte;
    while (in.read((char*)&byte, 1)) {
        bitset<8> b(byte);
        for (int i = 7; i >= 0; i--)
            bits.push_back(b[i]);
    }

    string result;
    Node* curr = root;
    for (bool bit : bits) {
        curr = bit ? curr->right : curr->left;
        if (!curr->left && !curr->right) {
            result += curr->ch;
            curr = root;
            if (result.size() == textSize) break;
        }
    }

    cout << "Decoded text:\n" << result << endl;
}

bool open_file(const string& filename)
{
    ifstream in(filename, ios::binary);
    return in.is_open();
}

#endif