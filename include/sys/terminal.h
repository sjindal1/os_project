#ifndef _TERMINAL_H
#define _TERMINAL_H

uint64_t _termwrite(uint8_t *, uint64_t size);

uint64_t __termread();

typedef struct bufdetails bufdetails;
struct bufdetails
{
	uint8_t valid;
	uint8_t size;
};

#endif