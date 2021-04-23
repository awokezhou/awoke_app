
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<unistd.h>  
#include<sys/types.h>  
#include<sys/stat.h>  
#include<fcntl.h>  
#include<termios.h>  
#include<errno.h>

#include "cmder.h"
#include "uart.h"


int baudrate_arr[] = {
	B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, 
	B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
};

int baudrate_name_arr[] = { 
	115200, 38400,  19200,  9600,  4800,  2400,  1200,  300, 
	115200, 38400,  19200,  9600,  4800,  2400,  1200,  300,
};

err_type uart_set_baudrate(int fd, int rate)
{
	int i;
	int status;
	struct termios attr;
	tcgetattr(fd, &attr);

	for (i=0; i<array_size(baudrate_arr); i++) {
		if  (rate == baudrate_name_arr[i]) {
			cfsetispeed(&attr, baudrate_arr[i]);
			cfsetospeed(&attr, baudrate_arr[i]);
			status = tcsetattr(fd, TCSANOW, &attr);
			if (status != 0) {
				cmder_err("set baudrate error:%d", status);
				return err_param;
			}
			return et_ok;
		}
	}

	return et_ok;
}

err_type uart_set_parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;
	
	if (tcgetattr(fd, &options) != 0) {
		cmder_err("get attr error");
		return err_param;
	}

	options.c_cflag |= CLOCAL | CREAD;
	options.c_cflag &= ~CSIZE;

	switch (databits) {

	case 7:
		options.c_cflag |= CS7;
		break;

	case 8:
		options.c_cflag |= CS8;
		break;

	default:
		cmder_err("unknown bits");
		return err_param;
	}

	switch (parity) {

	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */ 
		//options.c_iflag &= ~INPCK;     /* Enable parity checking */
		break;

	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);
		options.c_iflag |= INPCK;             /* Disnable parity checking */
		break;

	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;

	case 's':
	case 'S':		/*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;

	default:
		cmder_err("unknown parity");
		return err_param;
	}

	switch (stopbits) {

	case 1:
		options.c_cflag &= ~CSTOPB;
		break;

	case 2:
		options.c_cflag |= CSTOPB;
		break;

	default:
		cmder_err("unknown stopbits");
		return err_param;
	}

    //options.c_iflag &=~(IXON | IXOFF | IXANY);
	//options.c_oflag &=~(ONLCR | OCRNL | ONOCR | ONLRET);
	//options.c_oflag &= ~OPOST;
    //options.c_iflag &=~(INLCR | IGNCR | ICRNL);
    //options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	
	tcflush(fd,TCIFLUSH);

	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 0; /* Update the options and do it NOW */

	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		cmder_err("set parity error");
		return err_fail;
	}

	return et_ok;
}

int uart_claim(const char *port, int rate, uint8_t databit, uint8_t stopbit, uint8_t parity)
{
	int fd;
	err_type ret;

	fd = open(port, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0) {
		cmder_err("open error");
		return err_open;
	}

	ret = uart_set_baudrate(fd, rate);
	if (ret != et_ok) {
		return ret;
	}

	ret = uart_set_parity(fd, databit, stopbit, parity);
	if (ret != et_ok) {
		return ret;
	}

	return fd;
}
