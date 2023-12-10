#ifndef RF_UART_H_
#define RF_UART_H_


#include <stdlib.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/PIN.h>
#include "Board.h"

#define UART_RECEIVE_SUCCES         RF_UART_DATA_LENGTH
#define UART_TRANSMIT_SUCCES        RF_UART_DATA_LENGTH
#define RF_UART_DATA_LENGTH         (10)
#define PIN_HIGH                    (1)
#define PIN_LOW                     (0)

void rf_uart_init(void);
void rf_uart_receive(uint8_t rf_receive_data[10]);
void rf_uart_transmit(uint8_t rf_transfer_data[10]);


#endif
