#ifndef MORSE_PARSE_H
#define MORSE_PARSE_H

#include "keymap.h"

#define MBUFLEN 16
static unsigned my_code = 0;
static int my_code_len = 0;

static char morse_flush(void)
{
	char c = 0;
	keymap_t* k;

	if (my_code_len > 0 && my_code_len <= MAX_CODE_LEN) {
		k = keydict[my_code_len - 1][my_code];
		if (k) {
			c = k->c;
		}
	}

	my_code_len = 0;
	my_code = 0;
	return c;
}

static char morse_parse(int on, int ms)
{
#if 1
	const int C = 5;
	if (!on) {
		if (ms > 250 * C)
			return morse_flush();
	}
	else {
		my_code_len++;
		my_code <<= 1;
		if (ms > 150 * C) {
			my_code |= 1; 
		}
		printk( KERN_INFO "code %d, len %d\n", my_code, my_code_len);
	}
	return 0;
	
#else
	const int C = 5;
	//if (on)	printk( KERN_INFO "ms %d\n", ms);
	char c = 0;
	if (!on) {
		if (ms > 250 * C)
			c = ' ';
	}
	else {
		if (ms < 150 * C)
			c = '.';
		else
			c = '-';
	}
	return c;
#endif
}

#endif // MORSE_PARSE_H
