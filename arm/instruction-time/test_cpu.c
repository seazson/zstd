/*******************************************************************************************************
 * 这是一个通用测试程序，测试内容包括：
 * 		整数运算性能   ：+，-，*，/
 * 		浮点数运算性能 ：+，-，*，/
 *      装载速度		：cache，sdram，flash
 *      写入速度      ：cache，sdram，
 *      算法性能测试   ：
 * 
 * 
 * */

#define SDRAM_SPACE (*(volatile unsigned long *)0x30000000)
#define REG_SPACE   (*(volatile unsigned long *)0x4A00001c)
#define FLASH_SPACE (*(volatile unsigned long *)0x00000000)

/*******************************************************************************************************
 * 整数运算测试
 * 
 */
void int_add(void)
{
	unsigned int i=0;
	unsigned int sum=0;
	unsigned int num1;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = num1 + i;		
	}
	sum = timer0_read();
	printf("int_add timer is %d\r\n",30000-sum);
}

void int_des(void)
{
	unsigned int i=0;
	unsigned int sum=0;
	unsigned int num1=1000;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = num1 - i;		
	}
	sum = timer0_read();
	printf("int_des timer is %d\r\n",30000-sum);
}

void int_multi(void)
{
	unsigned int i=0;
	unsigned int sum=0;
	unsigned int num1;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = num1*i;		
	}
	
	sum = timer0_read();
	printf("int_multi timer is %d\r\n",30000-sum);
}

void int_div(void)
{
	unsigned int i=0;
	unsigned int sum=0;
	unsigned int num1=100;

	timer0_begin();
	for(i=1;i<=100;i++)
	{
		sum = num1/2;		
	}
	
	sum = timer0_read();
	printf("int_div timer is %d\r\n",30000-sum);
}
/*******************************************************************************************************
 * float运算测试
 * 
 
void float_add(void)
{
	unsigned int i=0;
	double sum=0.0;
	double num1=1000;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = num1 + i;		
	}
	sum = timer0_read();
	printf("float_add timer is %d\r\n",30000-sum);
}

void float_des(void)
{
	unsigned int i=0;
	double sum=0.0;
	double num1=1000;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = num1 - i;		
	}
	sum = timer0_read();
	printf("float_des timer is %d\r\n",30000-sum);
}

void float_multi(void)
{
	unsigned int i=0;
	double sum=0.0;
	double num1=1000;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = num1*i;		
	}
	
	sum = timer0_read();
	printf("float_multi timer is %d\r\n",30000-sum);
}

void float_div(void)
{
	unsigned int i=0;
	double sum=0.0;
	double num1=1000;

	timer0_begin();
	for(i=1;i<=100;i++)
	{
		sum = num1/1;		
	}
	
	sum = timer0_read();
	printf("float_div timer is %d\r\n",30000-sum);
}*/
/*******************************************************************************************************
 * 加载储存测试
 * 
 */
void load_test(void)
{
	unsigned int i=0;
	unsigned long sum=0;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = SDRAM_SPACE;		
	}
	sum = timer0_read();
	printf("load at sdram timer is %d\r\n",30000-sum);
	
	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = REG_SPACE;		
	}
	sum = timer0_read();
	printf("load at reg timer is %d\r\n",30000-sum);

	timer0_begin();
	for(i=0;i<100;i++)
	{
		sum = FLASH_SPACE;		
	}
	sum = timer0_read();
	printf("load at flash timer is %d\r\n",30000-sum);
}

void store_test(void)
{
	unsigned int i=0;
	unsigned long sum=0;

	timer0_begin();
	for(i=0;i<100;i++)
	{
		SDRAM_SPACE = sum;		
	}
	sum = timer0_read();
	printf("store at sdram timer is %d\r\n",30000-sum);

	timer0_begin();
	for(i=0;i<100;i++)
	{
		REG_SPACE = sum;		
	}
	sum = timer0_read();
	printf("store at reg timer is %d\r\n",30000-sum);
	
}
