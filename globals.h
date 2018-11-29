#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BLOCK_SIZE 4096

/* Return codes */
#define TOTALBLOCKS_INVALID -1001
#define FS_TOO_SMALL -1002
#define BLOCKSIZE_TOO_SMALL -1003
#define DATA_FULL -1004
#define BAD_INODE -1005
#define ILIST_FULL -1007

#define DISC_UNINITIALIZED -1
#define INVALID_BLOCK -2
#define BUF_NULL -3
#define INT_NULL -4


#define SUCCESS 0
#define UNEXPECTED_ERROR -99999

/* Current debug and error status */
//#define CUR_DEBUG DB_INODEFREE
#define CUR_DEBUG DB_INODECREATE
#define CUR_ERR SHOW_ERRORS

/* Error codes */
#define SUPPRESS_ERRORS 1
#define SHOW_ERRORS 0

/* Debug codes */
#define DEBUG_ALL -1

#define NO_DEBUG 0
#define DB_WRITEBLOCK 100
#define DB_READBLOCK 101
#define DB_MKFS 1100
#define DB_READSB 1101
#define DB_WRITEDATA 1102
#define DB_READDATA 1103
#define DB_FREELIST 1104
#define DB_WRITESB 1105
#define DB_DATAFREE 1106
#define DB_DATAALL 1107
#define DB_INODEREAD 1108
#define DB_INODEWRITE 1109
#define DB_INODEFREE 1110
#define DB_IBITMAP 1111
#define DB_INODECREATE 1112


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Conditional print macros for debugging */
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