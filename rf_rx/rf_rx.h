#ifndef RF_RX_H_
#define RF_RX_H_

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
#include "RFQueue.h"
#include "smartrf_settings/smartrf_settings.h"

/* Max length byte the radio will accept */
#define PAYLOAD_LENGTH         (10)
/* Set Transmit (echo) delay to 100ms */
#define NUM_DATA_ENTRIES       (2)
/* The Data Entries data field will contain:
 * 1 Header byte (RF_cmdPropRx.rxConf.bIncludeHdr = 0x1)
 * Max 30 payload bytes
 * 1 status byte (RF_cmdPropRx.rxConf.bAppendStatus = 0x1) */
#define NUM_APPENDED_BYTES     (2)

/** @brief rf_transmit_init function set RF as a RX
 *  @return void
**/
void rf_receive_init(void);


/** @brief send_rf function receive a specific RF signal
 *  @return void
**/
void rf_receive_data(void);

#endif
