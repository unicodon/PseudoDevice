#ifndef KEYMAP_H
#define KEYMAP_H

typedef struct keymap_t
{
	char c; // 'c'
	const char* s; // "-.-."
} keymap_t; 

static keymap_t keymap[] =
{
{ 'a', ".-" },
{ 'b', "-..." }, 
{ 'c', "-.-." }, 
{ 'd', "-.." }, 
{ 'e', "." }, 
{ 'f', "..-." }, 
{ 'g', "--." }, 
{ 'h', "...." }, 
{ 'i', ".." }, 
{ 'j', ".---" }, 
{ 'k', "-.-" }, 
{ 'l', ".-.." }, 
{ 'm', "--" }, 
{ 'n', "-." }, 
{ 'o', "---" }, 
{ 'p', ".--." }, 
{ 'q', "--.-" }, 
{ 'r', ".-." }, 
{ 's', "..." }, 
{ 't', "-" }, 
{ 'u', "..-" }, 
{ 'v', "...-" }, 
{ 'w', ".--" }, 
{ 'x', "-..-" }, 
{ 'y', "-.--" }, 
{ 'z', "--.." }, 

{ '0', "-----" }, 
{ '1', ".----" }, 
{ '2', "..---" }, 
{ '3', "...--" }, 
{ '4', "....-" }, 
{ '5', "....." }, 
{ '6', "-...." }, 
{ '7', "--..." }, 
{ '8', "---.." }, 
{ '9', "----." }, 
}; 


#define MAX_CODE_LEN 5

static keymap_t* keymap_l1[ 1 << 1 ];
static keymap_t* keymap_l2[ 1 << 2 ];
static keymap_t* keymap_l3[ 1 << 3 ];
static keymap_t* keymap_l4[ 1 << 4 ];
static keymap_t* keymap_l5[ 1 << 5 ];

static keymap_t** keydict[MAX_CODE_LEN] = 
{
	keymap_l1,
	keymap_l2,
	keymap_l3,
	keymap_l4,
	keymap_l5,
};

static void init_keymap(void)
{
	int i,j;
	int n;
	int code;
	int len;	

	n = 2;
	for (i = 0; i < MAX_CODE_LEN; i++) {
		for (j = 0; j < n; j++) {
			keydict[i][j] = NULL;
		}
		n <<= 1;
	}

	n = sizeof(keymap) / sizeof(keymap[0]);
	for (i = 0; i < n; i++) {
		len = strlen(keymap[i].s);
		if (len > MAX_CODE_LEN) {
			continue;
		}
		code = 0;
		for (j = 0; j < len; j++) {
			code <<= 1;
			if (keymap[i].s[j] == '-')
				code |= 1;
		}
		printk( KERN_INFO "%s %c %d\n", keymap[i].s, keymap[i].c, code);
		keydict[len-1][code] = &keymap[i];
	}
}

#endif // KEYMAP_H
