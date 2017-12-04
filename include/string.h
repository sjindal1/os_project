


#ifndef _STRING_H
#define _STRING_H

#include <sys/defs.h>

int strcmp(int8_t *string1, int8_t *string2);

int strlen(int8_t *str);

int strcpy(char *s1, char *s2);

int strspt(char * input, char str[][256], char delim);

int strStartsWith(char origStr[], char checkStr[]);

void strconcat(char *first, char *second, char *final);

#endif
