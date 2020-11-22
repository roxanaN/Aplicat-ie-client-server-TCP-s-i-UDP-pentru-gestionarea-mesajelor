#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <cstdio>
#include <cstdlib>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		256	// dimensiunea maxima a calupului de date
#define DIM 		30
#define MAX_CLIENTS	5
#define MAXLEN 1500
#define TOPIC_DIM 50
#define IDX_TYPE 50
#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3
#endif
