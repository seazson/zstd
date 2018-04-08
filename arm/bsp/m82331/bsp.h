#ifndef __BSP_H__
#define __BSP_H__

void serial_setbrg(void);
int serial_init(void);
void serial_putc(const char c);
void serial_puts(const char *s);
int serial_getc(void);
int serial_tstc(void);

#endif