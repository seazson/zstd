#ifndef _TTY_FLIP_H
#define _TTY_FLIP_H

static inline void tty_insert_flip_char(struct rctty_struct *tty,
				   unsigned char ch, char flag)
{
	if (tty->flip.count < TTY_FLIPBUF_SIZE) 
    {
		tty->flip.count++;
		*tty->flip.flag_buf_ptr++ = flag;
		*tty->flip.char_buf_ptr++ = ch;
	}
}

#endif

