#include<stdio.h>
#include <sys/stat.h>

#define TOLOWER(x) ((x) | 0x20)

static unsigned int simple_guess_base(const char *cp)
{
	if (cp[0] == '0') {
		if (TOLOWER(cp[1]) == 'x' && isxdigit(cp[2]))
			return 16;
		else
			return 8;
	} else {
		return 10;
	}
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0;

	if (!base)
		base = simple_guess_base(cp);

	if (base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'x')
		cp += 2;

	while (isxdigit(*cp)) {
		unsigned int value;

		value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
		if (value >= base)
			break;
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;
	return result;
}

unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;	
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}

int main(int argc, char *argv[])
{
	int filesize=0;
	FILE *fp;
	int i;
	unsigned char style=0;
	
	if(argc <= 2 || argc > 4)
	{
		printf("Usega : fillfile filename 0x** [filesize]\n");
		return 1;
	}
	else if(argc == 3)
	{
		filesize = get_file_size(argv[1]);
	}
	else if(argc == 4)
	{
		filesize = simple_strtoul(argv[3], NULL, 0);
	}

	if(filesize == -1)
	{
		printf("Error: Filesize = 0x%x\n",filesize);
		return 1;
	}
	
	style=simple_strtoul(argv[2], NULL, 0);
	if((fp = fopen(argv[1], "wb+")) == NULL)
	{
		printf("Error: Open %s\n",argv[1]);
		return 1;
	}
		
	for(i = 0; i < filesize; i++)
	{
		fputc(style,fp);
	}
	fclose(fp);

	printf("Fill %s[0x%x] with 0x%02x\n",argv[1],filesize,style);

	return 0;
}
