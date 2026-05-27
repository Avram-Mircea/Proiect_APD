#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <bitset>
#include "huffman_alg.h"

using namespace std;


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double start, end;
    string text, fileName;
    vector<int> global_freq(256, 0);

    if (rank == 0) {
        cout << "File name: ";
        cin >> fileName;

        if (!read_file(text, fileName + ".txt")) {
            cout << "File not found!\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    int textSize;
    if (rank == 0) textSize = text.size();
    MPI_Bcast(&textSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    text.resize(textSize);

    int chunkSize = textSize / size;
    int remainder = textSize % size;

    vector<int> sendCounts(size), displs(size);

    if (rank == 0) {
        int offset = 0;
        for (int i = 0; i < size; i++) {
            sendCounts[i] = chunkSize + (i < remainder ? 1 : 0);
            displs[i] = offset;
            offset += sendCounts[i];
        }
    }

    int localSize;
    MPI_Scatter(sendCounts.data(), 1, MPI_INT, &localSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    string localText(localSize, ' ');

    MPI_Scatterv(text.data(), sendCounts.data(), displs.data(), MPI_CHAR,
        &localText[0], localSize, MPI_CHAR,
        0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    vector<int> local_freq = character_freq(localText);

    MPI_Reduce(local_freq.data(), global_freq.data(), 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    vector<string> codes;
    unordered_map<unsigned char, string> codeMap;

    if (rank == 0) {
        codes = huffmanCodes(global_freq);
        for (auto& c : codes)
            codeMap[(unsigned char)c[0]] = c.substr(2);
    }

    MPI_Bcast(global_freq.data(), 256, MPI_INT, 0, MPI_COMM_WORLD);

    vector<char> local_bits = encode_chunk(localText, codeMap);

    int localLen = local_bits.size();

    vector<int> recvCounts(size);
    MPI_Gather(&localLen, 1, MPI_INT,
        recvCounts.data(), 1, MPI_INT,
        0, MPI_COMM_WORLD);

    vector<int> recvDispls(size);
    int totalBits = 0;

    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            recvDispls[i] = totalBits;
            totalBits += recvCounts[i];
        }
    }

    vector<char> allBits(totalBits);

    MPI_Gatherv(local_bits.data(), localLen, MPI_CHAR,
        allBits.data(), recvCounts.data(), recvDispls.data(),
        MPI_CHAR, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    if (rank == 0) {
        ofstream out(fileName + ".bin", ios::binary);

        unsigned char count = 0;
        for (int f : global_freq)
            if (f > 0) count++;

        out.write((char*)&count, 1);

        for (int i = 0; i < 256; i++) {
            if (global_freq[i] > 0) {
                unsigned char ch = i;
                out.write((char*)&ch, 1);
                out.write((char*)&global_freq[i], sizeof(int));
            }
        }

        out.write((char*)&textSize, sizeof(textSize));

        while (allBits.size() % 8 != 0)
            allBits.push_back(0);

        for (size_t i = 0; i < allBits.size(); i += 8) {
            bitset<8> b;
            for (int j = 0; j < 8; j++)
                b[7 - j] = allBits[i + j];

            unsigned char byte = b.to_ulong();
            out.write((char*)&byte, 1);
        }

        out.close();
        cout << "Timp executie: " << (end - start) * 1000 << " ms\n";
    }

    MPI_Finalize();
    return 0;
}