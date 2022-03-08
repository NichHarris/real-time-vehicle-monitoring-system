#include <sys/time.h>
#include <sys/neutrino.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

// 94380 rows containing sensor data and 5 columns of interest in the dataset
#define NUM_ROWS 94380
#define NUM_COLUMNS 5

// Column number for each variable of interest
#define COL_FUEL_CONSUMPTION 1
#define COL_ENGINE_SPEED 13
#define COL_ENGINE_COOLANT_TEMP 18
#define COL_CURRENT_GEAR 34
#define COL_VEHICLE_SPEED 44

// Define Constants
#define ONE_THOUSAND	1000
#define ONE_MILLION		1000000


//-----
// Define Thread Index for Each Producer 
#define FUEL_CONSUMPTION 0
#define ENGINE_SPEED 1
#define ENGINE_COOLANT_TEMP 2
#define CURRENT_GEAR 3
#define VEHICLE_SPEED 4

// Define Offset and Period for All Tasks
// - Starting at 1s, Occuring Every 5s
#define OFFSET 1000000 // 1 second
#define PERIOD 5000000 // 5 seconds

// Define Number of Producer Threads
#define NUM_PRODUCER_THREADS 5
//---

/* int with # of seconds elapsed to update sensor data each 1 second */
int currentTime = 0;

//array with use to hold data produced by the producer threads
double produced[NUM_COLUMNS];

/* 2D Array containing recorded sensor information (the dataset read into memory) */
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

// --------------------- Change this (from Mark's code) ---------------------
/* Extract specified column from extracted row of dataset
 * Function code adapted from https://stackoverflow.com/questions/12911299/read-csv-file-in-c */
const char* getfield(char* line, int num) {
    for (tok = strtok(line, ",");
        tok && *tok;
        tok = strtok(NULL, ",\n")) {
        if (!--num)
            return tok;
    }
    return NULL;
}

// Define Global Thread Attribute to Specify Characteristics of POSIX (Portable Operating System Interface) Thread
pthread_attr_t attr;

// Define Global Signal Set to Specify Set of Signals Affected
sigset_t sigst;

int main (int argc, char *argv[]) {
    // Define Return Code to Validate Thread Initialization and Creation
    // 0: Successful, -1: Unsuccessful
    int rc;

    // Instantiate Consumer and Producer POSIX Threads
	pthread_t consumer, producers[NUM_PRODUCER_THREADS];

	// Initialize Default Attributes of POSIX Threads
	rc = pthread_attr_init(&attr);
	if (rc) {
		printf("Error %d: Failed to initialize pthread attributes! \n", rc);
		return -1;
	}

    // Create Consumer Thread
    // - Pass Thread Pointer to Provide Thread Id to Created Thread
	// - Pass Customized Attributes to Create Custom Thread
	// - Pass Start Routine and Arguments to Routine
	rc = pthread_create(&consumer, &attr, threadConsumer, NULL);
	if (rc) {
		printf("Error %d: Failed to create consumer thread! \n", rc);
		return -1;
	}

    // Create Producers Threads
	// - Pass Thread Index as Argument to Specify Desired Data
	for(int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		rc = pthread_create(&producers[i], &attr, threadProducer, i + 1);
        if (rc) {
		    printf("Error %d: Failed to create producer thread #%d! \n", rc, i);
			return -1;
		}
    }

    // Create and Active Periodic Timer to Synchronize Threads
	int res = start_periodic_timer(OFFSET, PERIOD);
	if (res < 0 ) {
		printf("Error %d: Failed to create and activate periodic timer!", res);
		return -1;
	}
	
    // Main Loop
	while (1) {
        // Use Timer to Wait for Expiration Before Executing Task
        wait_next_activation();
		task_body();
	}

    // Cleanup After Completing Program
    // Destroy Attribute Object and Terminate Thread
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);
	
	return 0;
}

// Producer Thread Routine
void *threadProducer (void *arg) {
	// Print Producer Thread Number Passed from Arguments
	printf("Producer Thread #%d Created!\n", arg);

	// Define Variable of Interest using Passed Argument (voi)
	int voi = arg; 
	char fileName[];

	while(1) {
		// TODO: Produce Data based on Argument Passed
		// Check arg and determine which file to read
		// Then Perform Msg Passing

		// Determine File Name using Variable of Interest 
		if(voi = 1) {
			// Fuel Consumption (0x01)
			fileName = "./"
		} else if(voi = 2) {
			// Engine Spped in RPM (0x02)
			fileName = "./"
		} else if(voi = 3) {
			//Engine Coolant Temperature (0x03)
			fileName = "./"
		} else if(voi = 4) {
			// Current Gear (0x04)
			fileName = "./"
		} else if(voi = 5) {
			// Vehicle Speed (0x05)
			fileName = "./"
		} else {
			printf(voi);
			printf("WTFF")
		}

		// Read 


		// Msg Pass

		printf("Producing!")
	}

	pthread_exit(NULL);
	return NULL;
}

// Consumer Thread Routine
void *threadConsumer (void *arg) {
	printf("Consumer Thread Created!\n");
	
	while(1) {
		// TODO: Consume Data from Producer
		// Perform message passing
		// Wait for all data before printing all the data
		printf("Consuming!")
	}

	pthread_exit(NULL);
	return NULL;
}

// Wait for next activation signal adapted from timers_code.c
static void wait_next_activation(void) {
    // Use Timer to Wait for Expiration Signal Before Executing Task
    // - Suspend Thread until Timer Sends Signal to Execute Thread
	int dummy; //TODO: Ask why dummy is empty
	sigwait(&sigst, &dummy);
}

// Create and Activate real-time timer to implement periodic tasks adapted from timers_code.c
int start_periodic_timer(uint64_t offset, int period) {
	// Instantiate Timer Thread Object with Unique Timer Id
    timer_t timer;
	
    // Instantiate Timer Specifications 
	// -> Specifies Kind of Timer by Setting Timer Parameters
    // Set it_value as offset and it_interval as period
	// - Timer Goes Off/Triggered First Time at it_value (One Shot Value)
	// - Timer Goes Off Trigger/Go Off Again Every it_interval - Reloads Timer with Relative Value (Reload Value)
    // Note: tv_sec Specifies Value in Seconds Position, tv_nsec Specifies Value in Nano Seconds Position
    struct itimerspec timer_spec;
	timer_spec.it_value.tv_sec = offset / ONE_MILLION;
	timer_spec.it_value.tv_nsec = (offset % ONE_MILLION) * ONE_THOUSAND;
	timer_spec.it_interval.tv_sec = period / ONE_MILLION;
	timer_spec.it_interval.tv_nsec = (period % ONE_MILLION) * ONE_THOUSAND;
	
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
	memset(&sigev, 0, sizeof(struct sigevent));
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = signal;
	
    // Create Timer Passing Clock Id, Signal Event, and Timer
    // - Using CLOCK_REALTIME with Custom sigev Struct 
	int res = timer_create(CLOCK_REALTIME, &sigev, &timer);
	if (res < 0) {
		perror("Error: Failed to create real-time timer!");
		exit(-1);
	}

    // Set and Start Time for Created Timer
    // Specify Timer Type as Relative (0) or Absolute (TIMER_ABSTIME)
	return timer_settime(timer, 0, &timer_spec, NULL);
}

// Update Current Time using Real time Clock adapted from timers_code.c
static void task_body(void) {
	uint64_t current;
	struct timespec tv;
	
	clock_gettime(CLOCK_REALTIME, &tv);
	current = tv.tv_sec * ONE_THOUSAND + tv.tv_nsec / ONE_MILLION;	
}


/*
// TODO: Create Header File
const char* getfield(char*, int);
void extractParameterValues(int, int);
void *threadProducer(void *);
void *threadConsumer(void *);
static void wait_next_activation();
int start_periodic_timer(uint64_t, int);
int task_body(long);
*/