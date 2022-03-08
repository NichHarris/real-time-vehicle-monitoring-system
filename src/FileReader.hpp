#ifndef FileReader_hpp
#define FileReader_hpp

#include <string>
#include <fstream>
#include <iostream>
#include <stdio.h>

#endif /* FileReader_hpp */

using namespace std;

class FileReader {
    private:
        // Data Read and File Reader
        double data;
        fstream reader;

    public:
        // Constructor and Destructor
        FileReader(string fileName);
        ~FileReader();
    
        // Update and Return Read data 
        void setData();
        double getData();
    
        //Print Read Data
        void print();
};