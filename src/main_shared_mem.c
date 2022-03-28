#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>

// Define Thread Index for Each Producer 
#define FUEL_CONSUMPTION 0
#define ENGINE_SPEED 1
#define ENGINE_COOLANT_TEMP 2
#define CURRENT_GEAR 3
#define VEHICLE_SPEED 4

// Define Constants for Timer Conversions
#define THOUSAND 1000
#define MILLION	1000000

// Phase and Period for All Tasks
// Starting at 1s, Occuring Every 5s (default)
#define PHASE 1000000
#define PERIOD 5000000
#define TIMER_PERIOD 1000000

// Define Number of Producer Threads
#define NUM_PRODUCER_THREADS 5

// 94380 rows containing sensor data and 5 columns of interest in the dataset
#define NUM_ROWS 94380
#define NUM_COLUMNS 5

// Column number for each variable of interest in dataset
#define COL_FUEL_CONSUMPTION 0
#define COL_ENGINE_SPEED 12
#define COL_ENGINE_COOLANT_TEMP 17
#define COL_CURRENT_GEAR 33
#define COL_VEHICLE_SPEED 43

// Dataset filepath (local machine)
char[] filepath = "/data/dataset.csv"
// Dataset filepath (qnx lab) 
// char[] filepath = "/public/coen320/dataset.csv"

// Array used to hold data produced by the producer threads
float produced[NUM_PRODUCER_THREADS];

// Two-dimensional array representing the recorded sensor data for each variable
float sensor_data[NUM_COLUMNS][NUM_ROWS];

// Array which holds the period of each producer
int producerPeriods[NUM_PRODUCER_THREADS] = {PERIOD, PERIOD, PERIOD, PERIOD, PERIOD};

// Mutex locks
pthread_mutex_t mutex[NUM_PRODUCER_THREADS];
// Conditions
// pthread_mutex_t cond[NUM_PRODUCER_THREADS];

// Holds the data members of producers
struct producerAttributes {
    int voi;
    int period;
    pthread_mutex_t* mutex;
	// pthread_cond_t* cond;
	bool isReleased;
	int releaseTime;
};

// Array of producer threads to run, sorted by next releaseTime
struct producerAttributes tasksToRun[NUM_PRODUCER_THREADS];

// Array in which the presently read sensor data resides
float *sharedData;

// Store current time of real-time clock/timer
uint64_t currentTime;

// Define Global Thread Attribute to Specify Characteristics of POSIX (Portable Operating System Interface) Thread
pthread_attr_t attr;

// Define Global Signal Set to Specify Set of Signals Affected
sigset_t sigst;

// Function Headers
void readDataset(void);
void *threadProducer(void *);
void *threadConsumer(void *);
static void async_wait_signal();
int activate_realtime_clock(uint64_t, int);
int update_current_time(long);

// Process the dataset, store the measurements in the sensor_data array
void readDataset() {
    FILE* stream = fopen(filepath, "r");
    char line[2048]; // Line buffer
	char *record; // Used to break lines into tokens

	// Row and column counter
    int row = -1;
	int col = 0;

    if (!stream) {
        fprintf(stderr, "Unable to open file.\n");
        exit(EXIT_FAILURE);
    }

    // Read the dataset line by line
    while (fgets(line, sizeof(line), stream)) {

		// Skip the column titles row
        if (row == -1) {
            row++;
            continue;
        }

		// Get line from buffer
        record = strtok(line, ",");

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
    fclose(stream);
}

// Initialize the mutexes
void initializeMutexes() {
    for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
        pthread_mutex_init(&mutex[i], NULL);
		//pthread_cond_inint(&cond[i], NULL);
    }
}

// Update a producer's period
void updateProducerPeriod(int index) {
    int period = 0;
    printf("\nEnter value of period for producer thread %d", index);
    scanf("%d", &period);
    if (period > 0) {
        producerPeriods[index] = period;
        printf("\nSuccessfully updated period of producer thread %d to %d\n", index, period);
    }

	// TODO: Fail Case, Non number value for period entered
}

// Handles errors produced at runtime
void error_handler(char function, char error) {
    printf("Error: %s - %s\n", function, error);
    return EXIT_FAILURE;
}

// Producer Thread Routine
void *threadProducer(void *arg) {

	// Get the producer thread's data members
    struct producerAttributes* attr = arg;
    int period = attr->period;
    int voi = attr->voi;
    pthread_mutex_t* mutex = attr->mutex;
	// pthread_cond_t* cond = attr->cond;
	printf("Producer Thread %d Initialized\n", voi);

	// Main producer loop
	while(1) {
		// Update the entry in the sharedData array for a given producer's array index (critical section)
		pthread_mutex_lock(mutex);
		/* Critical Section Start */

		//  TODO: Lock thread on condition that it is released
		//	pthread_cond_wait(cond, mutex)
		//  When cond_wait is released by the checkProducers section, by running cond_signal the critcal section can execute.
		
		// Write to shared memory segment of respective variable of interest
		sharedData[voi] = (float) sensor_data[voi][currentTime];

		// Change the run status to true, we update the next releaseTime
		updateProducerAttributes(attr, TRUE);

		/* Critical Section End */
		pthread_mutex_unlock(mutex);
		
		// // Wait until next period to release the producer thread. TODO: replace this
		// // async_wait_signal();
		// // checkProducers();
	}

	// Destroy attribute objecte and terminate producer thread
	pthread_attr_destroy(attr);
	pthread_exit(NULL);

	return NULL;
}

// Consumer Thread Routine
void *threadConsumer(struct producerAttributes* producers) {

	printf("Consumer Thread Initialized\n");
	
	// Main Loop - Consume Data from Producer
	while(1) {
		// Print the current time and all variables of interest
		printf("Current Time:  %ld\n", currentTime);

		// Critical section for reading the current fuel consumption data
		pthread_mutex_lock(producers[FUEL_CONSUMPTION]->mutex);
		printf("Fuel Consumption: %f\n", sharedData[FUEL_CONSUMPTION]);
		pthread_mutex_unlock(producers[FUEL_CONSUMPTION]->mutex);

		// Critical section for reading the current engine speed data
		pthread_mutex_lock(producers[ENGINE_SPEED]->mutex);
		printf("Engine Speed: %f\n", sharedData[ENGINE_SPEED]);
		pthread_mutex_unlock(producers[ENGINE_SPEED]->mutex);

		// Critical section for reading the current engine coolant temp data
		pthread_mutex_lock(producers[ENGINE_COOLANT_TEMP]->mutex);
		printf("Engine Coolant Temperature: %f\n", sharedData[ENGINE_COOLANT_TEMP]);
		pthread_mutex_unlock(producers[ENGINE_COOLANT_TEMP]->mutex);

		// Critical section for reading the current gear data
		pthread_mutex_lock(producers[CURRENT_GEAR]->mutex);
		printf("Current Gear: %f\n", sharedData[CURRENT_GEAR]);
		pthread_mutex_unlock(producers[CURRENT_GEAR]->mutex);

		// Critical section for reading the vehicle speed data
		pthread_mutex_lock(producers[VEHICLE_SPEED]->mutex);
		printf("Vehicle Speed: %f\n", sharedData[VEHICLE_SPEED]);
		pthread_mutex_unlock(producers[VEHICLE_SPEED]->mutex);

		// TODO: Use Timer to Wait for Expiration Before Executing Task
        async_wait_signal();
	}

	// Terminate Thread and Exit
	pthread_exit(NULL);
	return NULL;
}

// Asynchronous wait for next signal activation/clock interrupt adapted from timers_code.c
static void async_wait_signal(void) {
    // Use Timer to Wait for Expiration Signal Before Executing Task
    // - Suspend Thread until Timer Sends Signal/Clock Interrupt to Execute Thread
	int sig;
	sigwait(&sigst, &sig);
}

// Create and activate real-time timer to implement periodic tasks adapted from timers_code.c
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
	
	// Add Signal (SIGALRM - Signal Alarm) to Signal Mask with sigaddset and Block All Other Signals with sigemptyset
	// Block signals (SIG_BLOCK) while in critical section (cs) with sigprocmask
	const int signal = SIGALRM;
	sigemptyset(&sigst);
	sigaddset(&sigst, signal);
	sigprocmask(SIG_BLOCK, &sigst, NULL);
	
	// Instantiate Signal Event Structure (sigevent)
	// -> Creates Notification Structure Using Signal Informing Kernel to Deliver Event
	// Specify Signal Event Notify Function as SIGEV_SIGNAL (Sending Signal - sigev_signo) and Signal Number as SIGALRM
    struct sigevent sigev;
	memset(&sigev, 0, sizeof(struct sigevent));
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
1
// Get Time in s from Real Time Clock
uint64_t get_time_sec(struct timespec tv) {
	return tv.tv_sec + tv.tv_nsec / (MILLION * THOUSAND);
}

// TODO: Call this after activating timer
// Update Current Time using Real time Clock adapted from timers_code.c
static void update_current_time(void) {	
	// Get Start Time
	static uint64_t startTime;

	// Get Current Time from Real Time Clock
	struct timespec tv;
	clock_gettime(CLOCK_REALTIME, &tv);

	// Get Start Time to Determine Current Time Starting from 0
	if (startTime == 0) {
		startTime = get_time_sec(tv);
	}

	// Update Current Time
	currentTime = get_time_sec(tv) - startTime;	
	printf("Current Time: %ld", (long) currentTime);
}

static void sortTasksToRun() {
	// Sorts tasksToRun array in ascending order by the producerReleaseTimes
	qsort(tasksToRun, NUM_PRODUCER_THREADS, sizeof(struct producerAttributes), compareReleaseTimes);
}

// Used for qsort function to compare left and right element releaseTimes
struct producerAttributes compareReleaseTimes(struct producerAttributes* left, struct producerAttributes* right){	
	return (left->releaseTime - right->releaseTime);
}

// TODO: Test
static void updateProducerAttributes(struct producerAttributes* producerAttr, bool hasRun) {
	// Thread has run the producer for this period, we can update the period and set the status to True
	if (hasRun) {
		producerAttr->releaseTime += producerAttr->period;
		producerAttr->isReleased = FALSE;
	} else {
		producerAttr->isReleased = TRUE;
	}
}

// TODINGUS
static void checkProducers() {
	bool hasChanged = FALSE;
	for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		// If task released time is less than or equal to current time and it has not already been released, we unlock it and update its status
		if (tasksToRun[i].releaseTime <= currentTime) {
			// Task should be released
			if (!tasksToRun[i].isReleased) {
				// We call an update on it to change its release time to the next instance, however it has not run yet
				updateProducerAttributes(&tasksToRun[i], FALSE);
				// TODO: Can release condition lock here to allow the producer thread to continue
				// TODO: pthread_cond_signal(tasksToRun[i].cond)

				hasChanged = TRUE;
			}
		} else {
			// Exit loop since we don't need to check the next tasks
			break;
		}
	}
	
	// Only sort arr if task release times were changed
	if (hasChanged) {
		sortTasksToRun();
	}
}

int main (int argc, char *argv[]) {

    // Process the dataset and store the sensor data in memory
    readDataset();

	// Get the size of the shared memory segment
	int SHM_SIZE = NUM_PRODUCER_THREADS * sizeof(float *);

	// Create shared memory segment for the sharedData array
	int shm_fd = shm_open("/sharedData", O_CREAT | O_RDWR, 0666);
	if (shm_fd == -1)
	{
		perror("Error: shm_open() failed. Exiting...");
		exit(1);
	}
	
	// Truncate file to specified size for shared memory segment
	ftruncate(shm_fd, SHM_SIZE);

	// Create new mapping in virtual address space
	sharedData = (float *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if(sharedData == MAP_FAILED){
		perror("Error: mmap() failed. Exiting...");
		exit(1);
	}

	// Initialize the shared memory to 0
	for (int i=0; i < NUM_PRODUCER_THREADS; i++) {
		sharedData[i] = 0;
	}

	struct producerAttributes *args[NUM_PRODUCER_THREADS + 1];

    // Instantiate Consumer and Producer POSIX Threads
	pthread_t consumer, producers[NUM_PRODUCER_THREADS];

	// Store Return Code to Validate Thread Initialization and Creation
	// 0: Successful, -1: Unsuccessful
	int result;

	// Initialize Default Attributes of POSIX Threads
	result = pthread_attr_init(&attr);
	if (result != 0) {
		error_handler("pthread_attr_init()", "Failed to initialize pthread attributes!");
	}

    // Create Consumer Thread
    // - Pass Thread Pointer to Provide Thread Id to Created Thread
	// - Pass Customized Attributes to Create Custom Thread
	// - Pass Start Routine and Arguments to Routine

	int input;
	printf("--- Select program setup options below ---\n");
	printf("[0] - Run all producer threads with default period\n");
	printf("[1] - Manually enter the period for all producer threads\n");
	printf("[2] - Modify only a specific producer thread's period\n");
	printf("[3] - Exit\n");
	printf("Enter value of selection: ");
	scanf("%d", &input);
	switch(input) {
		case 0:
			printf("\nRunning all threads in default mode\n");
			break;
		case 1:
			for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
				updateProducerPeriod(i);
			}
			break;
		case 2:
			int threadIndex;
			printf("\nModify specific thread period selected, select thread to modify [0 to 4]: ");
			scanf("%d", &threadIndex);
			if (threadIndex < NUM_PRODUCER_THREADS && threadIndex >= 0) {
				updateProducerPeriod(threadIndex);
				break;
			} else {
				printf("\nInvalid thread, exiting...");
				return 0;
			}
		case 3:
			printf("\nProgram exit selected, ending...");
			return 0;
		default:
			printf("\nInvalid entry, ending program...");
			return 0;
	}

	initializeMutexes();

    // Set the attributes of each producer thread
    for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
        args[i]->voi = i;
        args[i]->period = producerPeriods[i];
        args[i]->mutex = &mutex[i];
        // args[i]->cond = &cond[i];
		args[i]->releaseTime = 0;
		args[i]->isReleased = FALSE;
    }

    // Create Producers Threads
	// - Pass Thread Index as Argument to Specify Desired Data
	for(int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		result = pthread_create(&producers[i], &attr, &threadProducer, (void *) &args[i]);
        if (result != 0) {
	        error_handler("pthread_create()", "Failed to create producer thread");
		} else {
			tasksToRun[i] = &args[i]; // TODO: Test
		}
    }

	// Initial sort
	sortTasksToRun();

	/*
	On success, pthread_create() returns 0; on error, it returns an
	error number, and the contents of *thread are undefined.
	*/
	result = pthread_create(&consumer, &attr, &threadConsumer, NULL);
	if (result != 0)
	{
		error_handler("pthread_create()", "Failed to create consumer thread!");
	}

	// Create and Active Periodic Timer to Synchronize Threads
	result = activate_realtime_clock(PHASE, TIMER_PERIOD);
	if (result != 0) {
	    error_handler("activate_realtime_clock()", "Failed to create and activate periodic timer!");
	}

	while(1) {		
		// Checks if tasks should be released.
		checkProducers();
		// Waits for timer interrupt, GCD of periods
		async_wait_signal();
	}

	// Unlink Shared Memory Segment
	if (shm_unlink("/sharedData") == -1) {
		perror("Error: shm_unlink() failed. Exiting...");
		exit(1);
	}

	// Cleanup after completing program
    // Destroy attribute object and terminate main thread
	pthread_exit(NULL);
	
	return EXIT_SUCCESS;
}
