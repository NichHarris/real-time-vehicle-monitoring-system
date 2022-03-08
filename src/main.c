#include <sys/time.h>
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

// Array index for each variable
#define FUEL_CONSUMPTION 0
#define ENGINE_SPEED 1
#define ENGINE_COOLANT_TEMP 2
#define CURRENT_GEAR 3
#define VEHICLE_SPEED 4

// Periods for all parameters
#define PERIOD_LENGTH 5000000 // 5 s



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

// Define Constants
#define ONE_THOUSAND	1000
#define ONE_MILLION		1000000

// Define Offset and Period in microseconds
#define OFFSET 1000000
#define PERIOD 5000000

// Define Number of Producer Threads
#define NUM_PRODUCER_THREADS 5




//The sigset_t data type is used to represent a signal set. Internally, it may be implemented as either an integer or structure type. 
sigset_t sigst;


int main (int argc, char *argv[]) {	
    // Define Thread Attribute to Specify Characteristics of POSIX (Portable Operating System Interface) Thread
    pthread_attr_t attr;

    // Define Return Code to Validate Thread Initialization and Creation
    // 0: Successful, -1: Unsuccessful
    int rc;

    // Instantiate Consumer and Producer POSIX Threads
	pthread_t consumer, producers[NUM_PRODUCER_THREADS];

    // TODO: What to modify in attributes
    // Change Detach State to Joinable to Be Able to Use Join
	rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (rc)
		printf("Error: Failed to update thread attributes with error code %d!\n", rc);
	
    // Create Consumer Thread with Custom Attributes (Customized Thread)
    // Pass Thread Pointer to Provide Thread Id to Created Thread
	rc = pthread_create(&consumer, &attr, threadConsumer, NULL);
	if (rc)
		printf("Error: Failed to create consumer thread with error code %d!\n", rc);
	
    // Create Producers Threads
	for(int i = 0; i < NUM_PRODUCER_THREADS - 1; i++) {
        // TODO: Update arg to pass to threadProducer (4th arg)
		rc = pthread_create(&producers[i], &attr, threadProducer, NULL);
        if (rc)
		    printf("Error: Failed to create producer thread with error code %d!\n", rc);
    }

    // Synchronize Threads using Join
    // Suggestion: Use Timers/Counters over Sleep 
	//pthread_join(threadD, NULL);
	pthread_join(threadC, NULL);

    // Create and Active Periodic Timer
	int res = start_periodic_timer(OFFSET, PERIOD);
	if (res < 0 ) {
		perror("Error: Start periodic timer failed");
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

// Start Routine for Customized Thread
// TODO: Create One for Producer and Consumer
void *threadProducer (void *arg) {
	int policy;
	int detachstate;
	printf("A thread with customized attributes is created!\n");
	
	/* Print out detach state */
	pthread_attr_getdetachstate(&attr, &detachstate);
	printf (" Detach state: %s\n",
	(detachstate == PTHREAD_CREATE_DETACHED) ?
	"PTHREAD_CREATE_DETACHED" :
	(detachstate == PTHREAD_CREATE_JOINABLE) ?
	"PTHREAD_CREATE_JOINABLE" : "???");
	
	/* Print out scheduling policy*/	
	pthread_attr_getschedpolicy(&attr, &policy);
	printf (" Scheduling policy: %s\n\n",
	(policy == SCHED_OTHER) ? "SCHED_OTHER" : 
	(policy == SCHED_FIFO)	? "SCHED_FIFO"  :
	(policy == SCHED_RR)	? "SCHED_RR" 	:
	"???");
	
	pthread_exit(NULL);
	return NULL;
}

void *threadConsumer (void *arg) {
	int policy;
	int detachstate;
	printf("A thread with customized attributes is created!\n");
	
	/* Print out detach state */
	pthread_attr_getdetachstate(&attr, &detachstate);
	printf (" Detach state: %s\n",
	(detachstate == PTHREAD_CREATE_DETACHED) ?
	"PTHREAD_CREATE_DETACHED" :
	(detachstate == PTHREAD_CREATE_JOINABLE) ?
	"PTHREAD_CREATE_JOINABLE" : "???");
	
	/* Print out scheduling policy*/	
	pthread_attr_getschedpolicy(&attr, &policy);
	printf (" Scheduling policy: %s\n\n",
	(policy == SCHED_OTHER) ? "SCHED_OTHER" : 
	(policy == SCHED_FIFO)	? "SCHED_FIFO"  :
	(policy == SCHED_RR)	? "SCHED_RR" 	:
	"???");
	
	pthread_exit(NULL);
	return NULL;
}

// Wait for next activation adapted from timers_code.c
static void wait_next_activation(void) {
    // Use Timer to Wait for Expiration Signal Before Executing Task
    // - Suspend Thread until Timer Sends Signal to Execute Thread
	int dummy;
	sigwait(&sigst, &dummy);
}

// Start periodic timer adapted from timers_code.c
int start_periodic_timer(uint64_t offset, int period) {
	// Instantiate Timer Object with Unique Timer Id
    timer_t timer;
	
    // Specify Kind of Timer by Setting Timer Parameters
    // Set it_value (Offset) and it_interval (Period)
    // Note: tv_sec Specifies Value in Seconds Position, tv_nsec Specifies Value in Nano Seconds Position
    struct itimerspec timer_spec;
	timer_spec.it_value.tv_sec = offset / ONE_MILLION;
	timer_spec.it_value.tv_nsec = (offset % ONE_MILLION) * ONE_THOUSAND;
	timer_spec.it_interval.tv_sec = period / ONE_MILLION;
	timer_spec.it_interval.tv_nsec = (period % ONE_MILLION) * ONE_THOUSAND;
	
    // Initialize, Add SIGALRM, and Block Signal Set
    // ...
    struct sigevent sigev;
	const int signal = SIGALRM;
	sigemptyset(&sigst);
	sigaddset(&sigst, signal);
	sigprocmask(SIG_BLOCK, &sigst, NULL);
	
    // Set Signal Event to 0, Notify Function, and Signal Number
    // ...
	memset(&sigev, 0, sizeof(struct sigevent));
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = signal;
	
    // Create Timer Passing Clock Id, Signal Event, and Timer
    // - Using CLOCK_REALTIME with Custom sigev Struct 
	int res = timer_create(CLOCK_REALTIME, &sigev, &timer);
	if (res < 0) {
		perror("Error: Timer Creation Failed.");
		exit(-1);
	}

    // Set and Start Time for Created Timer
    // Specify Timer Type as Relative (0) or Absolute (TIMER_ABSTIME)
	return timer_settime(timer, 0, &timer_spec, NULL);
}

// TODO: Update this
// Task Main Function
static void task_body(void) {
	static int cycles = 0;
	static uint64_t start;
	uint64_t current;
	struct timespec tv;
	
	if (start == 0) {
		clock_gettime(CLOCK_MONOTONIC, &tv);
		start = tv.tv_sec * ONE_THOUSAND + tv.tv_nsec / ONE_MILLION;
	}
	
	clock_gettime(CLOCK_MONOTONIC, &tv);
	current = tv.tv_sec * ONE_THOUSAND + tv.tv_nsec / ONE_MILLION;
	
	if (cycles > 0) {
		fprintf(stderr, "Avg interval between instances: %f millisecons\n",
			(double)(current-start)/cycles);
	}
	
	cycles++;
}