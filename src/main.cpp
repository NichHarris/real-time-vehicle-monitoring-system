#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include "FileReader.hpp"

using namespace std;

// Create Clock
//atomic<int> CLOCK{0};

int main(){
    FileReader fr("./Data/Current_Gear.csv");

    cout << "Nice" << endl;



    // Trying to Read from File Here ...
    // ifstream current_gear_file ("./Data/Current_Gear.csv");
    // string current_gear_output;
    // int j = 0;

    // if(current_gear_file.is_open()) {
    //     while(getline (current_gear_file, current_gear_output)) {
    //         if ((j - 1) % 5 == 0) {
    //             cout << current_gear_output << endl;
    //         }

    //         if(j > 25) {
    //             break;
    //         }

    //         j++;
    //     }
    //     //current_gear_file >> current_gear_output;
    //     //cout << current_gear_output;
    // }
}