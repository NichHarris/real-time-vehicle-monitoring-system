#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/siginfo.h>
#include <signal.h>

// Define Constants
#define ONE_THOUSAND	1000
#define ONE_MILLION		1000000

// Define Offset and Period in microseconds
#define OFFSET 1000000
#define PERIOD 5000000

sigset_t sigst;

struct timespec { 
    long tv_sec, tv_nsec;
};

// NOTE: Not in original file, Might Not Be Needed
// Create Notification Structure by Signal, Pulse, or Thread 
// Informs Kernel of Event to Deliver
struct sigevent {
    // Define Method of Notification
    int sigev_notify;

    // Define Another Data Type Using Union
    union {
        int sigev_signo;
        int sigev_coid;
        int sigev_id;
        void (*sigev_notify_function)(union sigval)
    };

    union sigval sigev_value;

    union {
        struct {
            short sigev_code;
            short sigev_priority;
        };

        pthread_attr_t *sigev_notify_attributes;
    };
};

// Create Timer Set Off Time
// - Timer Goes Off/Triggered First Time At it_value (One Shot Value)
// - Reload Timer with Relative Value it_interval (Reload Value), Gets Trigger/Go Off Again Every it_interval
// Note: Use 0 as it_interval for One Shot Timer 
// Note: Set it_value and it_interval to Period for Pure Periodic Timer
struct itimerspec { 
    struct timespec it_value, it_interval; 
};

static void wait_next_activation(void) {
    // Use Timer to Wait for Expiration Signal Before Executing Task
    // - Suspend Thread until Timer Sends Signal to Execute Thread
	int dummy;
	sigwait(&sigst, &dummy);
}

int start_periodic_timer(uint64_t offset, int period) {
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
    struct sigevent sigev;
	const int signal = SIGALRM;
	sigemptyset(&sigst);
	sigaddset(&sigst, signal);
	sigprocmask(SIG_BLOCK, &sigst, NULL);
	
    // Set Signal Event to 0, Notify Function, and Signal Number
	memset(&sigev, 0, sizeof(struct sigevent));
	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = signal;
	
    // Create Timer Passing Clock Id, Signal Event, and Timer
    // Note: Use CLOCK_REALTIME with Custom sigev Struct 
	int res = timer_create(CLOCK_MONOTONIC, &sigev, &timer);
	if (res < 0) {
		perror("Error: Timer Create Failed.");
		exit(-1);
	}

    // Set and Start Time for Created Timer
    // Specify Timer Type as Relative (0) or Absolute (TIMER_ABSTIME)
	return timer_settime(timer, 0, &timer_spec, NULL);
}

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

int main (int argc, char *argv[]) {	
    // Create and Active Periodic Timer
	int res = start_periodic_timer(OFFSET, PERIOD);
	if (res < 0 ){
		perror("Error: Start periodic timer failed");
		return -1;
	}
	
	while (1) {
        // Use Timer to Wait for Expiration Before Executing Task
        wait_next_activation();
		task_body();
	}
	
	return 0;
}
