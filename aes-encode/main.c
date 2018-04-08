#include <stdio.h>
#include "Aes.h"

unsigned char buf[131072];
CAesCbc g_aes;

//cbc向量 
unsigned char g_iv[]={0xa3, 0x3e, 0x15, 0xb1, 0x94, 0x8, 0xc8, 0xef, 
                            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
//key
unsigned char g_key[]={0x1e, 0x3b, 0xec, 0x8c, 0x31, 0xb7, 0x62, 0xe3, 
                       0x38, 0xf6, 0xef, 0x4a, 0xec, 0xf1, 0x53, 0xe9, 
                       0xcc, 0xf7, 0xaa, 0x11, 0x38, 0x84, 0x58, 0x26, 
                       0x5b, 0xaf, 0x1a, 0x51, 0x34, 0x4f, 0xfe, 0x83};

int main(int argc,char *args[])
{
	FILE *fp_src;
	FILE *fp_des;
	unsigned long size;
	unsigned long filesize;
	unsigned long i;
	unsigned long percent;
	int decode=0;
//check
	if(argc < 3)
	{
		printf("error args aes -e/d src_file dest_file\n");
		return -1;
	}
	if(args[1][1] == 'd')
	{
		printf("***********Decode File***********\n");
		decode=1;
	}
	else if(args[1][1] == 'e')
	{
		printf("***********Encode File***********\n");
		decode=0;
	}
	else
	{
			printf("unkown args\n");
			return -1;	
	}	
//open file	
	fp_src = fopen(args[2],"rb");
	if(fp_src == NULL)
	{
		printf("[error]:open input file error\n");
		return -1;
	}

	fp_des = fopen(args[3],"wb+");
	if(fp_des == NULL)
	{
		printf("[error]:open output file error\n");
		return -1;
	}
//get input file size
    fseek(fp_src, 0L, SEEK_END);
    filesize = ftell(fp_src);  //不能超过2g，否则不正确 
    fseek(fp_src, 0L, SEEK_SET);
    printf("Input  File  : %s\n",args[2]);
    printf("Output File  : %s\n",args[3]);
    printf("File   Size  : %ldB [%ldMB]\n",filesize,filesize/1024/1024);
	printf("*********************************\n");

//now do it
	AesGenTables();			    //初始化Aes表	
	AesCbc_Init(&g_aes,g_iv);   //初始化cbc向量表
	
	if(decode == 0)
	{
			Aes_SetKeyEncode(&g_aes.aes,g_key,32);     //根据256bits key 生成aes相关结构数据
	}
	else
	{
			Aes_SetKeyDecode(&g_aes.aes,g_key,32);
	}
	
	printf("Process      :          ");
	percent=0;
	for(i=0; i < filesize; )
	{
//process print
		unsigned long tmp = (i/1024)*100/(filesize/1024);
		if(percent != tmp)
		{
			percent = tmp;
			printf("\b \b\b \b\b \b");
			printf("%02d%%",percent);
			fflush(stdout);
		}
		
		size = fread(buf,1,131072,fp_src);
		if(size ==0)
            break;
		if(decode == 0)
		{
				AesCbc_Encode(&g_aes, buf, size);		//加密
		}
		else
		{
				AesCbc_Decode(&g_aes, buf, size);       //解密
		}
		fwrite(buf,1,size,fp_des);
		i+=size;
	}
	
//finish	
	printf("\n*************Finish**************\n");
	fclose(fp_des);
	fclose(fp_src);	
	return 0;	
}
