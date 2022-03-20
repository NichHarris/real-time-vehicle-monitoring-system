#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

/*
TODO: 
- Fix Scheduling of Producer and Consumer Threads/Tasks using Clock
- Perform Message Passing on Producer
- Perform Message Passing on Consumer
- Print in Consumer the Data!
*/

// Define Thread Index for Each Producer 
#define FUEL_CONSUMPTION 0
#define ENGINE_SPEED 1
#define ENGINE_COOLANT_TEMP 2
#define CURRENT_GEAR 3
#define VEHICLE_SPEED 4

// Define Constants for Timer Conversions
#define THOUSAND	1000
#define MILLION		1000000

// Harris: Phase is static but period can be set by the user, for each producer, default is 5 sec
// Define Phase and Period for All Tasks
// - Starting at 1s, Occuring Every 5s
#define PHASE 1000000
#define PERIOD 5000000

// Define Number of Producer Threads
#define NUM_PRODUCER_THREADS 5

// 94380 rows containing sensor data and 5 columns of interest in the dataset
#define NUM_ROWS 94380
#define NUM_COLUMNS 5

// Column number for each variable of interest
#define COL_FUEL_CONSUMPTION 1
#define COL_ENGINE_SPEED 13
#define COL_ENGINE_COOLANT_TEMP 18
#define COL_CURRENT_GEAR 34
#define COL_VEHICLE_SPEED 44

// Array used to hold data produced by the producer threads
double produced[NUM_COLUMNS];

// Two-dimensional array representing the recorded sensor data for each variable
float sensor_data[NUM_COLS][NUM_ROWS];

struct producerAttributes {
    int voi;
    long period;
    sem_t* mutex;
};

// Fill sensor_data array with data read from dataset.csv
void readDataset() {
    FILE* dataset = fopen("./dataset.csv", "r");
    char line[2048]; // Line buffer
	char *record; // Used to break lines into tokens

	// Row and column counter
    int row = -1;
	int col = 0;

    if (!dataset) {
        fprintf(stderr, "Unable to open file.\n");
        return -1;
    }

    // Read the dataset line by line
    while (fgets(line, sizeof(line), dataset)) {

		// Skip the column titles row
        if (row < 0) {
            row++;
            continue;
        }

		// Get line from buffer
        record = strok(line, ",");

		// Store the tokens in their respective array entries
		while (record != NULL) {
			switch (col) {
				case COL_FUEL_CONSUMPTION:
					sensor_data[FUEL_CONSUMPTION][row] = atof(record);
					break;
				case COL_ENGINE_SPEED:
					sensor_data[ENGINE_SPEED][row] = atof(record);
					break;
				case COL_ENGINE_COOLANT_TEMP:
					sensor_data[ENGINE_COOLANT_TEMP][row] = atof(record);
					break;
				case COL_CURRENT_GEAR:
					sensor_data[CURRENT_GEAR][row] = atof(record);
					break;
				case COL_VEHICLE_SPEED:
					sensor_data[VEHICLE_SPEED][row] = atof(record);
					break;
				default:
					break;
			}

			// Increment the column number and get the next token
			col++;
			record = strtok(NULL, ",");
		}

		// Reset column counter and increment the row counter
		col = 0;
        row++;
    }

	// Close the file
    fclose(file);
}

void error_handler(char function, char error) {
    printf("Error: %s - %s\n", function, error);
    return EXIT_FAILURE;
}

//// TODO: Read and Wait for Signal to Continue
//void readVariableOfInterest(char* filename) {
//    // Open and Read Specified File
//	FILE* file = fopen(filename, "r");
//    if (!file) {
//        printf("Error: File %s could not be opened!\n", filename);
//        exit(EXIT_FAILURE);
//    }
//
//	// Define Current Line Number and Data Value
//	int line = 0;
//    char val[10];
//
//	// Skip header (first line)
//	fgets(val, 10, file);
//
//	// Read File Line By Line
//    while (fgets(val, 10, file)) {
//		printf("Line %d: %s", line, val);
//        line++;
//    }
//
//	// Close File
//    fclose(file);
//}

// Store Current Time from Real-time Clock
uint64_t currentTime;

// Define Global Thread Attribute to Specify Characteristics of POSIX (Portable Operating System Interface) Thread
pthread_attr_t attr;

// Define Global Signal Set to Specify Set of Signals Affected
sigset_t sigst;

// OLD APPROACH, UPDATED BELOW
// //// Determine File Name using Variable of Interest Index
// char* getFileName(int index) {
// 	switch (index) {
// 		case 0:
// 			// Fuel Consumption (0x01)
// 			return "./data/Fuel_Consumption.csv";
// 		case 1:
// 			// Engine Speed in RPM (0x02)
// 			return "./data/Engine_Speed.csv";
// 		case 2:
// 			// Engine Coolant Temperature (0x03)
// 			return "./data/Engine_Coolant_Temperature.csv";
// 		case 3:
// 			// Current Gear (0x04)
// 			return "./data/Current_Gear.csv";
// 		case 4:
// 			// Vehicle Speed (0x05)
// 			return "./data/Vehicle_Speed.csv";
// 		default:
// 			// Potential Error
//             error_handler("getFileName()", "Provided invalid value for data file!");
// 	}
// }

// // Producer Thread Routine
// void *threadProducer (void *arg) {
// 	// Get Variable of Interest (voi) Number Passed in Arguments
// 	int voi = *((int *) arg); 
	
// 	// Print Producer Thread Number Passed from Arguments
// 	printf("Producer Thread #%d Created!\n", voi);

// 	// Determine File Name using Thread Id
// 	char* filename = getFileName(voi);
// 	printf("Producer Thread #%d: File Used %s!\n", voi, filename);

// 	while(1) {
// 		// TODO: Produce Data based on Argument Passed
// 		// Check arg and determine which file to read
// 		// Then Perform Msg Passing

// 		// Read 
// 		/* ASK: Do we need to place all file datapoints in array or can we read in the producer using the current clock time */

// 		// Msg Pass


//         // TODO: Use Timer to Wait for Expiration Before Executing Task
// 		/* ASK: Do we use sigwait in each producer and consumer thread? */
//         // async_wait_signal();

// 		printf("Producing!");
// 	}

// 	// Terminate Thread and Exit
// 	pthread_exit(NULL);
// 	return NULL;
// }

struct producerAttributes {
    int voi;
    long period;
    sem_t* mutex;
};

// Producer Thread Routine
void *threadProducer(void *arg) {
	while (1) {
		// Lock mutex to prevent consumer thread from reading from producer while it's being written to
        sem_wait(arg->mutex);

		// Critical section, write sensor data to the producer
		produced[arg->voi] = dataset[arg->voi][currentTime];

		// Unlock mutex
		sem_post(arg->mutex);

		// Put thread to sleep for its assigned period
		uSecSleep(arg->period);
	}

	// Terminate Thread and Exit
	pthread_exit(NULL);
	return NULL;
}

// Consumer Thread Routine
void *threadConsumer() {
	printf("Consumer Thread Created!\n");
	
	while(1) {
		// TODO: Consume Data from Producer
		// Perform message passing
		// Wait for all data before printing all the data


		// Print All Variables of Interest
		printf("Current Time:  %ld\n", currentTime);
		// printf("Fuel Consumption: %f\n", fuelConsumption);
		// printf("Engine Speed: %d\n", engineSpeed);
		// printf("Engine Coolant Temperature: %d\n", engineCoolantTemperature);
		// printf("Current Gear: %d\n", currentGear);
		// printf("Vehicle Speed: %d\n", vehicleSpeed);

		// TODO: Use Timer to Wait for Expiration Before Executing Task
        // async_wait_signal();
	}

	// Terminate Thread and Exit
	pthread_exit(NULL);
	return NULL;
}

// Asynchronous wait for next signal activation adapted from timers_code.c
static void async_wait_signal(void) {
    // Use Timer to Wait for Expiration Signal Before Executing Task
    // - Suspend Thread until Timer Sends Signal to Execute Thread
	// Upon Receiving Signal, Signal Removed from Signal Set and Program Continues
	int sig; /* ASK: Do we need to pass SIGEV_SIGNAL, replace sig with signal waiting for ? */
	sigwait(&sigst, &sig); 
	
	/* ASK: Do we need to add the signal back to wait for next period after sigwait call ? */
	//const int signal = SIGALRM;
	//sigemptyset(&sigst);
	//sigaddset(&sigst, signal);
	//sigprocmask(SIG_BLOCK, &sigst, NULL);
}

// Create and Activate real-time timer to implement periodic tasks adapted from timers_code.c
int activate_realtime_clock(uint64_t phase, int period) {
	// Instantiate Timer Thread Object with Unique Timer Id
    timer_t timer;
	
    // Instantiate Timer Specifications 
	// -> Specifies Kind of Timer by Setting Timer Parameters
	// - Timer Goes Off/Triggered First Time at it_value (One Shot Value)
	// - Timer Goes Off Trigger/Go Off Again Every it_interval - Reloads Timer with Relative Value (Reload Value)
    // Note: tv_sec Specifies Value in Seconds Position, tv_nsec Specifies Value in Nano Seconds Position
    struct itimerspec timer_spec;
	timer_spec.it_value.tv_sec = phase / MILLION;
	timer_spec.it_value.tv_nsec = (phase % MILLION) * THOUSAND;
	timer_spec.it_interval.tv_sec = period / MILLION;
	timer_spec.it_interval.tv_nsec = (period % MILLION) * THOUSAND;
	
	// Add Signal (SIGALRM) to Signal Mask with sigaddset and Block All Other Signals with sigemptyset
	// Block signals (SIG_BLOCK) while in critical section (cs) with sigprocmask
	const int signal = SIGALRM;
	sigemptyset(&sigst);
	sigaddset(&sigst, signal);
	sigprocmask(SIG_BLOCK, &sigst, NULL);
	
	// Instantiate Signal Event Structure (sigevent)
	// -> Creates Notification Structure Using Signal Informing Kernel to Deliver Event
	// Specify Signal Event Notify Function as SIGEV_SIGNAL and Signal Number as SIGALRM
    struct sigevent sigev;
	memset(&sigev, 0, sizeof(struct sigevent)); /* ASK: Do we need to change memory size? */
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = signal;
	
    // Create Timer Passing Clock Id, Signal Event, and Timer
    // - Using CLOCK_REALTIME with Custom sigev Struct 
	int res = timer_create(CLOCK_REALTIME, &sigev, &timer);
	if (res < 0) {
	    error_handler("timer_create()", "Failed to create real-time timer!");
	}

    // Set and Start Time for Created Timer
    // Specify Timer Type as Relative (0) or Absolute (TIMER_ABSTIME)
	return timer_settime(timer, 0, &timer_spec, NULL);
}

// Update Current Time using Real time Clock adapted from timers_code.c
static void update_current_time(void) {	
	// Get Current Time from Real Time Clock
	struct timespec tv;
	clock_gettime(CLOCK_REALTIME, &tv);

	// Update Current Time
	currentTime = tv.tv_sec * THOUSAND + tv.tv_nsec / MILLION;	
	printf("Current Time: %ld", currentTime);
}

int main (int argc, char *argv[]) {
    // Define Return Code to Validate Thread Initialization and Creation
    // 0: Successful, -1: Unsuccessful
    // Not sure how i feel about using an int as a bool. It's fine but not best practice
    int result;

    struct producerAttributes *args;
    // Instantiate Consumer and Producer POSIX Threads
	pthread_t consumer, producers[NUM_PRODUCER_THREADS];

	// Initialize Default Attributes of POSIX Threads
	result = pthread_attr_init(&attr);
	if (result != 0) {
	    error_handler("pthread_attr_init()", "Failed to initialize pthread attributes!");
	}

    // Create Consumer Thread
    // - Pass Thread Pointer to Provide Thread Id to Created Thread
	// - Pass Customized Attributes to Create Custom Thread
	// - Pass Start Routine and Arguments to Routine

	/*
	On success, pthread_create() returns 0; on error, it returns an
	    error number, and the contents of *thread are undefined.
	Harris: I think its safer for the pthread_ functions to check that the result
	does not equal 0 rather than check if it is true.
    */
	result = pthread_create(&consumer, &attr, threadConsumer, NULL);
	if (result != 0) {
	    error_handler("pthread_create()", "Failed to create consumer thread!");
	}
	
	// Create Producers Arguments Array to  
	int producer_args[NUM_PRODUCER_THREADS];

    // Create Producers Threads
	// - Pass Thread Index as Argument to Specify Desired Data
	for(int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		producer_args[i] = i + 1;
		result = pthread_create(&producers[i], &attr, threadProducer, (void *) &producer_args[i]);
        if (result != 0) {
	        error_handler("pthread_create()", "Failed to create producer thread");
		}
    }

    // Create and Active Periodic Timer to Synchronize Threads
	result = activate_realtime_clock(PHASE, PERIOD);
	if (result < 0) {
	    error_handler("activate_realtime_clock()", "Error: Failed to create and activate periodic timer!");
	}

    // Main Loop
	while (1) {
        /* ASK: What should we do in the main thread after creating timer, producer and consumer? */
        // In main thread we should run until stop time is reached then end program exec.
        async_wait_signal();
		update_current_time();
		// Can add some break here after x amount of time to stop the program
		// This could be after it runs through all the data (function of the phase and period)
	}

    // Cleanup After Completing Program
    // Destroy Attribute Object and Terminate Thread
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);
	
	return EXIT_SUCCESS;
}