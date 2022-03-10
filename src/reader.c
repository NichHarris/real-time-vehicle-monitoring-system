#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

//TODO: Review with Slides
int main() {
	const char *name = "/my_shm";
	const int SIZE = 4096;
	int shm_fd;
	void *ptr;
	/* open the shared memory segment */ 
	shm_fd = shm_open(name, O_RDONLY, 0666); 
	if (shm_fd == -1) {
    	perror("in shm_open()");
		exit(1);
	}
	
	/* now map the shared memory segment in the address space of the process */
	ptr = mmap(0,SIZE, PROT_READ, MAP_SHARED, shm_fd, 0); 
	if (ptr == MAP_FAILED) {
    	perror("in mmap()");
		exit(1);
	}
	
	/* now read from the shared memory region */
	printf("Content in the shared memory:\n");
	printf("    %s", ptr);
	/* remove the shared memory segment */ 
	if (shm_unlink(name) == -1) {
    	perror("in shm_unlink()");
		exit(1);
	}

	return 0;
}