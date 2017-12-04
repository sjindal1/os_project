#ifndef _UTILS_H
#define _UTILS_H

void memset(void* p, int v, int size);

int strcmp(uint8_t *string1, uint8_t *string2);

uint64_t strlen(uint8_t *str);

int strStartsWith(uint8_t origStr[], uint8_t checkStr[]);

void strConcat(uint8_t *first, uint8_t *second, uint8_t *final);

int strspt(uint8_t * input, uint8_t str[][256], uint8_t delim);

int strcontains(uint8_t *input, uint8_t uint8_tacter);

uint8_t strlastuint8_t(uint8_t *input);

void reverse(uint8_t *str, int len);

void itoa(uint32_t x, uint8_t *str);

uint8_t* trimwhitespaces(uint8_t *input);

#endif
