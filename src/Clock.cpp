#include <thread>
#include <iostream>
#include <ctime>

using namespace std;

#include "Clock.hpp"

// Constructor - Initializes Singleton Clock to Count Time, Running on Thread
Clock::Clock() {
    time = 0;
    instance = new Clock();
    cout << "New clock object created" << endl;
}

// Destructor
Clock::~Clock() {
    cout << "Clock Object is Deleted!" << endl;
}

Clock* Clock::getInstance() {
    return instance;
}

// Get Current Clock Time
int Clock::getTime() {
    return time;
}

// Increment Current Clock
void Clock::setTime() {
    time = time + 10;
}


// Thread Methods
thread Clock::startThread() {
    thread clockThread = thread([=] { run(); });
    //handle = clockThread.native_handle();
    return clockThread;
}

void Clock::run(){
    while(true) {
        // Update Clock time here...
        time = clock();
    }
}

void Clock::terminate(){
    //TerminateThread(handle, 0);
}

