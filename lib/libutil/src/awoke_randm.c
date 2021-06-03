#include <stdlib.h>
#include <stdio.h>
#include <time.h>


int awoke_random_int(int max, int min)
{
	srand((unsigned)time(NULL));
	return ((rand()%(max-min))+min);
}
