#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

int main() {
	const char *name = "/my_shm";
	const int SIZE = 4096;
	int shm_fd;
	void *ptr;

    // Open the Shared Memory Segment as Read Only (Consumer)
	shm_fd = shm_open(name, O_RDONLY, 0666); 
	if (shm_fd == -1) {
    	perror("Error: shm_open() Failed");
		exit(1);
	}
	
    // Create and Map Shared Memory Segment in Address Space of Process
	ptr = mmap(0,SIZE, PROT_READ, MAP_SHARED, shm_fd, 0); 
	if (ptr == MAP_FAILED) {
    	perror("Error: mmap() Failed");
		exit(1);
	}

    // TODO: Add Mutex to Lock and Unlock for Reading when Other Process is Writing
    // Read Shared Memory Segment
	printf("Shared Memory: %s", ptr);

    // Unlink Shared Memory Segment
	if (shm_unlink(name) == -1) {
    	perror("Error: shm_unlink() Failed");
		exit(1);
	}

	return 0;
}