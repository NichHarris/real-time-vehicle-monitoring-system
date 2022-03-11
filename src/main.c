#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

/*
TODO: 
- Fix Scheduling of Producer and Consumer Threads/Tasks using Clock
- Work on getting file name, then reading specific file
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

// Define Phase and Period for All Tasks
// - Starting at 1s, Occuring Every 5s
#define PHASE 1000000
#define PERIOD 5000000

// Define Number of Producer Threads
#define NUM_PRODUCER_THREADS 5


/*
// 94380 rows containing sensor data and 5 columns of interest in the dataset
#define NUM_ROWS 94380
#define NUM_COLUMNS 5

// Column number for each variable of interest
#define COL_FUEL_CONSUMPTION 1
#define COL_ENGINE_SPEED 13
#define COL_ENGINE_COOLANT_TEMP 18
#define COL_CURRENT_GEAR 34
#define COL_VEHICLE_SPEED 44

//array with use to hold data produced by the producer threads
double produced[NUM_COLUMNS];

// 2D Array containing recorded sensor information (the dataset read into memory) 
double dataset[ROW_NUM][COL_NUM];

// Two-dimensional array representing the recorded data for each variable
float sensor_data[NUM_COLS][NUM_ROWS];

// --------------------- Change this (from Mark's code) ---------------------
// Fill sensor_data with data read from dataset.csv
void readDataset(int col, int param) {
    FILE* file = fopen("./dataset.csv", "r");
    char line[2048];
    int row = -1; // Discard first (title) row

    if (!file) {
        fprintf(stderr, "File cannot be opened.\n");
        exit(EXIT_FAILURE);
    }

    // Grabs a line of the dataset
    while (fgets(line, 2048, file)) {
        char* tmp = strdup(line); // Store row from buffer to temporary memory
        if (row < 0) {
            // Discard first (title) row
            row++;
            free(tmp);
            continue;
        }

        dataset[row][param] = atof(getfield(tmp, col)); // Save row to array, getfield picks demanded column from saved row
        row++;
        free(tmp); // Free temporarily saved row
    }
    fclose(file);
}
*/

// TODO: Read and Wait for Signal to Continue
void readVariableOfInterest(char* filename) {
    // Open and Read Specified File
	FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: File %s could not be opened!\n", filename);
        exit(EXIT_FAILURE);
    }

	// Define Current Line Number and Data Value
	int line = 0;
    char val[10];

	// Skip header (first line)
	fgets(val, 10, file);

	// Read File Line By Line
    while (fgets(val, 10, file)) {
		printf("Line %d: %s", line, val);
        line++;
    }

	// Close File
    fclose(file);
}

// Store Current Time from Real-time Clock
uint64_t currentTime;

// Define Global Thread Attribute to Specify Characteristics of POSIX (Portable Operating System Interface) Thread
pthread_attr_t attr;

// Define Global Signal Set to Specify Set of Signals Affected
sigset_t sigst;

// Determine File Name using Variable of Interest Index
char* getFileName(int voi) {
	switch (voi) {
		case 1:
			// Fuel Consumption (0x01)
			return "./data/Fuel_Consumption.csv";
		case 2:
			// Engine Speed in RPM (0x02)
			return "./data/Engine_Speed.csv";
		case 3:
			// Engine Coolant Temperature (0x03)
			return "./data/Engine_Coolant_Temperature.csv";
		case 4:
			// Current Gear (0x04)
			return "./data/Current_Gear.csv";
		case 5:
			// Vehicle Speed (0x05)
			return "./data/Vehicle_Speed.csv";
		default:
			// Potential Error
			printf("Error: Provided value %d for file!", voi);
			exit(EXIT_FAILURE);
	}
}

// Producer Thread Routine
void *threadProducer (void *arg) {
	// Get Variable of Interest (voi) Number Passed in Arguments
	int voi = *((int *) arg); 
	
	// Print Producer Thread Number Passed from Arguments
	printf("Producer Thread #%d Created!\n", voi);

	// Determine File Name using Thread Id
	char* filename = getFileName(voi);
	printf("Producer Thread #%d: File Used %s!\n", voi, filename);

	while(1) {
		// TODO: Produce Data based on Argument Passed
		// Check arg and determine which file to read
		// Then Perform Msg Passing

		// Read 
		/* ASK: Do we need to place all file datapoints in array or can we read in the producer using the current clock time */

		// Msg Pass


        // TODO: Use Timer to Wait for Expiration Before Executing Task
		/* ASK: Do we use sigwait in each producer and consumer thread? */
        // async_wait_signal();

		printf("Producing!");
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
		perror("Error: Failed to create real-time timer!");
		exit(EXIT_FAILURE);
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
    int rc;

    // Instantiate Consumer and Producer POSIX Threads
	pthread_t consumer, producers[NUM_PRODUCER_THREADS];

	// Initialize Default Attributes of POSIX Threads
	rc = pthread_attr_init(&attr);
	if (rc) {
		printf("Error: Failed to initialize pthread attributes! \n");
		return EXIT_FAILURE;
	}

    // Create Consumer Thread
    // - Pass Thread Pointer to Provide Thread Id to Created Thread
	// - Pass Customized Attributes to Create Custom Thread
	// - Pass Start Routine and Arguments to Routine
	rc = pthread_create(&consumer, &attr, threadConsumer, NULL);
	if (rc) {
		printf("Error: Failed to create consumer thread! \n");
		return EXIT_FAILURE;
	}
	
	// Create Producers Arguments Array to  
	int producer_args[NUM_PRODUCER_THREADS];

    // Create Producers Threads
	// - Pass Thread Index as Argument to Specify Desired Data
	for(int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		producer_args[i] = i + 1;
		rc = pthread_create(&producers[i], &attr, threadProducer, (void *) &producer_args[i]);
        if (rc) {
		    printf("Error: Failed to create producer thread #%d! \n", i);
			return EXIT_FAILURE;
		}
    }

    // Create and Active Periodic Timer to Synchronize Threads
	int res = activate_realtime_clock(PHASE, PERIOD);
	if (res < 0 ) {
		printf("Error: Failed to create and activate periodic timer!");
		return EXIT_FAILURE;
	}
	
    // Main Loop
	while (1) {
        /* ASK: What should we do in the main thread after creating timer, producer and consumer? */
        async_wait_signal();
		update_current_time();
	}

    // Cleanup After Completing Program
    // Destroy Attribute Object and Terminate Thread
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);
	
	return EXIT_SUCCESS;
}

/*
// TODO: Create Header File
void extractParameterValues(int, int);
void *threadProducer(void *);
void *threadConsumer(void *);
static void async_wait_signal();
int activate_realtime_clock(uint64_t, int);
int update_current_time(long);
*/