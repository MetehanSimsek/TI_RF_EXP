#ifndef RF_TX_H_
#define RF_TX_H_

/* Standard C Libraries */
#include <stdlib.h>
/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/PIN.h>
/* Driverlib Header files */
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)
/* Board Header files */
#include "Board.h"
/* Application Header files */
#include "smartrf_settings/smartrf_settings.h"

/* Packet TX/RX Configuration */
#define PAYLOAD_LENGTH      (10)
/* Set Receive timeout to 500ms */
#define RX_TIMEOUT          (uint32_t)(4000000*0.5f)


/** @brief rf_transmit_init function set RF as a TX
 *  @return void
**/
void rf_transmit_init(void);


/** @brief send_rf function sends a specific RF signal
 *  @return void
**/
void send_rf(void);

#endif
