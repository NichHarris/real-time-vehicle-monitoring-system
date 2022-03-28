#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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

// Define phase and period for all tasks/threads
// Starting at 1s, occuring every 5s (default)
#define PHASE 1000000
#define PERIOD 5000000


// Instantiate gloabl signal set (sigst) to specify set of signals affected by timer activation
sigset_t sigst;

// Define Mutexes For Each Producer Thread to Lock Critical Sections
pthread_mutex_t mutex[NUM_PRODUCER_THREADS];

// Define Global Thread Attribute to Specify Characteristics of POSIX (Portable Operating System Interface) Thread
pthread_attr_t attr;

// Holds the data members of producers
struct producer_args {
    int voi;
    int period;
    pthread_mutex_t* mutex;
};

// Store global current time of real-time clock/timer
double currentTime;

// Array which holds the period of each producer
int producerPeriods[NUM_PRODUCER_THREADS] = {PERIOD, PERIOD, PERIOD, PERIOD, PERIOD};

// Prints and handles errors produced at runtime
void error_handler(char *function, char *error) {
    printf("Error: %s - %s\n", function, error);
    exit(EXIT_FAILURE);
}

// Wait for signal activation/clock interrupt to schedule and synchronize threads adapted from timers_code.c
static void wait_clock_interrupt(void) {
	// Suspend thread until timer expiration by waiting for alarm signal
	int sig;
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
	printf("Current Time: %f\n", currentTime);
}

// Perform greatest common divisor (GCD) using Euclidean Division
// -> Obtain suitable clock interval for accurate timing
int calc_gcd(int p1, int p2) {
	if(p1 == 0)
		return p2;

	return calc_gcd(p2 % p1, p1);
}

// Perform least common multiple (LCM) using GCD factor
// -> Obtain suitable time to request user input at start of major cycle
int calc_lcm(int p1, int p2) {
	return (p1 * p2)/calc_gcd(p1, p2);
}

// Get clock interval by calculating GCD among producer thread periods
int get_clock_interval(int periods[]) {
	// Iterate over each period to obtain greatest common divisor, suitable clock interval
	int clock_interval = periods[0];
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		clock_interval = calc_gcd(periods[i], clock_interval);
	}

	return clock_interval;
}

// Get hyperperiod by calculating LCM among producer thread periods
int get_hyperperiod(int periods[]) {
	// Iterate over each period to obtain least common multiple for major cycle
	int major_cycle = periods[0];
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		major_cycle = calc_lcm(periods[i], major_cycle);
	}

	return major_cycle;
}


// Get consumer period
// -> Represents minimum producer period
int get_consumer_period(int periods[]) {
	int min_period = periods[0];
	for(int i = 1; i < NUM_PRODUCER_THREADS; i++) {
		if(min_period > periods[i]) {
			min_period = periods[i];
		}
	}

	return min_period;
}

// Get file name from variable of interest index
char* getFileName(int voi) {
    // Determine file name using variable of interest
	switch (voi) {
		case 0:
			// Fuel Consumption (0x00)
			return "Fuel_Consumption.csv";
		case 1:
			// Engine Speed in RPM (0x01)
			return "Engine_Speed.csv";
		case 2:
			// Engine Coolant Temperature (0x02)
			return "Engine_Coolant_Temperature.csv";
		case 3:
			// Current Gear (0x03)
			return "Current_Gear.csv";
		case 4:
			// Vehicle Speed (0x04)
			return "Vehicle_Speed.csv";
		default:
			// Potential error
			error_handler("getFileName()", "Provided invalid value for file!");
	}

	return "Failed!";
}

// Producer thread routine
void *threadProducer(void *arg) {
	// Get the producer thread's data members
    struct producer_args* pa = arg;
    int period = pa->period;
    int voi = pa->voi;
    pthread_mutex_t* mutex = pa->mutex;

	printf("Producer Thread %d Initialized\n", voi);

	char* a = getFileName(voi);

	// Wait for clock interrupt
	//wait_clock_interrupt();
	update_current_time();


	return NULL;
}

// Consumer thread routine
void *threadConsumer(void *arg) {

	printf("Consumer Thread Initialized\n\n");

	// Wait for clock interrupt
	//wait_clock_interrupt();
	update_current_time();


	return NULL;
}

// Update a producer's period
void updateProducerPeriod(int index) {
    int period = 0;
    printf("\nEnter period for producer thread %d: ", index);
    fflush(stdout);
    scanf("%d", &period);
    if (period > 0) {
        producerPeriods[index] = period;
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
	puts("[3] - Exit");
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
				error_handler("requestUserInput()", "Invalid thread index, exiting....");
			}
			break;
		case 3:
			printf("\nProgram exit selected, ending program successfully...");
			exit(EXIT_SUCCESS);
		default:
			error_handler("requestUserInput()", "Invalid entry, ending program....");
	}
}

int main(void) {
	struct producer_args args[NUM_PRODUCER_THREADS];

    // Instantiate consumer and producer POSIX threads
	pthread_t consumer, producers[NUM_PRODUCER_THREADS];

	// Store result to validate thread and timer function calls
	// 0: Successful, -1: Unsuccessful
	int res;

	// Initialize default attributes of POSIX threads
	res = pthread_attr_init(&attr);
	if (res != 0) {
		error_handler("pthread_attr_init()", "Failed to initialize pthread attributes!");
	}

	// Request user input
	requestUserInput();

	// Create and activate periodic timer to synchronize threads
	res = activate_realtime_clock(PHASE, PERIOD);
	if (res != 0) {
		error_handler("activate_realtime_clock()", "Failed to create and activate periodic timer!");
	}

	// Update current time
	update_current_time();


   for(int i = 0; i < NUM_PRODUCER_THREADS; i++) {
		// Initialize mutex for each producer thread
		pthread_mutex_init(&mutex[i], NULL);

		// Create thread arguments used in thread start routine
		args[i].voi = i;
		args[i].period = producerPeriods[i];
		args[i].mutex = &mutex[i];

		// Create producer threads
		res = pthread_create(&producers[i], &attr, threadProducer, (void *) &args[i]);
		if (res != 0) {
			error_handler("pthread_create()", "Failed to create producer thread");
		}
    }

	// Create consumer thread
	// - Pass thread pointer to provide thread id to created thread
	// - Pass customized attributes to create custom thread
	// - Pass start routine and arguments to routine
	res = pthread_create(&consumer, &attr, threadConsumer, NULL);
	if (res != 0) {
		error_handler("pthread_create()", "Failed to create consumer thread!");
	}

	// Request user input every hyperperiod
	int hyperperiod = get_hyperperiod(producerPeriods);
	printf("%d", hyperperiod);
	while(1) {
		if(hyperperiod <= currentTime) {
			requestUserInput();
		}

		wait_clock_interrupt();
		update_current_time();
	}

	// Cleanup after completing program
    // Destroy attribute object and terminate main thread
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);

	return EXIT_SUCCESS;
}
