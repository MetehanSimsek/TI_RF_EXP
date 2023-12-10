#include "rf_tx.h"

/***** Variable declarations *****/
static RF_Object rfObject;
static RF_Handle rfHandle;
static RF_Params rfParams;

/* Pin driver handle */
PIN_Handle ledPinHandle;
PIN_State ledPinState;

/* Receive Statistics */
static rfc_propRxOutput_t rxStatistics;
/* Receive dataQueue for RF Core to fill in data */
static dataQueue_t dataQueue;

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

uint8_t txPacket[PAYLOAD_LENGTH] = {1,2,3,4,5,6,7,8,9,0};

/***** Function definitions *****/
void rf_transmit_init(void)
{
    RF_Params_init(&rfParams);
    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, pinTable);
    if (ledPinHandle == NULL)
    {
        while(1);
    }
    /* Modify CMD_PROP_TX and CMD_PROP_RX commands for application needs */
    RF_cmdPropTx.pktLen = PAYLOAD_LENGTH;
    RF_cmdPropTx.pPkt = txPacket;
    RF_cmdPropTx.startTrigger.triggerType = TRIG_ABSTIME;
    RF_cmdPropTx.startTrigger.pastTrig = 1;
    RF_cmdPropTx.startTime = 0;
    RF_cmdPropTx.pNextOp = (rfc_radioOp_t *)&RF_cmdPropRx;
    /* Only run the RX command if TX is successful */
    RF_cmdPropTx.condition.rule = COND_STOP_ON_FALSE;
    /* Set the Data Entity queue for received data */
    RF_cmdPropRx.pQueue = &dataQueue;
    /* Discard ignored packets from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushIgnored = 1;
    /* Discard packets with CRC error from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushCrcErr = 1;
    /* Implement packet length filtering to avoid PROP_ERROR_RXBUF */
    RF_cmdPropRx.maxPktLen = PAYLOAD_LENGTH;
    RF_cmdPropRx.pktConf.bRepeatOk = 0;
    RF_cmdPropRx.pktConf.bRepeatNok = 0;
    RF_cmdPropRx.pOutput = (uint8_t *)&rxStatistics;
    /* Receive operation will end RX_TIMEOUT ms after command starts */
    RF_cmdPropRx.endTrigger.triggerType = TRIG_REL_PREVEND;
    RF_cmdPropRx.endTime = RX_TIMEOUT;
    /* Request access to the radio */
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);
    /* Set the frequency */
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);
    PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 0);
}

void send_rf(void)
{
    RF_EventMask Result =
                    RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTx, RF_PriorityNormal,
                              NULL, (RF_EventCmdDone | RF_EventRxEntryDone |
                              RF_EventLastCmdDone));
    switch(Result)
        {
            case RF_EventLastCmdDone:
                PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 1);
            // A stand-alone radio operation command or the last radio
            // operation command in a chain finished.
            break;
            case RF_EventCmdCancelled:
                PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 0);
            break;
            case RF_EventCmdAborted:
                PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 0);
            break;
            case RF_EventCmdStopped:
                PIN_setOutputValue(ledPinHandle, Board_PIN_LED2, 0);
            break;
            default:
            // Uncaught error event
            while(1);
        }
    uint32_t cmdStatus = ((volatile RF_Op*)&RF_cmdPropTx)->status;
    switch(cmdStatus)
        {
            case PROP_DONE_OK:
            // Packet transmitted successfully
            break;
            case PROP_DONE_STOPPED:
            // received CMD_STOP while transmitting packet and finished
            // transmitting packet
            break;
            case PROP_DONE_ABORT:
            // Received CMD_ABORT while transmitting packet
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
            case PROP_ERROR_TXUNF:
            // TX underflow observed during operation
            break;
            default:
            // Uncaught error event - these could come from the
            // pool of states defined in rf_mailbox.h
            while(1);
        }
}


