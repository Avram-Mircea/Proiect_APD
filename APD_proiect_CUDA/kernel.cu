#include <iostream>
#include <vector>
#include <string>
#include <cuda_runtime.h>
#include <chrono>
#include <fstream>

#include "huffman_alg.h"

using namespace std;

#define ALPHABET 256
#define BLOCK_SIZE 256

// ================= GPU KERNEL =================
__global__ void freqKernelOptimized(const char* text, int n, int* globalFreq)
{
    __shared__ int localFreq[ALPHABET];

    int tid = threadIdx.x;

    for (int i = tid; i < ALPHABET; i += blockDim.x)
        localFreq[i] = 0;

    __syncthreads();

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = gridDim.x * blockDim.x;

    for (int i = idx; i < n; i += stride)
    {
        atomicAdd(&localFreq[(unsigned char)text[i]], 1);
    }

    __syncthreads();

    for (int i = tid; i < ALPHABET; i += blockDim.x)
    {
        atomicAdd(&globalFreq[i], localFreq[i]);
    }
}

// ================= CPU FREQ =================
vector<int> cpuFreq(const string& text)
{
    vector<int> freq(ALPHABET, 0);

    for (unsigned char c : text)
        freq[c]++;

    return freq;
}

// ================= FAST ENCODING (FIX MAJOR BOTTLENECK) =================
vector<unsigned char> fastEncode(
    const string& text,
    unordered_map<unsigned char, string>& codeMap)
{
    vector<unsigned char> bytes;
    unsigned char current = 0;
    int bitCount = 0;

    for (unsigned char c : text)
    {
        const string& code = codeMap[c];

        for (char bitChar : code)
        {
            int bit = bitChar - '0';

            current |= (bit << (7 - bitCount));
            bitCount++;

            if (bitCount == 8)
            {
                bytes.push_back(current);
                current = 0;
                bitCount = 0;
            }
        }
    }

    if (bitCount > 0)
        bytes.push_back(current);

    return bytes;
}

// ================= MAIN =================
int main()
{
    string text, fileName;

    cout << "File name (fara extensie): ";
    cin >> fileName;

    if (!read_file(text, fileName + ".txt"))
    {
        cout << "File not found!\n";
        return 0;
    }

    int n = text.size();

    // ================= GPU SETUP =================
    char* d_text;
    int* d_freq;

    vector<int> h_freq(ALPHABET, 0);

    cudaMalloc(&d_text, n);
    cudaMalloc(&d_freq, ALPHABET * sizeof(int));

    cudaMemcpy(d_text, text.data(), n, cudaMemcpyHostToDevice);
    cudaMemset(d_freq, 0, ALPHABET * sizeof(int));

    dim3 threads(BLOCK_SIZE);
    dim3 blocks(256);

    // ================= GPU =================
    auto start_gpu = chrono::high_resolution_clock::now();

    freqKernelOptimized << <blocks, threads >> > (d_text, n, d_freq);
    cudaDeviceSynchronize();

    auto end_gpu = chrono::high_resolution_clock::now();

    cout << "GPU freq time: "
        << chrono::duration_cast<chrono::microseconds>(end_gpu - start_gpu).count()
        << " us\n";

    cudaMemcpy(h_freq.data(), d_freq, ALPHABET * sizeof(int), cudaMemcpyDeviceToHost);

    // ================= CPU =================
    auto start_cpu = chrono::high_resolution_clock::now();

    vector<int> cpu_freq = cpuFreq(text);

    auto end_cpu = chrono::high_resolution_clock::now();

    cout << "CPU freq time: "
        << chrono::duration_cast<chrono::microseconds>(end_cpu - start_cpu).count()
        << " us\n";

    // ================= HUFFMAN BUILD =================
    auto start_huff = chrono::high_resolution_clock::now();

    vector<string> codes = huffmanCodes(h_freq);

    unordered_map<unsigned char, string> codeMap;
    for (auto& c : codes)
        codeMap[(unsigned char)c[0]] = c.substr(2);

    auto huff_build_end = chrono::high_resolution_clock::now();

    // ================= FAST ENCODING =================
    vector<unsigned char> encoded = fastEncode(text, codeMap);

    auto end_huff = chrono::high_resolution_clock::now();

    cout << "Huffman CPU build time: "
        << chrono::duration_cast<chrono::milliseconds>(huff_build_end - start_huff).count()
        << " ms\n";

    cout << "Huffman encoding time: "
        << chrono::duration_cast<chrono::milliseconds>(end_huff - huff_build_end).count()
        << " ms\n";

    // ================= FILE WRITE (FAST) =================
    ofstream out(fileName + ".bin", ios::binary);
    out.write((char*)encoded.data(), encoded.size());
    out.close();

    cudaFree(d_text);
    cudaFree(d_freq);

    return 0;
}