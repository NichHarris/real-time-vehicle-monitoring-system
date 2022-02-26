#include <iostream>

using namespace std;

// Creating a Clock Singleton to Count Time, Running on a Thread
class Clock {

    // Create Singleton Clock Instance
    static Clock *instance;

    // Store Time in ms
    int time;

    // Default Constructor - Initializes Clock to Time 0ms
    Clock() {
        time = 0;
        instance = new Clock();
    }

    public:
        // Return Clock Instance
        static Clock *getInstance() {
            return instance;
        }

        // Get Current Clock Time
        int getTime() {
            return time;
        }

        // Increment Current Clock
        void setTime() {
            time = time + 10;
        }
};