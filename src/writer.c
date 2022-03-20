#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>

int main() {
    // Shared Memory Objected Created
    // Processes Have Access to Pointer for Direct Read and Write Accesses
    // Advantage: Fastest IPC Method, Disadvantage: Need Synchronization Primitives (Semaphores or Mutexes) to Avoid Reading and Writing Concurrently
	const int SHM_SIZE = 4096; 
	const int MSG_SIZE = 100;

    // Shared Memory Object - File for POSIX Threads
	const char *name = "/my_shm"; 
	char message[MSG_SIZE];

	int shm_fd;

    // Shared Memory Pointer
	void *ptr;

    // Create Shared Memory Segment
    // -> Name of Shared Memory Object, Flags (O_RDWR - Allows Read and Write), Mode
	shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
    	perror("Error: shm_open() Failed!");
		exit(1);
	}

    // Truncate File to Specified Size for Shared Memory Segment
	ftruncate(shm_fd, SHM_SIZE);

    // Create and Map Shared Memory Segment in Address Space of Processes
    // Location in Address Space of Process, Number of Bytes to Map for Shared Memory, Memory Protections (PROT_READ | PROT_WRITE = Protect Read and Write Bits Enabled),
    // Mapping Flags for , Shared Memory Object's File Descriptor (shm_fd), Offset Within Shared Memory to Start Mapping
	ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) { 
		printf("Error: Map failed!");
        return -1;
	}

    // Read User Input using fgets
	printf("Type a message:\n");
	fgets (message, MSG_SIZE, stdin);

    // TODO: Add Mutex to Lock and Unlock for Writing when Other Process is Reading or Writing
    // Write Message to Shared Memory using sprintf
	sprintf(ptr,"%s",message);
	printf("Shared Memory Content: %s\n", message);

	return 0;
}