
#ifndef __UART_H__
#define __UART_H__


err_type uart_set_baudrate(int fd, int rate);
err_type uart_set_parity(int fd, int databits, int stopbits, int parity);
int uart_claim(const char *port, int rate, uint8_t databit, uint8_t stopbit, uint8_t parity);


#endif /* __UART_H__ */

