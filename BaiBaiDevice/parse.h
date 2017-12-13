
#ifndef MORSE_PARSE_H
#define MORSE_PARSE_H

#define MBUFLEN 16
static int mbuf[MBUFLEN];
static int mbuf_len = 0;
char morse_parse(int on, int ms)
{
	char c = 0;
	const int WPM = 10;
	const int T = 1200 / WPM;
	if (!on) {
		if (ms > T * 2)
			c = ' ';
	}
	else {
		if (ms < T)
			c = '.';
		else
			c = '-';
	}
	return c;
}

#endif // MORSE_PARSE_H
