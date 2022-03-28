#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

// For shared mem
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

// For POSIX Threads
#include <pthread.h>

// For Interval Timer
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

// Define constants for timer conversions
#define THOUSAND 1000
#define MILLION	1000000

// Define number of producer threads
#define NUM_PRODUCER_THREADS 5

// 94380 rows containing sensor data and 5 columns of interest in the dataset
#define NUM_ROWS 94380
#define NUM_COLUMNS 5

// Define Thread Index for Each Producer
#define FUEL_CONSUMPTION 0
#define ENGINE_SPEED 1
#define ENGINE_COOLANT_TEMP 2
#define CURRENT_GEAR 3
#define VEHICLE_SPEED 4

// Column number for each variable of interest in dataset
#define COL_FUEL_CONSUMPTION 0
#define COL_ENGINE_SPEED 12
#define COL_ENGINE_COOLANT_TEMP 17
#define COL_CURRENT_GEAR 33
#define COL_VEHICLE_SPEED 43

// Define phase and period for all tasks/threads
// Starting at 1s, occuring every 5s (default)
#define PHASE 1000000
#define PERIOD 5000000

// Dataset filepath (local machine)
char filepath[] = "/data/dataset.csv";
// Dataset filepath (qnx lab)
// char[] filepath = "/public/coen320/dataset.csv"

// Array used to hold data produced by the producer threads
float produced[NUM_PRODUCER_THREADS];

// Two-dimensional array representing the recorded sensor data for each variable
float sensor_data[NUM_COLUMNS][NUM_ROWS];

// Array in which the presently read sensor data resides
float *sharedData;

// Instantiate gloabl signal set (sigst) to specify set of signals affected by timer activation
sigset_t sigst;

// Define Mutexes For Each Producer Thread to Lock Critical Sections
pthread_mutex_t mutex[NUM_PRODUCER_THREADS];

// Define Global Thread Attribute to Specify Characteristics of POSIX (Portable Operating System Interface) Thread
pthread_attr_t attr;

// Holds the data members of producers
typedef struct producerAttributes {
    int voi;
    int period;
    pthread_mutex_t* mutex;
	pthread_cond_t* cond;
	bool isReleased;
	int releaseTime;
};

// Store global current time of real-time clock/timer
double currentTime;

// Array which holds the period of each producer
int producerPeriods[NUM_PRODUCER_THREADS] = {PERIOD, PERIOD, PERIOD, PERIOD, PERIOD};

// Mutex locks
pthread_mutex_t mutex[NUM_PRODUCER_THREADS];

// Conditions
pthread_mutex_t cond[NUM_PRODUCER_THREADS];

// Array of producer threads to run, sorted by next releaseTime
struct producerAttributes tasksToRun[NUM_PRODUCER_THREADS];
struct producerAttributes producersAttrs[NUM_PRODUCER_THREADS];

// Function headers
void updateProducerAttributes(struct producerAttributes*, bool);
int compareReleaseTimes(const void*, const void*);
static void update_current_time(void);
static void checkProducers();

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

// Initialize the mutexes and condition locks
void initializeMutexes() {
    for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
        pthread_mutex_init(&mutex[i], NULL);
		pthread_cond_init(&cond[i], NULL);
    }
}

// Prints and handles errors produced at runtime
void error_handler(char *function, char *error) {
    printf("Error: %s - %s\n", function, error);
    exit(EXIT_FAILURE);
}

// Wait for signal activation/clock interrupt to schedule and synchronize threads adapted from timers_code.c
static void wait_clock_interrupt(void) {
	// Suspend thread until timer expiration by waiting for alarm signal
	int sig;
	update_current_time();
	sigwait(&sigst, &sig);
}

// Create and activate real-time timer to implement periodic tasks adapted from timers_code.c
int activate_realtime_clock(uint64_t phase, int period) {
	// Instantiate timer thread (timer_t) object with unique timer id
    timer_t timer;

    // Initialize interval timer specifications (itimerspec)
	// -> Specifies kind of timer by setting timer parameters
    // -> Timer starts with it_value (one shot value) and reloads it_interval (reload value) after timer expiration
    // Note: tv_sec and tv_nsec specify timer value (tv) in seconds and nano seconds position
    struct itimerspec timer_spec;
	timer_spec.it_value.tv_sec = phase / MILLION;
	timer_spec.it_value.tv_nsec = (phase % MILLION) * THOUSAND;
	timer_spec.it_interval.tv_sec = period / MILLION;
	timer_spec.it_interval.tv_nsec = (period % MILLION) * THOUSAND;

	// Initialize empty signal set to block all signals
	// -> Add alarm signal (SIGALRM) to signal mask
	// -> Block signals when in critical section (sigprocmask with SIG_BLOCK)
	const int signal = SIGALRM;
	sigemptyset(&sigst);
	sigaddset(&sigst, signal);
	sigprocmask(SIG_BLOCK, &sigst, NULL);

	// Initialize structure to signal an event (sigevent)
	// -> Use a signal as the event notification method (sigev_notify using SIGEV_SIGNAL)
	// -> Send alarm signal to notify of interval timer expiration (SIGALRM)
    struct sigevent sigev;
	memset(&sigev, 0, sizeof(struct sigevent));
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = signal;

    // Create timer passing timer object and signal event structure
    // -> Make a real time clock (CLOCK_REALTIME)
	int res = timer_create(CLOCK_REALTIME, &sigev, &timer);
	if (res < 0) {
	    error_handler("timer_create()", "Failed to create real-time timer!");
	}

    // Set and start interval timer passing timer specifications
	return timer_settime(timer, 0, &timer_spec, NULL);
}

// Get time in seconds from real time clock
double get_time_sec(struct timespec tv) {
	return tv.tv_sec + (double) tv.tv_nsec / (MILLION * THOUSAND);
}

// Update global current time variable using real time clock adapted from timers_code.c
static void update_current_time(void) {
	// Instantiate start time
	static double startTime;

	// Get current time value (tv) from real time clock
	struct timespec tv;
	clock_gettime(CLOCK_REALTIME, &tv);

	// Initialize start time to determine current time
	if (startTime == 0) {
		startTime = get_time_sec(tv);
	}

	// Update current time
	currentTime = get_time_sec(tv) - startTime;
}

// TODO: Ensure correct base
// Perform greatest common divisor (GCD) using Euclidean Division
// -> Obtain suitable clock interval for accurate timing
int calc_gcd(int period1, int period2) {
	if(period1 == 0)
		return period2;
	if(period2 == 0)
			return period1;
	if(period2 == period1)
		return period1;

	return calc_gcd(period2 % period1, period1);
}

// TODO: Ensure correct base
// Perform least common multiple (LCM) using GCD factor
// -> Obtain suitable time to request user input at start of major cycle
int calc_lcm(int p1, int p2) {
	return (p1 * p2)/calc_gcd(p1, p2);
}

// TODO: Ensure correct base
// Get clock interval by calculating GCD among producer thread periods
int get_clock_interval(int periods[]) {
	// Iterate over each period to obtain greatest common divisor, suitable clock interval
	int clock_interval = periods[0]/MILLION;
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		clock_interval = calc_gcd(periods[i]/MILLION, clock_interval);
	}

	return clock_interval*MILLION;
}

// TODO: Change to correct base
// Get hyperperiod by calculating LCM among producer thread periods
int get_hyperperiod(int periods[]) {
	// Iterate over each period to obtain least common multiple for major cycle
	int major_cycle = periods[0]/MILLION;
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		major_cycle = calc_lcm(major_cycle, periods[i]/MILLION);
	}

	return major_cycle*MILLION;
}

// Get consumer period
// -> Represents minimum producer period
int get_consumer_period(int periods[]) {
	int min_period = periods[0]/MILLION;
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		if(min_period > periods[i]/MILLION) {
			min_period = periods[i]/MILLION;
		}
	}

	return min_period*MILLION;
}

// Producer thread routine
void *threadProducer(void *args) {
	// Get the producer thread's data members
    struct producerAttributes* producerAttr = args;
    int voi = producerAttr->voi;
    pthread_mutex_t* mutex = producerAttr->mutex;
    pthread_mutex_t* cond = producerAttr->cond;

	printf("Producer Thread %d Initialized\n", voi);

	while (1) {
		// Update the entry in the sharedData array for a given producer's array index (critical section)
		pthread_mutex_lock(mutex);
		/* Critical Section Start */
		pthread_cond_wait(cond, mutex);
		printf("Curren time: %d\n", (int) currentTime);
		// Write to shared memory segment of respective variable of interest
		printf("Data read is: %f\n", (float) sensor_data[voi][(int) currentTime]);
		sharedData[voi] = (float) sensor_data[voi][(int) currentTime];

		// Change the run status to true, we update the next releaseTime
		updateProducerAttributes(producerAttr, true);

		/* Critical Section End */
		pthread_mutex_unlock(mutex);
	}

	return NULL;
}

// Consumer thread routine
void *threadConsumer(void *arg) {

	printf("Consumer Thread Initialized\n\n");

	struct producerAttributes** producers = arg;

	// Main Loop - Consume Data from Producer
	while(1) {
		// Print the current time and all variables of interest
//		printf("Current Time:  %f\n", currentTime);

//		// Critical section for reading the current fuel consumption data
//		pthread_mutex_lock(producers[FUEL_CONSUMPTION].mutex);
//		printf("Fuel Consumption: %f\n", sharedData[FUEL_CONSUMPTION]);
//		pthread_mutex_unlock(producers[FUEL_CONSUMPTION].mutex);
//
//		// Critical section for reading the current engine speed data
//		pthread_mutex_lock(producers[ENGINE_SPEED].mutex);
//		printf("Engine Speed: %f\n", sharedData[ENGINE_SPEED]);
//		pthread_mutex_unlock(producers[ENGINE_SPEED].mutex);
//
//		// Critical section for reading the current engine coolant temp data
//		pthread_mutex_lock(producers[ENGINE_COOLANT_TEMP].mutex);
//		printf("Engine Coolant Temperature: %f\n", sharedData[ENGINE_COOLANT_TEMP]);
//		pthread_mutex_unlock(producers[ENGINE_COOLANT_TEMP].mutex);
//
//		// Critical section for reading the current gear data
//		pthread_mutex_lock(producers[CURRENT_GEAR].mutex);
//		printf("Current Gear: %f\n", sharedData[CURRENT_GEAR]);
//		pthread_mutex_unlock(producers[CURRENT_GEAR].mutex);
//
//		// Critical section for reading the vehicle speed data
//		pthread_mutex_lock(producers[VEHICLE_SPEED].mutex);
//		printf("Vehicle Speed: %f\n", sharedData[VEHICLE_SPEED]);
//		pthread_mutex_unlock(producers[VEHICLE_SPEED].mutex);
	}

	// Terminate Thread and Exit
	pthread_exit(NULL);
	return NULL;
}

// Sorts tasksToRun array in ascending order by the producerReleaseTimes
void sortTasksToRun() {
	qsort(tasksToRun, NUM_PRODUCER_THREADS, sizeof(struct producerAttributes*), compareReleaseTimes);
	return;
}

// Used for qsort function to compare left and right element releaseTimes
int compareReleaseTimes(const void* left, const void* right){
	const struct producerAttributes* releaseTimeLeft = (struct producerAttributes*) left;
	const struct producerAttributes* releaseTimeRight = (struct producerAttributes*) right;
	return (releaseTimeLeft->releaseTime - releaseTimeRight->releaseTime);
}

// Thread has run the producer for this period, we can update the period and set the status to true
void updateProducerAttributes(struct producerAttributes* producerAttr, bool hasRun) {
	if (hasRun) {
//		printf("Task: %d\n", producerAttr->voi);
//		printf("ReleaseTime: %d\n", producerAttr->releaseTime);
//		printf("New release time %d.\n", producerAttr->period/MILLION + producerAttr->releaseTime);
		producerAttr->releaseTime += producerAttr->period/MILLION;
		producerAttr->isReleased = false;
	} else {
//		printf("Is released is true now.\n");
		producerAttr->isReleased = true;
	}
}

// TODINGUS
static void checkProducers() {
	bool hasChanged = false;
	for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		// If task released time is less than or equal to current time and it has not already been released, we unlock it and update its status
		if (tasksToRun[i].releaseTime <= currentTime) {
			printf("Release time has been reached for task %d.\n", tasksToRun[i].voi);
			printf(tasksToRun[i].isReleased ? "true":"false");
			// Task should be released
			if (!tasksToRun[i].isReleased) {

				// We call an update on it to change its release time to the next instance, however it has not run yet
				updateProducerAttributes(&tasksToRun[i], false);
				// TODO: Can release condition lock here to allow the producer thread to continue
				pthread_cond_signal(tasksToRun[i].cond);
				printf("Released task %d.\n", tasksToRun[i].voi);
				hasChanged = true;
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

// TODO: Set period to base million
// Update a producer's period
void updateProducerPeriod(int index) {
    int period = 0;
    printf("\nEnter period for producer thread %d: ", index);
    fflush(stdout);
    scanf("%d", &period);
    if (period > 0) {
        producerPeriods[index] = period*MILLION;
        printf("Successfully updated period of producer thread %d to %d\n", index, period);
    } else {
        error_handler("updateProducerPeriod()", "Entered invalid period for producer thread!\n");
    }
}

// Request user input
void requestUserInput() {
	int input, threadIndex;
	puts("--- Select program setup options below ---");
	puts("[0] - Run all producer threads with default period");
	puts("[1] - Manually enter the periods for all producer threads");
	puts("[2] - Modify only a specific producer thread's period");
	puts("[3] - Continue execution");
	puts("[4] - Exit");
	puts("Enter value of selection: ");
	scanf("%d", &input);

	switch(input) {
		case 0:
			puts("\nRunning all threads in default mode!\n");
			break;
		case 1:
			for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
				updateProducerPeriod(i);
			}
			break;
		case 2:
			puts("\nModify specific thread period selected.");
			puts("Select thread index to modify [0 to 4]]: ");
			scanf("%d", &threadIndex);
			if (threadIndex < NUM_PRODUCER_THREADS && threadIndex >= 0) {
				updateProducerPeriod(threadIndex);
			} else {
				error_handler("requestUserInput()", "Invalid thread index, exiting...");
			}
			break;
		case 3:
			break;
		case 4:
			printf("\nProgram exit selected, ending program successfully...");
			exit(EXIT_SUCCESS);
		default:
			error_handler("requestUserInput()", "Invalid entry, ending program...");
	}
}

int main(void) {

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
	sharedData = (float*) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if(sharedData == MAP_FAILED){
		perror("Error: mmap() failed. Exiting...");
		exit(1);
	}

	// Initialize the shared memory to 0
	for (int i=0; i < NUM_PRODUCER_THREADS; i++) {
		sharedData[i] = 0;
	}

    // Instantiate consumer and producer POSIX threads
	pthread_t consumer, producers[NUM_PRODUCER_THREADS];

	// Initialize mutex for each producer thread
	initializeMutexes();

	// Store result to validate thread and timer function calls
	// 0: Successful, -1: Unsuccessful
	int res;

	// Initialize default attributes of POSIX threads
	res = pthread_attr_init(&attr);
	if (res != 0) {
		error_handler("pthread_attr_init()", "Failed to initialize pthread attributes!");
	}

	// Request user input for producer periods
	requestUserInput();

	// Get clock period
	int period = get_clock_interval(producerPeriods);

	// Create and activate periodic timer to synchronize threads
	res = activate_realtime_clock(PHASE, period);
	if (res != 0) {
		error_handler("activate_realtime_clock()", "Failed to create and activate periodic timer!");
	}

	wait_clock_interrupt();

    for(int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		// Create thread arguments used in thread start routine
		tasksToRun[i].voi = i;
		tasksToRun[i].period = producerPeriods[i];
		tasksToRun[i].mutex = &mutex[i];
		tasksToRun[i].cond = &cond[i];
		tasksToRun[i].releaseTime = 0;
		tasksToRun[i].isReleased = false;

		// Create producer threads
		res = pthread_create(&producers[i], &attr, threadProducer, (void *) &tasksToRun[i]);
		if (res != 0) {
			error_handler("pthread_create()", "Failed to create producer thread");
		} else {
			producersAttrs[i] = tasksToRun[i];
		}
    }

	// Initial sort
	sortTasksToRun();
	// Run scheduling algorithm
//	checkProducers();
	// Create consumer thread
	// - Pass thread pointer to provide thread id to created thread
	// - Pass customized attributes to create custom thread
	// - Pass start routine and arguments to routine
	// TODO: Pass lcm of periods to be the timing of consumer
	res = pthread_create(&consumer, &attr, threadConsumer, (void *) producersAttrs);
	if (res != 0) {
		error_handler("pthread_create()", "Failed to create consumer thread!");
	}

	// Request user input every hyperperiod
	int hyperperiod = get_hyperperiod(producerPeriods);
	printf("%d\n", hyperperiod);
	printf("%d\n", period);

	update_current_time();
	while(1) {
		// Run scheduling algorithm
		// Call timer interrupt
		wait_clock_interrupt();
		checkProducers();
		if(hyperperiod/MILLION <= currentTime) {
			requestUserInput();
		}

//		// Call timer interrupt
//		wait_clock_interrupt();
//		printf("Current Time: %f\n", currentTime);
//		update_current_time();
	}

	// Unlink Shared Memory Segment
	if (shm_unlink("/sharedData") == -1) {
		perror("Error: shm_unlink() failed. Exiting...");
		exit(1);
	}

	// Cleanup after completing program
    // Destroy attribute object and terminate main thread
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);

	return EXIT_SUCCESS;
}
