#ifndef _ZYNQ_UART_H_
#define _ZYNQ_UART_H_

enum {
    UART_DATA_8_BIT = 1,
    UART_DATA_7_BIT,
    UART_DATA_6_BIT,
};

enum {
    UART_EVEN_PARITY,
    UART_ODD_PARITY,
    UART_SPACE_PARITY,
    UART_MARK_PARITY,
    UART_NO_PARITY,
};

enum {
    UART_1_STOP_BIT,
    UART_1_5_STOP_BIT,
    UART_2_STOP_BIT,
};

typedef struct UartDataFormat
{
	int32_t iDataBits;	/**< Number of data bits */
	int32_t iParity;	/**< Parity */
	uint8_t iStopBits;	/**< Number of stop bits */

}UartDataFormat_t;

/* All driver's ioctl magic number */
/* uart */
#define UART_MAGIC_IOC	'u'
#define UART_IOC_NREAD		    	_IOR(UART_MAGIC_IOC, 0, int)
#define UART_IOC_NWRITE			    _IOW(UART_MAGIC_IOC, 1, int)
#define UART_IOC_SET_BAUDRATE	    _IOW(UART_MAGIC_IOC, 2, int)
#define UART_IOC_SET_MODE		    _IOW(UART_MAGIC_IOC, 3, int)
#define UART_IOC_SET_DATA_FORMAT	_IOW(UART_MAGIC_IOC, 4, UartDataFormat_t)


#endif
