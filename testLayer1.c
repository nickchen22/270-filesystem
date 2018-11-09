#include "globals.h"
#include "layer0.h"
#include "layer1.h"

int main(int argc, const char **argv){
	printf("Hello World\n");
	
	total_blocks = 5;
	disk = malloc(BLOCK_SIZE * 5);
	uint8_t testBuf[BLOCK_SIZE];
	
	readBlock(1, testBuf);
	writeBlock(0, NULL);

	return EXIT_SUCCESS;
}