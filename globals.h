#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define BLOCK_SIZE 4096

#define CUR_DEBUG DB_WRITEBLOCK
#define CUR_ERR SHOW_ERRORS


#define SUPPRESS_ERRORS 1
#define SHOW_ERRORS 0

#define DEBUG_ALL -1
#define NO_DEBUG 0

#define DB_WRITEBLOCK 100
#define DB_READBLOCK 101

#define SUCCESS 0


#define DEBUG(debugCode, action) {\
	if (CUR_DEBUG == (debugCode)  || CUR_DEBUG == DEBUG_ALL){\
		action;\
	}\
}

#define ERR(action) {\
	if (CUR_ERR == SHOW_ERRORS){\
		action;\
	}\
}

#endif