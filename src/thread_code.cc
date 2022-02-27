#include <pthread.h>
#include <stdio.h>

// Define Thread Attribute to Specify Characteristics of POSIX Thread
pthread_attr_t attr;

// Start Routine for Default Thread
void *threadDefault (void *arg) {
	printf("A thread with default attributes is created\n\n");
	return NULL;
}

// Start Routine for Customized Thread
void *threadCustomized (void *arg) {
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

int main (int argc, char* argv[]) {
    // Define Return Code to Validate Thread Initialization and Creation
    // 0: Successful, -1: Unsuccessful
    int rc;

    // Define 2 POSIX Threads
	pthread_t threadD, threadC;

	// Initialize Attribute Structure of POSIX Threads using Default Attributes
	rc = pthread_attr_init(&attr);
	if (rc)
		printf("Error: pthread_attr_init() with code %d \n", rc);
		
    // Change Detach State to Joinable to Be Able to Use Join
	rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (rc)
		printf("Error: pthread_attr_setdetachstate() with code %d \n", rc);
		
    // Create Thread with Default Attributes
    // Pass Thread Pointer to Provide Thread Id to Created Thread
	rc = pthread_create (&threadD, NULL, threadDefault, NULL);
	if (rc)
		printf("Error: Default pthread_create() with code %d \n", rc);
	
    // Create Thread with Custom Attributes (Customized Thread)
    // Pass Thread Pointer to Provide Thread Id to Created Thread
	rc = pthread_create(&threadC, &attr, threadCustomized, NULL);
	if (rc)
		printf("Error: Customized pthread_create() with code %d \n", rc);
	
    // Synchronize Threads using Join
    // Suggestion: Use Timers/Counters over Sleep 
	pthread_join(threadD, NULL);
	pthread_join(threadC, NULL);
	
    // Destroy Attribute Object and Terminate Thread
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);
}