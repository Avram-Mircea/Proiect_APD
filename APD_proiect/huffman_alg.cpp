#include "huffman_alg.h"

vector<int> character_freq(const string& str) {
    vector<int> freq(256, 0);
    for (unsigned char ch : str)
        freq[ch]++;
    return freq;
}

//to see the path
void preOrder(Node* root, vector<string>& ans, string curr) {
    if (!root) return;

    if (!root->left && !root->right) {
        ans.push_back(string(1, root->ch) + " " + curr);
        return;
    }

    preOrder(root->left, ans, curr + '0');
    preOrder(root->right, ans, curr + '1');
}

//used to get the character and Huffman cod
vector<string> huffmanCodes(const vector<int>& frequency) {
    priority_queue<Node*, vector<Node*>, Compare> pq;

    //add nodes for characters
    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            Node* tmp = new Node(frequency[i]);
            tmp->ch = (unsigned char)i;
            pq.push(tmp);
        }
    }

    //we combine nodes until we get the root node
    //and in this way, we create the tree
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

//used to get the root of the Huffman tree
Node* rebuildHuffmanTree(const vector<int>& frequency) {
    priority_queue<Node*, vector<Node*>, Compare> pq;

    //add nodes for characters
    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            Node* tmp = new Node(frequency[i]);
            tmp->ch = (unsigned char)i;
            pq.push(tmp);
        }
    }

    //we combine nodes until we get the root node
    //and in this way, we create the tree
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

void write_compressed_file(const string& text, const string& filename) {
    ofstream out(filename, ios::binary);
    if (!out.is_open()) return;

    vector<int> frequency = character_freq(text);

    //number of distinct characters
    unsigned char count = 0;
    for (int f : frequency)
        if (f > 0) count++;
    out.write((char*)&count, 1);

    //we write the characters and frequencies
    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            unsigned char ch = i;
            out.write((char*)&ch, 1);
            out.write((char*)&frequency[i], sizeof(int));
        }
    }

    //we generate Huffman codes
    vector<string> codes = huffmanCodes(frequency);
    unordered_map<unsigned char, string> map;
    for (auto& c : codes)
        map[(unsigned char)c[0]] = c.substr(2);

    //we encode the text
    vector<bool> bits;
    for (unsigned char c : text) {
        for (char b : map[c])
            bits.push_back(b == '1');
    }

    //we write the length of the original text
    int textSize = text.size();
    out.write((char*)&textSize, sizeof(textSize));

    //we add 0 if we don't have enough bits to create a byte
    while (bits.size() % 8 != 0)
        bits.push_back(0);

    //we write the new cod
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
    if (!in.is_open()) return;

    //read characters and frequencies
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

    //rebuild the tree
    Node* root = rebuildHuffmanTree(freq);

    //read the original text size
    int textSize;
    in.read((char*)&textSize, sizeof(textSize));

    //read bytes
    vector<bool> bits;
    unsigned char byte;
    while (in.read((char*)&byte, 1)) {
        bitset<8> b(byte);
        for (int i = 7; i >= 0; i--)
            bits.push_back(b[i]);
    }

    //decode
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