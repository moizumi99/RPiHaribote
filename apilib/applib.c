#include <stdarg.h>
#include <stdint.h>

unsigned int x = 1;
	
unsigned int srand(unsigned int seed)
{
	x = seed;
}

int rand()
{
	x = x*1103515245 + 12345;
	x = x % 2147483647;
	return x & 0x7FFFFFFF;
}

void log_error(int i)
{
	return;
}

#define STR_GUARD 1024

int vsprintf(char *str, const char *format, va_list listPointer)
{
	char c;
	int i, si;
	unsigned int ui;
	int pf;
	char *s;
	int len, fill, sign_flag;
	
	i=0;
	pf = 0;
	len = 0;
	fill = 0;
	while((c=*format++)!=0 && i<STR_GUARD) {
		if (pf==0) {
			// after regular character
			if (c=='%') {
				pf=1;
				len = 0;
				fill = 0;
			} else {
				str[i++]=c;
			}
		} else if (pf>0) {
			// previous character was '%'
			if (c=='x' || c=='X') {
				ui = va_arg(listPointer, unsigned);
				i += strHex(str+i, (unsigned) ui, len, fill);
				pf=0;
			} else if (c=='u' || c=='U' || c=='i' || c=='I') {
				ui = va_arg(listPointer, unsigned);
				i += strNum(str+i, (unsigned) ui, len, fill, 0);
				pf=0;
			} else if (c=='d' || c=='D') {
				si = va_arg(listPointer, int);
				if (si<0) {
					ui = -si;
					sign_flag = 1;
				} else {
					ui = si;
					sign_flag = 0;
				}
				i += strNum(str+i, (unsigned) ui, len, fill, sign_flag);
				pf=0;
			} else if (c=='s' || c=='S') {
				s = va_arg(listPointer, char *);
				i += strcpy(str+i, s);
				pf=0;
			} else if ('0'<=c && c<='9') {
				if (pf==1 && c=='0') {
					fill = 1;
				} else {
					len = len*10+(c-'0');
				}
				pf=2;
			} else {
				// this shouldn't happen
				str[i++]=c;
				pf=0;
			}
		}
	}
	str[i]=0;
	if (i>STR_GUARD) {
		log_error(1);
	}
	return (i>0) ? i : -1;
}

int sprintf(char *str, const char *format, ...)
{
	va_list ap;
	int i;

	va_start(ap, format);
	i = vsprintf(str, format, ap);
	va_end(ap);
	return i;
}

uint32_t u32_div(uint32_t n, uint32_t d)
{
	signed int q;
	uint32_t bit;

	q = 0;
	if (d==0) {
		q=0xFFFFFFFF;
	} else {
		bit = 1;
		while(((d & 0x80000000) == 0) && (d<n)) {
			d <<=1;
			bit <<=1;
		}
		
		while(bit>0) {
			if (n>=d) {
				n -= d;
				q += bit;
			}
			d   >>= 1;
			bit >>= 1;
		}
	}

	return q;
}


int strHex(char *str, unsigned int ad, int len, int fill)
{
	int i, j=0;
	int c;
	char s[1024];
	int st;

	st = 0;
	for(i=0; i<8; i++) {
		if ((c = ((ad>>28) & 0x0F))>0) {
			c = (c<10) ? (c+'0') : (c+'a'-10);
			s[j++] = (char) c;
			st = 1;
		} else if (st || i==7) {
			s[j++] = '0';
		}
		ad = ad << 4;
	}
	s[j]=0;
	for(i=0; i<len-j; i++) {
		str[i] = (fill) ? '0' : ' ';
	}
	strcpy(str+i, s);
	
	return j+i;
}

int strNum(char *str, unsigned int ui, int len, int fill, int sflag)
{
	unsigned int cmp = 1;
	int i, j;
	int d;
	int l;
	char c;
	char s[1024];

	cmp=1;
	l=1;
	//	printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
	while(cmp*10<=ui && l<10) {
		//2^32 = 4.29 * 10^9 -> Max digits =10 for 32 bit
		cmp *=10;
		l++;
		//		printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
	}

	j = 0;
	if (sflag) {
		s[j++]='-';
		l++; // add one space for '0'
	}
	//	printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
	while(j<l) {
		d = u32_div(ui, cmp);
		c = (char) (d+'0');
		s[j++] = c;
		ui = ui - d*cmp;
		cmp = u32_div(cmp, 10);
		//		Printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
	}
	s[j]=0;
	for(i=0; i<len-l; i++) {
		str[i] = (fill) ? '0' : ' ';
	}
	strcpy(str+i, s);
	
	return j+i;
}

int strcpy(char *dst, char const *src)
{
	int i;
	i = 0;
	while(*src!=0) {
		*(dst++) = *(src++);
		i++;
	} 
	*dst = 0;
	return i;
}


int strlen(const char* str)
{
	int ret = 0;
	while ( str[ret] != 0 ) {
		ret++;
	}
	return ret;
}

int cton(char c)
{
	if ('0'<=c && c<='9') {
		return c-'0';
	} else if ('A'<=c && c<='Z') {
		return c-'A'+10;
	} else if ('a'<=c && c<='z') {
		return c-'a'+10;
	} else {
		return -1;
	}
}
 long int strtol(const char *str, char **endptr, int base)
{
	long int r = 0;
	if (base==0) {
		base=10;
	} else if (base<2 || 36<base) {
		*endptr = (char *) str;
		return 0;
	}
	while(0<= cton(*str) && cton(*str)<base) {
		r = r*10 + cton(*str);
		str++;
	}
	*endptr = (char *) str;
	return r;
}

