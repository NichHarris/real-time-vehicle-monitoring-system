#include <sys/time.h>
#include <sys/neutrino.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

/* Global variables */
// Size of dataset is 94380 rows (94381 in total but first row is title row)
#define ROW_NUM 94380
#define COL_NUM 8

// Define columns for each parameter to be studied in the dataset
#define FUEL_CONSUMPTION_COL 1 // Col A
#define ENGINE_SPEED_COL 13 // Col M
#define ENGINE_COOLANT_TEMP_COL 18 // Col R
#define CURRENT_GEAR_COL 34 // Col AH
#define TRANSMISSION_OIL_TEMP_COL 35 // Col AI
#define VEHICLE_SPEED_COL 44 // Col AR
#define ACCEL_SPEED_LONG_COL 45 // Col AS
#define BRAKE_SWITCH_COL 46 // Col AT

// Define array columns for each parameter
#define FUEL_CONSUMPTION 0
#define ENGINE_SPEED 1
#define ENGINE_COOLANT_TEMP 2
#define CURRENT_GEAR 3
#define TRANSMISSION_OIL_TEMP 4
#define VEHICLE_SPEED 5
#define ACCEL_SPEED_LONG 6
#define BRAKE_SWITCH 7

// Number of threads total to be created
#define NUMTHREAD 9

/* Timer periods and offsets
 * offset and period are in microseconds. */
#define ONE_THOUSAND	1000
#define ONE_MILLION		1000000

// Sensor update timer
#define SENSOR_UPDATE_OFFSET 1000000
#define SENSOR_UPDATE_PERIOD 1000000 // Global timer period 1 sec (counts seconds elapsed)

// Consumer thread
#define CONSUMER_PERIOD 100000 // Consumer thread period = 100 ms

// Time to run program for (will stop after STOP_TIME seconds elapsed)
#define STOP_TIME 100 // Time that program will run in seconds

// Periods for each parameter timer
#define FUEL_COMSUMPTION_PERIOD 10000 // 10 ms
#define ENGINE_SPEED_PERIOD 500000 // 500 ms
#define ENGINE_COOLANT_PERIOD 2000000 // 2 s
#define CURRENT_GEAR_PERIOD 100000 // 100 ms
#define TRANSMISSION_OIL_PERIOD 5000000 // 5 s
#define VEHICLE_SPEED_PERIOD 100000 // 100 ms
#define ACCEL_SPEED_LONG_PERIOD 150000 // 150 ms
#define BRAKE_SWITCH_PERIOD 100000 // 100 ms

/* Path of the dataset */
char path[] = "/public/coen320/dataset.csv";

/* 2D Array containing recorded sensor information (the dataset read into memory) */
double dataset[ROW_NUM][COL_NUM];

/* int with # of seconds elapsed to update sensor data each 1 second */
int currentTime = 0;

/* Array that will hold data produced by the producer threads (buffer, one slot for each parameter) */
double produced[COL_NUM];

/* Mutex locks */
sem_t mutex_0;
sem_t mutex_1;
sem_t mutex_2;
sem_t mutex_3;
sem_t mutex_4;
sem_t mutex_5;
sem_t mutex_6;
sem_t mutex_7;

/* Signal set for the global timer */
sigset_t sigst;

/* Struct for producer thread arguments, carries the parameter column value and period from the macros */
struct pArgs {
	int param;
	long period;
	sem_t* mutex;
};

/* Function Headers */
const char* getfield(char*, int);
void extractParameterValues(int, int);
static void wait_next_activation();
int start_periodic_timer(uint64_t, int);
int uSecSleep(long);
void *producerThread(void *);
void *consumerThread(void *);

/* Main function */
int main (int argc, char *argv[]) {
	// Declare variables
	int i = 0;
	int res;

	// Initialize structs for the arguments for each producer pthread
	struct pArgs *args[NUMTHREAD-1];

	// Initialize each producer and the consumer pthread
	pthread_t thread[NUMTHREAD];

	// Initialize mutex locks for each buffer
    sem_init(&mutex_0, 0, 1) ;
    sem_init(&mutex_1, 0, 1) ;
    sem_init(&mutex_2, 0, 1) ;
    sem_init(&mutex_3, 0, 1) ;
    sem_init(&mutex_4, 0, 1) ;
    sem_init(&mutex_5, 0, 1) ;
    sem_init(&mutex_6, 0, 1) ;
    sem_init(&mutex_7, 0, 1) ;

	// Load required csv columns into memory
	printf("Loading dataset into memory\n");
	printf("Loading Fuel Consumption Data\n");
  	extractParameterValues(FUEL_CONSUMPTION_COL, FUEL_CONSUMPTION);
  	printf("Loading Engine Speed Data\n");
    extractParameterValues(ENGINE_SPEED_COL, ENGINE_SPEED);
    printf("Loading Engine Coolant Temperature Data\n");
    extractParameterValues(ENGINE_COOLANT_TEMP_COL, ENGINE_COOLANT_TEMP);
    printf("Loading Current Gear Data\n");
    extractParameterValues(CURRENT_GEAR_COL, CURRENT_GEAR);
    printf("Loading Transmission Oil Temperature Data\n");
    extractParameterValues(TRANSMISSION_OIL_TEMP_COL, TRANSMISSION_OIL_TEMP);
    printf("Loading Vehicle Speed Data\n");
    extractParameterValues(VEHICLE_SPEED_COL, VEHICLE_SPEED);
    printf("Loading Acceleration Speed Longitudinal Data\n");
    extractParameterValues(ACCEL_SPEED_LONG_COL, ACCEL_SPEED_LONG);
    printf("Loading Brake Switch Indication Data\n");
    extractParameterValues(BRAKE_SWITCH_COL, BRAKE_SWITCH);
	printf("Dataset loaded into memory\n");

	// Set arguments for all 8 producer threads
	args[0] = calloc(1, sizeof(struct pArgs));
	args[0]->param = FUEL_CONSUMPTION;
	args[0]->period = FUEL_COMSUMPTION_PERIOD;
	args[0]->mutex = &mutex_0;

	args[1] = calloc(1, sizeof(struct pArgs));
	args[1]->param = ENGINE_SPEED;
	args[1]->period = ENGINE_SPEED_PERIOD;
	args[1]->mutex = &mutex_1;

	args[2] = calloc(1, sizeof(struct pArgs));
	args[2]->param = ENGINE_COOLANT_TEMP;
	args[2]->period = ENGINE_COOLANT_PERIOD;
	args[2]->mutex = &mutex_2;

	args[3] = calloc(1, sizeof(struct pArgs));
	args[3]->param = CURRENT_GEAR;
	args[3]->period = CURRENT_GEAR_PERIOD;
	args[3]->mutex = &mutex_3;

	args[4] = calloc(1, sizeof(struct pArgs));
	args[4]->param = TRANSMISSION_OIL_TEMP;
	args[4]->period = TRANSMISSION_OIL_PERIOD;
	args[4]->mutex = &mutex_4;

	args[5] = calloc(1, sizeof(struct pArgs));
	args[5]->param = VEHICLE_SPEED;
	args[5]->period = VEHICLE_SPEED_PERIOD;
	args[5]->mutex = &mutex_5;

	args[6] = calloc(1, sizeof(struct pArgs));
	args[6]->param = ACCEL_SPEED_LONG;
	args[6]->period = ACCEL_SPEED_LONG_PERIOD;
	args[6]->mutex = &mutex_6;

	args[7] = calloc(1, sizeof(struct pArgs));
	args[7]->param = BRAKE_SWITCH;
	args[7]->period = BRAKE_SWITCH_PERIOD;
	args[7]->mutex = &mutex_7;

	//Create Producer pthreads
	for(i = 0; i< NUMTHREAD-1; i++){
		pthread_create(&thread[i], NULL, (void *) &producerThread, (void *) args[i]);
	}
	// Create Consumer pthread
	pthread_create(&thread[8], NULL, (void *) &consumerThread, NULL);

	// Global timer, update sensor data each 1 second (uses periodic_thread.c)
	res = start_periodic_timer(SENSOR_UPDATE_OFFSET, SENSOR_UPDATE_PERIOD);
	if (res < 0 ){
		perror("Start periodic timer");
		return -1;
	}
	while (1) {
		wait_next_activation(); //wait for timer expiration
		currentTime++;
		if (currentTime >= STOP_TIME) { // Stops full program execution after given stop time (currently 100 sec)
			printf("**************************************************\n");
			printf("Stop time of %d seconds has been reached\n", STOP_TIME);
			break;
		}
	}

	// Program execution has concluded
	printf("Program Execution Has Ended Successfully");
	return EXIT_SUCCESS;
}


/* Producer thread */
void *producerThread(void *arg) {
	struct pArgs* vars = arg; // Arguments passed
	long period = vars->period;
	int column = vars->param;
	sem_t* mutex = vars->mutex; // Mutex for parameter

	while (1) {
        sem_wait(mutex); // Lock mutex so consumer does not read while data is being written
		produced[column] = dataset[currentTime][column]; // Write data, critical section
		sem_post(mutex); // Unlock mutex for consumer to read
		uSecSleep(period); // Time for specific producer thread
	}
	return NULL;
}

/* Consumer Thread */
void *consumerThread(void *arg) {
	while(1){
		printf("**************************************************\n");
		printf("Elapsed Time (sec): %d\n", currentTime);
        sem_wait(&mutex_0); // Lock mutex for specified parameter so data isn't written while being read
        printf("Fuel Consumption: %f\n", produced[FUEL_CONSUMPTION]);
        sem_post(&mutex_0); // Unlock mutex for specified parameter, allow producer to write data
        sem_wait(&mutex_1);
        printf("Engine Speed (RPM): %f\n", produced[ENGINE_SPEED]);
        sem_post(&mutex_1);
        sem_wait(&mutex_2);
        printf("Engine Cooland Temperature: %f\n", produced[ENGINE_COOLANT_TEMP]);
        sem_post(&mutex_2);
        sem_wait(&mutex_3);
        printf("Current Gear: %f\n", produced[CURRENT_GEAR]);
        sem_post(&mutex_3);
        sem_wait(&mutex_4);
        printf("Transmission Oil Temperature: %f\n", produced[TRANSMISSION_OIL_TEMP]);
        sem_post(&mutex_4);
        sem_wait(&mutex_5);
        printf("Vehicle Speed: %f\n", produced[VEHICLE_SPEED]);
        sem_post(&mutex_5);
        sem_wait(&mutex_6);
        printf("Acceleration Speed Longitudinal: %f\n", produced[ACCEL_SPEED_LONG]);
        sem_post(&mutex_6);
        sem_wait(&mutex_7);
        printf("Brake Switch Indication: %f\n", produced[BRAKE_SWITCH]);
        sem_post(&mutex_7);
        uSecSleep(CONSUMER_PERIOD); // Timer for consumer thread
	}

	return NULL;
}

/* Extract specified column from extracted row of dataset
 * Function code adapted from https://stackoverflow.com/questions/12911299/read-csv-file-in-c */
const char* getfield(char* line, int num) {
    const char* tok;
    for (tok = strtok(line, ",");
        tok && *tok;
        tok = strtok(NULL, ",\n")) {
        if (!--num)
            return tok;
    }
    return NULL;
}

/* Fill all specified parameter values from dataset into 2-D array
 * Function code adapted from https://stackoverflow.com/questions/12911299/read-csv-file-in-c */
void extractParameterValues(int col, int param) {
    FILE* file = fopen(path, "r");
    char line[2048];
    int row = -1; // Discard first (title) row

    if (!file) {
        fprintf(stderr, "Can't open file.\n");
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

/* Function that will sleep thread for given microseconds using nanosleep function */
int uSecSleep(long tus) {
    int res;
    struct timespec timer_spec;

	/* set timer parameters */
    timer_spec.tv_sec = tus / ONE_MILLION;
    timer_spec.tv_nsec = (tus % ONE_MILLION) * ONE_THOUSAND; // sec -> ns

    do {
        res = nanosleep(&timer_spec, &timer_spec); // Run timer
    } while (res && errno == EINTR);
    // do-while ensures nanosleep will resume if interrupted by a signal (EINTR -> interrupt error)
    return res;
}

/* Function which executes the currentTime timer each period (1 sec)
 * Function code adapted  from periodic_task.c */
static void wait_next_activation() {
	int dummy;
	/* suspend calling process until a signal is pending */
	sigwait(&sigst, &dummy);
}

/* Timer to control currentTime
 * Function code adapted  from periodic_task.c */
int start_periodic_timer(uint64_t offset, int period) {
	struct itimerspec timer_spec;
	struct sigevent sigev;
	timer_t timer;
	const int signal = SIGALRM;
	int res;

	/* set timer parameters */
	timer_spec.it_value.tv_sec = offset / ONE_MILLION;
	timer_spec.it_value.tv_nsec = (offset % ONE_MILLION) * ONE_THOUSAND;
	timer_spec.it_interval.tv_sec = period / ONE_MILLION;
	timer_spec.it_interval.tv_nsec = (period % ONE_MILLION) * ONE_THOUSAND;

	sigemptyset(&sigst); // initialize a signal set
	sigaddset(&sigst, signal); // add SIGALRM to the signal set
	sigprocmask(SIG_BLOCK, &sigst, NULL); //block the signal

	/* set the signal event a timer expiration */
	memset(&sigev, 0, sizeof(struct sigevent));
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = signal;

	/* create timer */
	res = timer_create(CLOCK_REALTIME, &sigev, &timer);

	if (res < 0) {
		perror("Timer Create");
		exit(-1);
	}

	/* activate the timer */
	return timer_settime(timer, 0, &timer_spec, NULL);
}
