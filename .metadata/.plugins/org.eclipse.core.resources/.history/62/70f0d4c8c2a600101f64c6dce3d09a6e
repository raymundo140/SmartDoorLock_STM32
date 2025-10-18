/*
 * UART.c
 */

#include "libraries.h"
#include "UART.h"
#include "main.h"
#include "GPIO.h"

// Initialize USART

void USER_USART_Init( uint8_t USART )
{
  /* Only USART1 is clocked with PCLK2 (72 MHz max). Other USARTs are clocked with
  PCLK1 (36 MHz max) */

  if( USART == 0 )
  {
    RCC->APB2ENR	|= 	RCC_APB2ENR_USART1EN; 		// Clock enable for USART1

    USER_GPIO_Define(PORTA, 9, OUT_10, OUT_AF_PP);		// Pin PA9 (USART1_TX) as alternate function output push-pull, max speed 10 MHz

    USER_GPIO_Define(PORTA, 10, INP, INP_PP);			// Pin PA10 (USART1_RX) as input pull-up
    USER_GPIO_Write(PORTA, 10, 1);

    USART1->CR1		|=	 USART_CR1_UE;			// Step 1 - USART enabled
    USART1->CR1		&=	~USART_CR1_M;			// Step 2 - 8 Data bits
    USART1->CR2		&=	~USART_CR2_STOP;		// Step 3 - 1 Stop bit
    USART1->BRR		=	 USARTDIV_64MHZ;		// Step 5 - Desired baud rate
    USART1->CR1		|= 	 USART_CR1_TE;			// Step 6 - Transmitter enabled
    USART1->CR1		|=	 USART_CR1_RE;			// Step 7 - Receiver enabled
  }
}

/* Custom implementation of _write function, called by the C
 * runtime library to perform the writing of any function that
 * writes to the standard output.
 *
 * It redirects standard output stream in C to USART peripheral
 * device */

int _write( int file, char *ptr, int len )
{
  int DataIdx;

  for( DataIdx = 0 ; DataIdx < len; DataIdx++ )
  {
    while(!( USART1->SR & USART_SR_TXE ));		// Wait until USART_DR is empty
    USART1->DR = *ptr++;				// Transmit data
  }

  return len;
}
