#include "rf_uart.h"


extern PIN_Handle ledPinHandle;
static UART_Handle uart_handle;

void rf_uart_init(void)
{
    UART_init();
    UART_Params uart_init;
    UART_Params_init(&uart_init);
    uart_init.writeDataMode = UART_DATA_BINARY;
    uart_init.readDataMode = UART_DATA_BINARY;
    uart_init.readReturnMode = UART_RETURN_FULL;
    uart_init.readEcho = UART_ECHO_OFF;
    uart_init.baudRate = 115200;
    uart_handle = UART_open(Board_UART0, &uart_init);
    if(!uart_handle)
        {
        // when uart failed
        while(1);
        }
}

void rf_uart_receive(uint8_t rf_receive_data[10])
{
       if(UART_read(uart_handle,rf_receive_data,RF_UART_DATA_LENGTH) == UART_RECEIVE_SUCCES)
        {
           PIN_setOutputValue(ledPinHandle, Board_PIN_LED1, PIN_HIGH);
        }
}

void rf_uart_transmit(uint8_t rf_transfer_data[10])
{
    if(UART_write(uart_handle, rf_transfer_data, RF_UART_DATA_LENGTH) == UART_TRANSMIT_SUCCES)
        {
            PIN_setOutputValue(ledPinHandle, Board_PIN_LED1, PIN_HIGH);
        }
}
