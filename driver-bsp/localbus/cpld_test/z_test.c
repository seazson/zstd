/************************************************************************/
//
//
//					This is a common test			
//
//
/************************************************************************/
#include <stdio.h>
#include<stdlib.h>

struct menu_entry 
{
    char * menu_name;
    int (*func)(int arg);
	int arg;
};

char console_buffer[256];		/* console I/O buffer	*/
int number_of_pots = 2;
static char erase_seq[] = "\b \b";		/* erase sequence	*/


/*************************************************************************/
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
	system("stty raw");        //one chae mode
    size = readline(buf);
	system("stty cooked");     //one line mode

	return size;
}

/*************************************************************************/
extern int cpld_init(void);
extern int cpld_sysled(char value);
extern int cpld_watchdog(char value);
extern int cpld_switch_reset(char value);
extern int cpld_dsp_reset(char value);
extern int cpld_slic1_reset(char value);
extern int cpld_slic2_reset(char value);
extern int cpld_hd_switch(char value);
extern int cpld_pon1_power(char value);
extern int cpld_pon2_power(char value);

struct menu_entry menu_list[] =
{
    {"ALL", cpld_init, 0},
    {"sys_led_on", cpld_sysled, 1},
    {"sys_led_off", cpld_sysled, 0},
    {"watch_dog_on", cpld_watchdog, 1},
    {"watch_dog_off", cpld_watchdog, 0},
    {"cpld_switch_reset", cpld_switch_reset, 1},
    {"cpld_switch_unreset", cpld_switch_reset, 0},
    {"cpld_dsp_reset", cpld_dsp_reset, 1},
    {"cpld_dsp_unreset", cpld_dsp_reset, 0},
    {"cpld_slic1_reset", cpld_slic1_reset, 1},
    {"cpld_slic1_unreset", cpld_slic1_reset, 0},
    {"cpld_slic2_reset", cpld_slic2_reset, 1},
    {"cpld_slic2_unreset", cpld_slic2_reset, 0},
    {"cpld_hd_switch_on", cpld_hd_switch, 1},
    {"cpld_hd_switch_off", cpld_hd_switch, 0},
    {"cpld_pon1_power_on", cpld_pon1_power, 0},
    {"cpld_pon1_power_off", cpld_pon1_power, 1},
    {"cpld_pon2_power_on", cpld_pon2_power, 0},
    {"cpld_pon2_power_off", cpld_pon2_power, 1},

		
    {NULL, NULL, NULL}
};

#include <signal.h>
static void sigusr2(int signr)
{
	printf("handle singnaeld \n");
}


int main(int args, char *argc[])
{
  	int i, ret, len;
    int index;
	struct sigaction	act;


	act.sa_handler = sigusr2;
	memset(&act.sa_mask, 0, sizeof(act.sa_mask));
	act.sa_flags = SA_RESTART;
	act.sa_restorer = 0;
	sigaction(SIGUSR2, &act, NULL);

	cpld_init();

    while(1)
    {
        i = 0;
		cpld_pon_stat(&ret);
        while(menu_list[i].menu_name)
        {
/*			if(0 == i)
			{
				printf("%2d.  %s(%d~%d)\r\n", (i + 1), menu_list[i].menu_name,ALL_TEST_START,ALL_TEST_END);
			}
			else */
			{
				printf("%2d.  %s\r\n", (i + 1), menu_list[i].menu_name);
			}
			i++;
        }

		printf("Please Input Index: ");
        len = get_cmdline(console_buffer);

		if(-1 == len)
		{
			printf("\n\n");
			return 0;
		}
		index = atol(console_buffer);
        if( index > i || index == 0 )
            continue;

        if( menu_list[index - 1].func != NULL )
        {
            printf("%s Testing\r\n", menu_list[index - 1].menu_name);
            ret = (*menu_list[index - 1].func)(menu_list[index - 1].arg);
        }
    }

	return 0;
}




