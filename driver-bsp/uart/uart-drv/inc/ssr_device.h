#ifndef SSR_DRIVE_H

#define CMD_CONFIG_BASE_PARA  0x80105201 
#define CMD_SET_MIN           0x80025204 
#define CMD_SET_BREAK         0x80045205 
#define CMD_GET_STATIC        0x80185206
#define CMD_SET_INTERFACE     0x80045208

//user parameter
struct rctty_base_param_s 
{
    unsigned int baud;         /* 波特率 9600 19200…… */
    unsigned int datalen;      /* 字长 5 6 7 8*/
    unsigned int check;        /* 校验 0- NO 1-odd 2-even 3-mark 4-space*/
    unsigned int stopbit;      /* 停止位 1-1,2-2 */           

};

struct rctty_min_param_s 
{
    unsigned char min_time;
    unsigned char min_char;
};

struct rcuart_icount 
{
  unsigned int	rx;
  unsigned int	tx;
  unsigned int	frame;
  unsigned int	overrun;
  unsigned int	parity;
  unsigned int	brk;
  unsigned int  rng;
  unsigned int  dsr;
  unsigned int  dcd;
  unsigned int  cts;
};  


#endif 
