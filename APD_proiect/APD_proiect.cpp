#include "huffman_alg.h"
#include <chrono>

using namespace std;

int main() {
    string text, fileName;
    char option;

    while (true) {
        cout << "1. Compress file\n2. Compress manual text\n3. Decompress file\n4. Exit program\nOption: ";
        cin >> option;

        switch (option) {
        case '1':
            cout << "File name: ";
            cin >> fileName;

            text.clear();
            if (read_file(text, fileName + ".txt")) {
                auto start = chrono::high_resolution_clock::now();

                vector<int> freq = character_freq(text);
                vector<string> codes = huffmanCodes(freq);

                for (auto& c : codes)
                    cout << c << endl;

                write_compressed_file(text, fileName + ".bin");

                auto end = chrono::high_resolution_clock::now();
                cout << "Timp executie: "
                    << chrono::duration_cast<chrono::milliseconds>(end - start).count()
                    << " ms" << endl;
            }
            else {
                cout << "File not found!\n";
            }
            break;

        case '2':
            cout << "Enter text: ";
            cin.ignore();
            getline(cin, text);

            {
                auto start = chrono::high_resolution_clock::now();
                vector<int> freq = character_freq(text);
                vector<string> codes = huffmanCodes(freq);

                for (auto& c : codes)
                    cout << c << endl;

                write_compressed_file(text, "manual.bin");

                auto end = chrono::high_resolution_clock::now();
                cout << "Timp executie: "
                    << chrono::duration_cast<chrono::milliseconds>(end - start).count()
                    << " ms" << endl;
            }
            break;

        case '3':
            cout << "File name: ";
            cin >> fileName;
            decode_file(fileName + ".bin");
            break;

        case '4':
            return 0;

        default:
            cout << "Invalid option\n";
            break;
        }

        system("pause");
        system("cls");
    }
}