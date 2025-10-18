/*
 * UART.h
 */

#ifndef UART_H_
#define UART_H_

/*	USARTDIV = F_PCLK / (16 X DESIRED BAUD RATE)
 * 	USARTDIV = 64 MHz / (16 X 115200 bps)
 * 	USARTDIV = 34.72222222
 * 	DIV_Mantissa = 0d34 = 0x22
 * 	DIV_Fraction = 16 X 0d0.340277778 = 0d11.55555556 = 0xC
 * 	USARTDIV = 0x22B
*/

/* 	USARTDIV = F_PCLK / (16 X DESIRED BAUD RATE)
* 	USARTDIV = 8 MHz / (16 X 115200 bps)
* 	USARTDIV = 4.340277778
* 	DIV_Mantissa = 0d4 = 0x4
* 	DIV_Fraction = 16 X 0d0.340277778 = 0d5.444444444 = 0x5
* 	USARTDIV = 0x45
*/

// USART selection

#define	USART_1		 0

// USART peripheral configuration bits and baud rate register value

#define USART_CR1_UE	0x1UL << 13U
#define USART_CR1_M	0x1UL << 12U
#define USART_CR2_STOP	0x3UL << 12U
#define USARTDIV_64MHZ	0x22B
#define USARTDIV_8MHZ	0x45

// Transmitter bits

#define USART_CR1_TE	0x1UL <<  3U
#define USART_SR_TXE	0x1UL <<  7U

// Receiver bits

#define USART_CR1_RE	0x1UL << 2U
#define USART_SR_RXNE	0x1UL << 5U

// Function prototypes for USART

void USER_USART_Init( uint8_t );
int _write( int, char*, int );

#endif /* UART_H_ */
