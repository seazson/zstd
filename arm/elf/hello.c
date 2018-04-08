int g_a=0;
int g_b=2;
int g_c;

int g_add_a;
int add(int a,int b);

int main()
{
	int c = add(&g_a, &g_b) + g_add_a;
	int d = add(&g_a, &g_c) + g_add_a;
}
