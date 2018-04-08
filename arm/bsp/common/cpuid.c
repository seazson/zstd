#include "common.h"
/*************************************************************
*  
*	获取cpuid和缓存类型
*
*/
int get_cpuid(void)
{
	int id=0;
	asm("mrc p15,0,%0,c0,c0,0":"=r"(id)::"cc");
	return id;
}

void show_cpuid(void)
{
	int id = get_cpuid();
	printf("CPUID :  \t0x%x\n",id);
}

int get_cacheid(void)
{
	int id=0;
	asm("mrc p15,0,%0,c0,c0,1":"=r"(id)::"cc");
	return id;
}

void show_cacheinfo(void)
{
	int id = get_cacheid();
	int m = (id >> 2) & 0x1;
	int size=0;
	int num = 0;
	int type =0;
	
	printf("CACHEID :\t0x%x\n",id);
	if(id & 0x01000000)
	{
		printf("support DCACHE and ICACHE\n\n");
		printf("\n----ICACHE----\n\n");
	}
	else
	{
		printf("support CACHE\n\n");
		printf("----CACHE----\n\n");
	}
	
	/*cache size*/
	switch((id >> 6) & 0x7)
	{
		case 0b000 : 
			if(m==1)	size = 750;
			else size = 500; break;
		case 0b001 : 
			if(m==1)	size = 1536;
			else size = 1024; break;
		case 0b010 : 
			if(m==1)	size = 3*1024;
			else size = 2*1024; break;
		case 0b011 : 
			if(m==1)	size = 6*1024;
			else size = 4*1024; break;
		case 0b100 : 
			if(m==1)	size = 12*1024;
			else size = 8*1024; break;
		case 0b101 : 
			if(m==1)	size = 24*1024;
			else size = 16*1024; break;
		case 0b110 : 
			if(m==1)	size = 48*1024;
			else size = 32*1024; break;
		case 0b111 : 
			if(m==1)	size = 96*1024;
			else size = 64*1024; break;
	}
	printf("cahe size        :\t0x%x\n",size);
	/*cache type*/
	switch((id >> 3) & 0x7)
	{
		case 0b000 : 
			if(m==1)	type = 0;
			else type = 1; break;
		case 0b001 : 
			if(m==1)	type = 3;
			else type = 2; break;
		case 0b010 : 
			if(m==1)	type = 6;
			else type = 4; break;
		case 0b011 : 
			if(m==1)	type = 12;
			else type = 8; break;
		case 0b100 : 
			if(m==1)	type = 24;
			else type = 16; break;
		case 0b101 : 
			if(m==1)	type = 48;
			else type = 32; break;
		case 0b110 : 
			if(m==1)	type = 96;
			else type = 64; break;
		case 0b111 : 
			if(m==1)	type = 192;
			else type = 128; break;
	}
	printf("cahe ways        :\t0x%x\n",type);

	switch(id  & 0x3)
	{
		case 0b00 : 
				num = 2; break;
		case 0b01 : 
				num = 4; break;
		case 0b10 : 
				num = 8; break;
		case 0b11 : 
				num = 16; break;
	}		
	printf("cache line(word) : \t0x%x\n",num);

	/*DCACHE */
	if(id & 0x01000000)
	{
		printf("\n----DCACHE----\n\n");
		/*cache size*/
		switch((id >> 18) & 0x7)
		{
			case 0b000 : 
				if(m==1)	size = 750;
				else size = 500; break;
			case 0b001 : 
				if(m==1)	size = 1536;
				else size = 1024; break;
			case 0b010 : 
				if(m==1)	size = 3*1024;
				else size = 2*1024; break;
			case 0b011 : 
				if(m==1)	size = 6*1024;
				else size = 4*1024; break;
			case 0b100 : 
				if(m==1)	size = 12*1024;
				else size = 8*1024; break;
			case 0b101 : 
				if(m==1)	size = 24*1024;
				else size = 16*1024; break;
			case 0b110 : 
				if(m==1)	size = 48*1024;
				else size = 32*1024; break;
			case 0b111 : 
				if(m==1)	size = 96*1024;
				else size = 64*1024; break;
		}
		printf("cahe size        :\t0x%x\n",size);
		/*cache type*/
		switch((id >> 15) & 0x7)
		{
			case 0b000 : 
				if(m==1)	type = 0;
				else type = 1; break;
			case 0b001 : 
				if(m==1)	type = 3;
				else type = 2; break;
			case 0b010 : 
				if(m==1)	type = 6;
				else type = 4; break;
			case 0b011 : 
				if(m==1)	type = 12;
				else type = 8; break;
			case 0b100 : 
				if(m==1)	type = 24;
				else type = 16; break;
			case 0b101 : 
				if(m==1)	type = 48;
				else type = 32; break;
			case 0b110 : 
				if(m==1)	type = 96;
				else type = 64; break;
			case 0b111 : 
				if(m==1)	type = 192;
				else type = 128; break;
		}
		printf("cahe ways        :\t0x%x\n",type);

		switch((id >>12) & 0x3)
		{
			case 0b00 : 
					num = 2; break;
			case 0b01 : 
					num = 4; break;
			case 0b10 : 
					num = 8; break;
			case 0b11 : 
					num = 16; break;
		}		
		printf("cache line(word) : \t0x%x\n",num);
	}

}
