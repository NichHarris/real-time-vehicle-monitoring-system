#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#include "FileReader.hpp"

// Constructor - Initializes Reader and Read Data
FileReader::FileReader(string fileName) {
    data = 0;
    reader.open(fileName);
}

// Destructor
FileReader::~FileReader() {
    cout << "FileReader Object is Deleted!" << endl;
}

// Read Next Data 
void FileReader::setData() {
    if(reader.is_open()) {
        string currData; 
        int currLine = -1;

        while(getline (reader, currData)) {
            // Discard First Line and Read Every Fifth Value
            if (currLine % 5 == 0) {
                cout << currData << endl;
            }

            // TODO: Replace 30 with Current Clock Time * 5
            if (currLine > 30) {
                // Convert String to Double 
                data = stod(currData);

                cout << "Final Data: " << data << endl;
                break;
            }

            // Increment Line Count
            currLine++;
        }
    } else {
        cout << "File is Not Open and Could Not Be Read!" << endl; 
    }
}

double FileReader::getData() {
    return data;
}

// Print Information about Current File Reader
void FileReader::print() {
    string fileName = "Test Name";
    string currTime = "Sample Time";

    cout << "File: " << fileName << endl;
    cout << "Current Data: " << data << endl;
    cout << "Clock Time: " << currTime << endl;
}