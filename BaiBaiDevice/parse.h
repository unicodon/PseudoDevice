
#ifndef MORSE_PARSE_H
#define MORSE_PARSE_H

#define MBUFLEN 16
static int mbuf[MBUFLEN];
static int mbuf_len = 0;
char morse_parse(int on, int ms)
{
//	if (on)	printk( KERN_INFO "ms %d\n", ms);
	char c = 0;
	if (!on) {
		if (ms > 250)
			c = ' ';
	}
	else {
		if (ms < 150)
			c = '.';
		else
			c = '-';
	}
	return c;
}

#endif // MORSE_PARSE_H
