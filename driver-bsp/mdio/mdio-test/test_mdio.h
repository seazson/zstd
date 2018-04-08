#ifndef __MDIO_H__
#define __MDIO_H__

extern int drv_mdio_open(void);
extern int drv_mdio_close(void);
extern int drv_mdio_read(unsigned short	 phyAddr, unsigned short regAddr, unsigned short* value);
extern int drv_mdio_write(unsigned short phyAddr, unsigned short regAddr, unsigned short value);

#endif
