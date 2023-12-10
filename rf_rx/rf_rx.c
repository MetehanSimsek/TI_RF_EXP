#include "rf_rx.h"


/***** Variable declarations *****/
static RF_Object rfObject;
static RF_Handle rfHandle;

/* Pin driver handle */
PIN_Handle ledPinHandle;
PIN_State ledPinState;

/* Buffer which contains all Data Entries for receiving data.
 * Pragmas are needed to make sure this buffer is aligned to a 4 byte boundary
 * (requirement from the RF core)
 */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(rxDataEntryBuffer, 4)
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  PAYLOAD_LENGTH,
                                                  NUM_APPENDED_BYTES)];
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment = 4
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  PAYLOAD_LENGTH,
                                                  NUM_APPENDED_BYTES)];
#elif defined(__GNUC__)
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  PAYLOAD_LENGTH,
                                                  NUM_APPENDED_BYTES)]
                                                  __attribute__((aligned(4)));
#else
#error This compiler is not supported
#endif //defined(__TI_COMPILER_VERSION__)

/* Receive Statistics */
static rfc_propRxOutput_t rxStatistics;
static void echoCallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);
/* Receive dataQueue for RF Core to fill in data */
static dataQueue_t dataQueue;
static rfc_dataEntryGeneral_t* currentDataEntry;
static uint8_t packetLength;
static uint8_t* packetDataPointer;

uint8_t txPacket[PAYLOAD_LENGTH];

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config pinTable[] =
{
#if defined(Board_CC1350_LAUNCHXL)
 Board_DIO30_SWPWR | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
#endif
 Board_PIN_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
 Board_PIN_LED2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
 PIN_TERMINATE
};

/***** Function definitions *****/

void rf_receive_init(void)
{
    RF_Params rfParams;
    RF_Params_init(&rfParams);

    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, pinTable);
    if (ledPinHandle == NULL)
    {
        while(1);
    }

    if( RFQueue_defineQueue(&dataQueue,
                            rxDataEntryBuffer,
                            sizeof(rxDataEntryBuffer),
                            NUM_DATA_ENTRIES,
                            PAYLOAD_LENGTH + NUM_APPENDED_BYTES))

    /* Modify CMD_PROP_TX and CMD_PROP_RX commands for application needs */
    /* Set the Data Entity queue for received data */
    RF_cmdPropRx.pQueue = &dataQueue;
    /* Discard ignored packets from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushIgnored = 1;
    /* Discard packets with CRC error from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushCrcErr = 1;
    /* Implement packet length filtering to avoid PROP_ERROR_RXBUF */
    RF_cmdPropRx.maxPktLen = PAYLOAD_LENGTH;
    /* End RX operation when a packet is received correctly and move on to the
     * next command in the chain */
    RF_cmdPropRx.pktConf.bRepeatOk = 0;
    RF_cmdPropRx.pktConf.bRepeatNok = 1;
    RF_cmdPropRx.startTrigger.triggerType = TRIG_NOW;
    RF_cmdPropRx.pNextOp = (rfc_radioOp_t *)&RF_cmdPropTx;
     /* Only run the TX command if RX is successful */
    RF_cmdPropRx.condition.rule = COND_STOP_ON_FALSE;
    RF_cmdPropRx.pOutput = (uint8_t *)&rxStatistics;

    /* Request access to the radio */
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);
    /* Set the frequency */
    RF_CmdHandle x = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropRx, RF_PriorityNormal, NULL, RF_EventRxEntryDone);
}

void rf_receive_data(void)
{

    /* Wait for a packet
     * - When the first of the two chained commands (RX) completes, the
     * RF_EventCmdDone and RF_EventRxEntryDone events are raised on a
     * successful packet reception, and then the next command in the chain
     * (TX) is run
     * - If the RF core runs into an issue after receiving the packet
     * incorrectly onlt the RF_EventCmdDone event is raised; this is an
     * error condition
     * - If the RF core successfully echos the received packet the RF core
     * should raise the RF_EventLastCmdDone event
     */
    RF_EventMask terminationReason = RF_runCmd(rfHandle, (RF_Op*) &RF_cmdPropRx,
                                               RF_PriorityNormal, echoCallback,
                                               (RF_EventRxEntryDone |
                                               RF_EventLastCmdDone));

    switch (terminationReason)
    {
    case RF_EventLastCmdDone:
        // A stand-alone radio operation command or the last radio
        // operation command in a chain finished.
        break;
    case RF_EventCmdCancelled:
        // Command cancelled before it was started; it can be caused
        // by RF_cancelCmd() or RF_flushCmd().
        break;
    case RF_EventCmdAborted:
        // Abrupt command termination caused by RF_cancelCmd() or
        // RF_flushCmd().
        break;
    case RF_EventCmdStopped:
        // Graceful command termination caused by RF_cancelCmd() or
        // RF_flushCmd().
        break;
    default:
        // Uncaught error event
        while (1);
    }

    uint32_t cmdStatus = ((volatile RF_Op*) &RF_cmdPropRx)->status;
    switch (cmdStatus)
    {
    case PROP_DONE_OK:
        // Packet received with CRC OK
        break;
    case PROP_DONE_RXERR:
        // Packet received with CRC error
        break;
    case PROP_DONE_RXTIMEOUT:
        // Observed end trigger while in sync search
        break;
    case PROP_DONE_BREAK:
        // Observed end trigger while receiving packet when the command is
        // configured with endType set to 1
        break;
    case PROP_DONE_ENDED:
        // Received packet after having observed the end trigger; if the
        // command is configured with endType set to 0, the end trigger
        // will not terminate an ongoing reception
        break;
    case PROP_DONE_STOPPED:
        // received CMD_STOP after command started and, if sync found,
        // packet is received
        break;
    case PROP_DONE_ABORT:
        // Received CMD_ABORT after command started
        break;
    case PROP_ERROR_RXBUF:
        // No RX buffer large enough for the received data available at
        // the start of a packet
        break;
    case PROP_ERROR_RXFULL:
        // Out of RX buffer space during reception in a partial read
        break;
    case PROP_ERROR_PAR:
        // Observed illegal parameter
        break;
    case PROP_ERROR_NO_SETUP:
        // Command sent without setting up the radio in a supported
        // mode using CMD_PROP_RADIO_SETUP or CMD_RADIO_SETUP
        break;
    case PROP_ERROR_NO_FS:
        // Command sent without the synthesizer being programmed
        break;
    case PROP_ERROR_RXOVF:
        // RX overflow observed during operation
        break;
    default:
        // Uncaught error event - these could come from the
        // pool of states defined in rf_mailbox.h
        while (1)
            ;
    }
}
static void echoCallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
#ifdef LOG_RADIO_EVENTS
    eventLog[evIndex++ & 0x1F] = e;
#endif// LOG_RADIO_EVENTS

    if (e & RF_EventRxEntryDone)
    {
        /* Successful RX */
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 1);

        /* Get current unhandled data entry */
        currentDataEntry = RFQueue_getDataEntry();

        /* Handle the packet data, located at &currentDataEntry->data:
         * - Length is the first byte with the current configuration
         * - Data starts from the second byte */
        packetLength      = *(uint8_t *)(&(currentDataEntry->data));
        packetDataPointer = (uint8_t *)(&(currentDataEntry->data) + 1);

        /* Copy the payload + status byte to the rxPacket variable, and then
         * over to the txPacket
         */
        memcpy(txPacket, packetDataPointer, packetLength);

        RFQueue_nextEntry();
    }
    else if (e & RF_EventLastCmdDone)
    {
        /* Toggle LED2, clear LED1 to indicate RX */
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED2,1);
    }
    else // any uncaught event
    {
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED1, 0);
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 0);
    }
}
