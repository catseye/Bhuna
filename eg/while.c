#include <stdio.h>

int main(int argc, char **argv)
{
	int f = 100;

	while (f > 20) {
		int f = 10;
		printf("%d", f);
	}
	printf("%d", f);
	
	exit(0);
}
