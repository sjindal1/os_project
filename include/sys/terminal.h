#ifndef _TERMINAL_H
#define _TERMINAL_H

uint64_t _termwrite(uint8_t *, uint64_t size);

uint64_t _termread(uint8_t *, uint64_t size);

typedef struct bufdetails bufdetails;
struct bufdetails
{
	uint8_t valid;
	uint8_t size;
	uint8_t xpos;
};

#endif