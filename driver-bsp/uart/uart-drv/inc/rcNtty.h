#ifndef _NTTY_H
#define _NTTY_H

ssize_t read_chan(struct rctty_struct *tty, struct file *file, unsigned char __user *buf, size_t nr);
ssize_t write_chan(struct rctty_struct * tty, struct file * file, const unsigned char * buf, size_t nr);
void n_tty_receive_buf(struct rctty_struct *tty, const unsigned char *cp, unsigned char *fp, int count);

#endif

