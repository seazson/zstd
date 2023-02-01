#include <stdio.h>

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

/* Obtain a backtrace and print it to stdout. */
void
print_trace (void)
{
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {

    printf ("Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++)
      printf ("%s\n", strings[i]);
  }

  free (strings);
}
int c[16];
int add(int a, int b);
int sub(int a, int b);


int d1(int a, int b)
{
	while(1);
	return a+b;
}

int d2(int a, int b)
{
	return d1(a,b) + b;
}

int d3(int a, int b)
{
	return d2(a,b) + a;
}

int d4(int a, int b)
{
	return d3(a,b) * a;
}

int d5(int a, int b)
{
	return d4(a,b) * a;
}

void do_stuff(int my_arg)
{
    int my_local = my_arg + 2;
    int i;

    for (i = 0; i < my_local; ++i)
        printf("i = %d\n", d5(i, i));
}

int main()
{
    do_stuff(2);
	c[0] = add(1, 5);
    return 0;

}
