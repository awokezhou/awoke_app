#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/wait.h>  
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/sem.h>



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_macros.h"
#include "awoke_log.h"
#include "awoke_vector.h"
#include "awoke_string.h"


#define STR_ALARM_A	"43"
#define STR_ALARM_B	"sensor_horizontal_adjust"
#define T_ALARM_A 8
#define T_ALARM_B 12

int get_alarm_type(char *buff)
{
	char s[]="sensor_horizontal_adjust#";
	char *delim="#";
	char *type;
	char *data;
	
	type = strtok(s, delim);
	printf("type %s\n", type);
	data = strtok(NULL, delim);
	printf("data %s\n", data);

	if (!strcmp(STR_ALARM_A, type))
		return T_ALARM_A;
	else if (!strcmp(STR_ALARM_B, type))
		return T_ALARM_B;
	return 0;
}

err_type vector_file_read(vector *vec)
{
	int fd;
	int len;
	char buff[1024] = {0x0};
	
	fd = open("vector_file", O_RDONLY);
	if (fd < 0) {
		log_err("open file error");
		return et_file_open;
	}

	len = read(fd, buff, sizeof(buff));
	if (len < 0) {
		close(fd);
		return et_file_read;
	}

	sscanf(buff, "%lf,%lf,%lf", &vec->x, &vec->y, &vec->z);
	log_debug("buff %lf, %lf, %lf", vec->x, vec->y, vec->z);
	close(fd);
	return et_ok;	
}

err_type vector_file_write(vector *vec)
{
	int fd;
	int len;
	char buff[1024] = {"\n"};

	fd = open("vector_file", O_WRONLY|O_CREAT);
	if (fd < 0) {
		log_err("open file error");
		return et_file_open;
	}

	sprintf(buff, "%lf, %lf, %lf", vec->x, vec->y, vec->z);
	log_debug("buff %s", buff);
	len = write(fd, buff, 1024);
	if (len < 0) {
		log_err("write file error");
		return et_file_send;
	}

	close(fd);
	return et_ok;
}

int main(int argc, char **argv)
{
	/*
	vector vec;

	vec.x = awoke_random_int(52, 35);
	vec.y = awoke_random_int(5, 7);
	vec.z = awoke_random_int(7, 93);

	vector_file_write(&vec);
	vector_file_read(&vec);
	*/

	uint8_t state[2];

	uint8_t states = 0x35;

	state[0] = 0x35;
	state[1] = 0x10;

	log_debug("init");

	if ((0 != state[0]&0xfe) || (0 != (state[1]&0x40)))
	{
		log_err("state err");
	}

	if (0 != states&0xfe)
	{
		log_err("states err");
	}

	log_debug("exit");

	log_debug("be32 %d", sizeof(__be32));
	
	return 0;
}

