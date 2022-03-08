#ifndef Clock_hpp
#define Clock_hpp

#include <thread>
#include <iostream>

#endif /* Clock_hpp */

using namespace std;

// Creating a Clock Singleton to Count Time, Running on a Thread
class Clock {
    private: 
        // Time in ms and Single Clock Instance
        int time;
        static Clock *instance;

    public:
        // Constructor and Destructor
        Clock();
        ~Clock();

        // Return Clock Instance
        static Clock *getInstance();

        // Update and Return Current Clock Time 
        int getTime();
        void setTime();

        // Clock Thread Methods
        thread startThread();
        void run();
        void terminate();
};