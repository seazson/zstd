#ifndef _LOCAL_BUS_H_
#define _LOCAL_BUS_H_

#include "opl_driver.h"

int oplLocalBusDevInit(void);
int oplLocalBusRead(UINT32 offset, UINT32 nbytes, UINT8 *buf );
int oplLocalBusWrite(UINT32 offset, UINT32 nbytes, UINT8 *buf );
int oplLocalBusWriteField(UINT32 offset, UINT8 field, UINT8 width, UINT8 *data);
int oplLocalBusRelease(void);
int oplLocalBusIrqEnable(void);
int oplLocalBusIrqDisable(void);

#endif/* _LOCAL_BUS_H_ */
