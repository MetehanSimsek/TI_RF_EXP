#include <stdint.h>
#include <stddef.h>

#include <NoRTOS.h>
/* Example/Board Header files */
#include "Board.h"

#include "rf_uart.h"
#include "rf_tx.h"


extern uint8_t txPacket[];

int main(void)
{
    /* Call driver init functions */
    Board_initGeneral();
    /* Start NoRTOS */
    NoRTOS_start();

    rf_transmit_init();
    rf_uart_init();

    while (1)
    {
        send_rf();
        rf_uart_receive(txPacket);
    }
}
