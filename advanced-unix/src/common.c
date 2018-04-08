#include <stdio.h>
#include <stdlib.h>

static char erase_seq[] = "\b \b";		/* erase sequence	*/

int readline (char buf[])
{
	char   *p = buf;
	char   c;
	int	   num = 0;				/* buffer index		*/
//	int	   cur = 0;			/* prompt length	*/
	int    machine=0;
//	int    i;

	/* print prompt */
	for (;;)
    {
		c = getc(stdin);

		if(c < 0x20)
		{
			printf(erase_seq);
			printf(erase_seq);
		}

		if(machine > 0)
		{
			switch(machine)
			{
				case 1 :
						if('[' == c)
						{
							printf(erase_seq);
							machine = 2;
							continue;
						}
						break;
				case 2 :
						printf (erase_seq);
						switch(c)
						{
							case 'A' : 
									   c = 0x10;
									   break;
							case 'B' : 
									   c = 0x10;
									   break;
							case 'C' :
									   c = 0x10;
									   break;
							case 'D' : 
									   c = 0x11;
									   break;
							default : break;
						}
						machine = 0;
						break;
				default : machine = 0; 
					break;
			}
		}
		/*
		 * Special character handling
		 */
		switch (c)
        {		
        case 0x1b:				/*^[ means control charcter perss*/
				machine = 1;
				break;

		case 0x00:				/* nul			*/
			continue;

		case 0x03:				/* ^C - break	*/
			buf[0] = '\0';		/* discard input */
			return (-1);

		case '\r':				/* Enter		*/
		case '\n':
			*p = '\0';
			printf("\r\n");
			return (p - buf);


		case 0x15:				/* ^U - erase line	*/
			continue;

		case 0x08:				/* ^H  - backspace	*/
		case 0x7F:				/* DEL - backspace	*/
			if(num>0)
			{
				p--;
				num--;
			}
			printf (erase_seq);
			continue;

		case 0x10:
		case 0x11:
			continue;
		
  		default:
			if (num < 256 - 2)
            {
	    		*p++ = c;
				++num;
			}
            else
            {           /* Buffer full      */
				putc ((int)'\a', stdout);
			}
		}
	}
}

int get_cmdline(char buf[])
{
	int size;
#if !defined(ISCOM4002_NP)	
	system("stty raw");        //one chae mode
    size = readline(buf);
	system("stty cooked");     //one line mode
#else
	system("busybox stty raw");        //one chae mode
    size = readline(buf);
	system("busybox stty cooked");     //one line mode
#endif

	return size;
}

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
 //#error ####################
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
    
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) 
	{
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}


