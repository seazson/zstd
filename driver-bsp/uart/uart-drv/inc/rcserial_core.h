#ifndef _SERIALCORE_H
#define _SERIALCORE_H

int uart_open(struct rctty_struct *tty, struct file *filp);

void uart_close(struct rctty_struct *tty, struct file *filp);

int uart_write(struct rctty_struct *tty, const unsigned char * buf, int count);

#endif

