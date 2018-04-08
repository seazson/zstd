/*============================================================================
 *							¼ÓÔØmsp¹Ì¼þ
 *							                   20130529			
 *
============================================================================*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

struct aif_header {
	u32	reserved1[3];
	u32	entry_point_offset;
	u32	reserved2;
	u32	read_only_size;
	u32	read_write_size;
	u32	reserved3;
	u32	zero_init_size;
	u32	reserved4;
	u32	image_base;
	u32	reserved5[2];
	u32	data_base;
	u32	first_region;	/* offset to the first non-root region */
	u32	reserved6[17];
};

struct aif_region {		/* non-root region header */
	u32	next_region;
	u32	load_address;
	u32	size;
	char	name[32];
};

#define MSP_BOTTOM_MEMORY_RESERVED_SIZE	(16*1024*1024) /*sea*/
#define MSP_TOP_MEMORY_RESERVED_SIZE	(1*1024*1024)  /*sea*/

static int check_region(u32 addr, int size, char *name)
{
	if ((addr + size) < MSP_BOTTOM_MEMORY_RESERVED_SIZE)
		return 0;

	printf("Section %s @ 0x%08x (%d bytes) is outside of supported memory ranges\n", name, addr, size);

	return -1;
}

/**
 * check_and_load - check given section to fit valid memory regions and load if it's ok
 *
 * ARGUMENTS:
 *    dst:  memory address of section to load to (destination)
 *    src:  memory address to load from (source), if zero - dst/size filled with zeroes
 *    size: section size in bytes
 *    name: symbolic section name
 *
 * RETURNS:
 *    0 success, -1 otherwise
 */
extern int write_to_voip(unsigned int addr,unsigned int len, char *buf);
static int check_and_load(u32 dst, u32 src, u32 size, char *name)
{
	int i;
	int len,ret;
	char buf[1000];
	if (check_region(dst, size, name))
		return -1;

	printf("%sing %s @ 0x%08x (%u bytes)\n", src ? "Load" : "Clear", name, dst, size);

	if (!src)
	{
		for(i=0;i<size;)
		{
			if( i + 1000 <= size)
			{
				len = 1000;
			}
			else
			{
				len = size - i;
			}
			memset(buf, 0, len);
			ret = write_to_voip(dst,len, buf);
			if(ret != len)
				printf("Need to send %d,but send %d\n",len,ret);
			i += ret;
		}
	}
	else
	{
		for(i=0;i<size;)
		{
			if( i + 1000 <= size)
			{
				len = 1000;
			}
			else
			{
				len = size - i;
			}
			memcpy(buf, (char *)src+i, len);

			ret = write_to_voip(dst+i,len, buf);
			if(ret != len)
				printf("Need to send %d,but send %d\n",len,ret);
			i += ret;
		}
	}
	return 0;
}

static int aif_load_read_only_area(struct aif_header *aif, u32 size)
{
	if (!aif->read_only_size)
		return 0;

	if ((sizeof(*aif) + aif->read_only_size) > size) {
		printf("Read-only area doesn't fit image size\n");
		goto err;
	}

	return check_and_load(aif->image_base, (u32)aif + sizeof(*aif), aif->read_only_size, "read-only area");
 
err:
	return -1;
}

static int aif_load_read_write_area(struct aif_header *aif, u32 size)
{
	u32 addr;

	if (!aif->read_write_size)
		return 0;

	if ((sizeof(*aif) + aif->read_only_size + aif->read_write_size) > size) {
		printf("Read-write area doesn't fit image size\n");
		goto err;
	}

	addr = aif->data_base;
	if (!addr)		/* place just after read-only area, if not specified */
		addr = aif->image_base + aif->read_only_size;

	return check_and_load(addr, (u32)aif + sizeof(*aif) + aif->read_only_size, aif->read_write_size, "read-write area"); 

err:
	return -1;
}

static int aif_load_zero_init(struct aif_header *aif)
{
	u32 addr = aif->data_base, size = aif->zero_init_size;

	if (!size)
		return 0;

	if (!addr)
		addr = aif->image_base + aif->read_only_size;

	addr = addr + aif->read_write_size;

	if (check_and_load(addr, 0, size, "zero-init") < 0)
		return -1;

	return 0;
}

static int aif_load_regions(struct aif_header *aif, u32 size)
{
	struct aif_region *region;
	u32 offset;
	char s[48];

	offset = aif->first_region;

	while (offset > 0) {
		if ((offset + sizeof(*region)) > size)
			goto err0;

		region = (struct aif_region *)((u32)aif + offset);

		if ((offset + sizeof(*region) + region->size) > size)
			goto err0;

		sprintf(s, "region %s", region->name);

		if (region->size > 0) {
        		if (check_and_load(region->load_address, (u32)aif + offset + sizeof(*region), region->size, s))
	        		goto err1;
                } else
                        printf("Skipping region %s\n", region->name);

		offset = region->next_region;
	}

	return 0;

err0:
	printf("Non-root region at offset 0x%08x does not fit image size (%u bytes)\n", offset, size);
err1:
	return -1;
}

/**
 * load_aif_image - simple AIF loader, basic checks and immediate load
 *
 * ARGUMENTS:
 *    addr: memory address of loaded region
 *    size: image size in bytes
 *
 * RETURNS:
 *    0 success, -1 otherwise
 */
	
u32 load_aif_image(u32 addr, u32 size)
{
	struct aif_header *aif = (void *) addr;

	if (size < sizeof(*aif)) {
		printf("No valid AIF header in image\n");
		goto err;
	}
/*
	printf("===========dump head=============\n");
	printf("0x%08x\n",aif->data_base);
	printf("0x%08x\n",aif->entry_point_offset);
	printf("0x%08x\n",aif->first_region);
	printf("0x%08x\n",aif->image_base);
	printf("0x%08x\n",aif->read_only_size);
	printf("0x%08x\n",aif->read_write_size);
	printf("0x%08x\n",aif->zero_init_size);
*/	
	if (aif_load_read_only_area(aif, size))
		goto err;

	if (aif_load_read_write_area(aif, size))
		goto err;

	if (aif_load_zero_init(aif))
		goto err;

	if (aif_load_regions(aif, size))
		goto err;

	return aif->image_base + aif->entry_point_offset;

err:
	return (u32)-1;
}
