/**************************************************************************************************
  Filename:       BlueBasic.c
  Revised:        $Date: 2010-08-06 08:56:11 -0700 (Fri, 06 Aug 2010) $
  Revision:       $Revision: 23333 $

  Description:    Basic interpreter for the CC254x BLE device.

  Copyright 2014 Tim Wilkinson. All rights reserved.

  Based on sample code with the following copyright notice:

  Copyright 2010 - 2013 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "hal_uart.h"

#include "OnBoard.h"

#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "linkdb.h"

#include "observer.h"
#include "peripheral.h"

#include "gapbondmgr.h"

#include "BlueBasic.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

#include "os.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// What is the advertising interval when device is discoverable (units of 625us, 160=100ms, 800=500ms, 1600=1s)
#define DEFAULT_ADVERTISING_INTERVAL          1600

#define INVALID_CONNHANDLE                    0xFFFF

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 blueBasic_TaskID;   // Task ID for internal task/event processing

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern void ble_connection_status(uint16 connHandle, uint8 changeType, int8 rssi);


/*********************************************************************
 * LOCAL VARIABLES
 */

#ifdef ENABLE_BLE_CONSOLE

#include "gatt_uuid.h"
#include "gap.h"

// Nordic UART Service (NUS)
// Service: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
// TX Char (Notify): 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
// RX Char (Write / Write Without Response): 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
#define CONSOLE_PROFILE_SERV_UUID 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E
static CONST uint8 consoleProfileServUUID[] = { CONSOLE_PROFILE_SERV_UUID };
static CONST gattAttrType_t consoleProfileService = { ATT_UUID_SIZE, consoleProfileServUUID };
static CONST uint8 inputUUID[] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E };
static CONST uint8 inputProps = GATT_PROP_READ|GATT_PROP_NOTIFY;
static CONST uint8 outputUUID[] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E };
static CONST uint8 outputProps = GATT_PROP_WRITE|GATT_PROP_WRITE_NO_RSP;
static gattCharCfg_t consoleProfileCharCfg[GATT_MAX_NUM_CONN];

unsigned char ble_console_enabled;

static struct
{
  uint8 write[128];
  uint8* writein;
  uint8* writeout;
} io;

static gattAttribute_t consoleProfile[] =
{
  { { ATT_BT_UUID_SIZE, primaryServiceUUID },   GATT_PERMIT_READ,                       0, (uint8*)&consoleProfileService },
  { { ATT_BT_UUID_SIZE, characterUUID },        GATT_PERMIT_READ,                       0, (uint8*)&inputProps },
  { { ATT_UUID_SIZE, inputUUID },               GATT_PERMIT_READ|GATT_PERMIT_WRITE,     0, io.write },
  { { ATT_BT_UUID_SIZE, clientCharCfgUUID },    GATT_PERMIT_READ|GATT_PERMIT_WRITE,     0, (uint8*)consoleProfileCharCfg },
  { { ATT_BT_UUID_SIZE, characterUUID },        GATT_PERMIT_READ,                       0, (uint8*)&outputProps },
  { { ATT_UUID_SIZE, outputUUID },              GATT_PERMIT_READ|GATT_PERMIT_WRITE,     0, NULL },
};

static bStatus_t consoleProfile_ReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen);
static bStatus_t consoleProfile_WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 len, uint16 offset);

static CONST gattServiceCBs_t consoleProfileCB =
{
  consoleProfile_ReadAttrCB,
  consoleProfile_WriteAttrCB,
  NULL
};

static CONST uint8 consoleAdvert[] =
{
  0x02, GAP_ADTYPE_FLAGS, GAP_ADTYPE_FLAGS_LIMITED|GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
  0x11, GAP_ADTYPE_128BIT_MORE, CONSOLE_PROFILE_SERV_UUID,
  0x09, GAP_ADTYPE_LOCAL_NAME_COMPLETE, 'B', 'A', 'S', 'I', 'C', '#', '?', '?'
};

#endif

#ifdef ENABLE_FAKE_OAD_PROFILE

static CONST uint8 oadProfileServiceUUID[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x40, 0x51, 0x04, 0xC0, 0xFF, 0x00, 0xF0 };
static CONST uint8 oadIdentUUID[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x40, 0x51, 0x04, 0xC1, 0xFF, 0x00, 0xF0 };
static CONST uint8 oadBlockUUID[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x40, 0x51, 0x04, 0xC2, 0xFF, 0x00, 0xF0 };
static CONST gattAttrType_t oadProfileService = { ATT_UUID_SIZE, oadProfileServiceUUID };
static CONST uint8 oadCharProps = GATT_PROP_WRITE_NO_RSP | GATT_PROP_WRITE | GATT_PROP_NOTIFY;
static CONST gattCharCfg_t oadConfig[GATT_MAX_NUM_CONN];
static CONST unsigned char oadDesc[] = "";

static gattAttribute_t oadProfile[] =
{
  { { ATT_BT_UUID_SIZE, primaryServiceUUID },   GATT_PERMIT_READ,                       0, (uint8*)&oadProfileService },
  { { ATT_BT_UUID_SIZE, characterUUID },        GATT_PERMIT_READ,                       0, (uint8*)&oadCharProps },
  { { ATT_UUID_SIZE, oadIdentUUID },            GATT_PERMIT_WRITE,                      0, NULL },
  { { ATT_BT_UUID_SIZE, clientCharCfgUUID },    GATT_PERMIT_READ|GATT_PERMIT_WRITE,     0, (uint8*)oadConfig },
  { { ATT_BT_UUID_SIZE, charUserDescUUID },     GATT_PERMIT_READ,                       0, NULL },
  { { ATT_BT_UUID_SIZE, characterUUID },        GATT_PERMIT_READ,                       0, (uint8*)&oadCharProps },
  { { ATT_UUID_SIZE, oadBlockUUID },            GATT_PERMIT_WRITE,                      0, (uint8*)oadDesc },
  { { ATT_BT_UUID_SIZE, clientCharCfgUUID },    GATT_PERMIT_READ|GATT_PERMIT_WRITE,     0, (uint8*)oadConfig },
  { { ATT_BT_UUID_SIZE, charUserDescUUID },     GATT_PERMIT_READ,                       0, (uint8*)oadDesc }
};

#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void blueBasic_deviceFound( gapDeviceInfoEvent_t* device );

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Link callback
static void blueBasic_HandleConnStatusCB(uint16 connHandle, uint8 changeType);
static void blueBasic_RSSIUpdate(int8 rssi);

// GAP Role Callbacks
static gapRolesCBs_t blueBasic_PeripheralCBs =
{
  NULL,                           // Profile State Change Callbacks
  blueBasic_RSSIUpdate,           // When a valid RSSI is read from controller
  blueBasic_deviceFound           // Device found
};

#ifdef GAP_BOND_MGR
// GAP Bond Manager Callbacks
static gapBondCBs_t blueBasic_BondMgrCBs =
{
  NULL,                     // Passcode callback (not used by application)
  NULL                      // Pairing / Bonding state Callback (not used by application)
};
#endif

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      BlueBasic_Init
 *
 * @brief   Initialization function for the Blue Basic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void BlueBasic_Init( uint8 task_id )
{
  blueBasic_TaskID = task_id;

  // Ensure 32-kHz RC oscillator continuous calibration is enabled in hardware (bit 7 of SLEEPCMD = 0)
  SLEEPCMD &= ~0x80;

  // Configure P1_7 (UART1 RX) falling-edge detection for wake-up from PM2
  PICTL |= BV(2); // Falling edge on P1.7..4
  IEN2 |= BV(4);  // Enable Port 1 interrupt

#if defined ( POWER_SAVING )
  // Hold active power state by default on startup
  osal_pwrmgr_task_state(blueBasic_TaskID, PWRMGR_HOLD);
#endif

#ifdef ENABLE_BLE_CONSOLE
  GAPRole_SetParameter( GAPROLE_ADVERT_DATA, 0, sizeof(consoleAdvert), (void*)consoleAdvert );
#endif

  // Set advertising interval
  GAP_SetParamValue( TGAP_LIM_DISC_ADV_INT_MIN, DEFAULT_ADVERTISING_INTERVAL );
  GAP_SetParamValue( TGAP_LIM_DISC_ADV_INT_MAX, DEFAULT_ADVERTISING_INTERVAL );
  GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, DEFAULT_ADVERTISING_INTERVAL );
  GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, DEFAULT_ADVERTISING_INTERVAL );

  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );            // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
#ifdef ENABLE_FAKE_OAD_PROFILE
  GATTServApp_RegisterService(oadProfile, GATT_NUM_ATTRS(oadProfile), NULL);
#endif
  DevInfo_AddService();                           // Device Information Service
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif

  // Enable clock divide on halt
  // This reduces active current while radio is active and CC254x MCU
  // is halted
#ifdef ENABLE_BLE_CONSOLE
  // Keep MCU clock undivided so UART baud rate generator never shifts or loses synchronization
  HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_DISABLE_CLK_DIVIDE_ON_HALT);
#endif

  // Overlap enabled
  HCI_EXT_OverlappedProcessingCmd(HCI_EXT_ENABLE_OVERLAPPED_PROCESSING);

  // Setup a delayed profile startup
  osal_set_event( blueBasic_TaskID, BLUEBASIC_START_DEVICE_EVT );
}

/*********************************************************************
 * @fn      BlueBasic_ProcessEvent
 *
 * @brief   Blue Basic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 BlueBasic_ProcessEvent( uint8 task_id, uint16 events )
{
  unsigned char i;

  VOID task_id; // OSAL required parameter that isn't used in this function

  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( blueBasic_TaskID )) != NULL )
    {
      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & BLUEBASIC_START_DEVICE_EVT )
  {
    // Start the Device
    VOID GAPRole_StartDevice( &blueBasic_PeripheralCBs );

#ifdef GAP_BOND_MGR
    // Start Bond Manager
    VOID GAPBondMgr_Register( &blueBasic_BondMgrCBs );
#endif
    // Start monitoring links
    VOID linkDB_Register( blueBasic_HandleConnStatusCB );

#ifdef ENABLE_BLE_CONSOLE
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, consoleProfileCharCfg);
    GATTServApp_RegisterService(consoleProfile, GATT_NUM_ATTRS(consoleProfile), &consoleProfileCB);
#endif

    // Start Interpreter
    interpreter_setup();

    return ( events ^ BLUEBASIC_START_DEVICE_EVT );
  }

#ifdef ENABLE_BLE_CONSOLE
  if ( events & BLUEBASIC_CONNECTION_EVENT )
  {
    while (io.writein != io.writeout)
    {
      uint8* save = io.writeout;
      if (GATTServApp_ProcessCharCfg(consoleProfileCharCfg, io.write, FALSE, consoleProfile, GATT_NUM_ATTRS(consoleProfile), INVALID_TASK_ID) != SUCCESS)
      {
        io.writeout = save;
        break;
      }
    }
    return ( events ^ BLUEBASIC_CONNECTION_EVENT );
  }
#endif

  if ( events & BLUEBASIC_INPUT_AVAILABLE )
  {
    interpreter_loop();
    return (events ^ BLUEBASIC_INPUT_AVAILABLE);
  }

  if ( events & BLUEBASIC_EVENT_INTERRUPTS )
  {
    for (i = 0; i < OS_MAX_INTERRUPT; i++)
    {
      if (blueBasic_interrupts[i].linenum && (events & (BLUEBASIC_EVENT_INTERRUPT << i)))
      {
        if (blueBasic_interrupts[i].debounce_ms)
        {
          unsigned char pin = blueBasic_interrupts[i].pin;
          unsigned char major = PIN_MAJOR(pin);
          unsigned char minor = PIN_MINOR(pin);
          unsigned char pin_val = 0;
          unsigned char expected = blueBasic_interrupts[i].falling ? 0 : 1;

          if (major == 0)
          {
            pin_val = (P0 >> minor) & 1;
            P0IFG &= ~(1 << minor);
            P0IF = 0;
            P0IEN |= (1 << minor);
          }
          else if (major == 1)
          {
            pin_val = (P1 >> minor) & 1;
            P1IFG &= ~(1 << minor);
            P1IF = 0;
            P1IEN |= (1 << minor);
          }
          else if (major == 2)
          {
            pin_val = (P2 >> minor) & 1;
            P2IFG &= ~(1 << minor);
            P2IF = 0;
            P2IEN |= (1 << minor);
          }

          blueBasic_interrupts[i].debouncing = 0;

          if (pin_val == expected)
          {
            interpreter_run(blueBasic_interrupts[i].linenum, 1);
          }
        }
        else
        {
          interpreter_run(blueBasic_interrupts[i].linenum, 1);
        }
      }
    }
    return (events ^ (events & BLUEBASIC_EVENT_INTERRUPTS));
  }

  if ( events & BLUEBASIC_EVENT_TIMERS )
  {
    for (i = 0; i < OS_MAX_TIMER; i++)
    {
      if (blueBasic_timers[i].linenum && (events & (BLUEBASIC_EVENT_TIMER << i)))
      {
        interpreter_run(blueBasic_timers[i].linenum, i == DELAY_TIMER ? 0 : 1);
      }
    }
    return (events ^ (events & BLUEBASIC_EVENT_TIMERS));
  }
  
  if ( events & BLUEBASIC_EVENT_SERIAL )
  {
#ifdef OS_UART_PORT
    if (serial[0].onread && Hal_UART_RxBufLen(OS_UART_PORT) > 0)
    {
      interpreter_run(serial[0].onread, 1);
    }
    if (serial[0].onwrite && Hal_UART_TxBufLen(OS_UART_PORT) > 0)
    {
      interpreter_run(serial[0].onwrite, 1);
    }
#endif

    return (events ^ BLUEBASIC_EVENT_SERIAL);
  }

  if ( events & BLUEBASIC_SLEEP_EVT )
  {
#ifdef OS_UART_PORT
    while (Hal_UART_TxBufLen(OS_UART_PORT) > 0 || (U1CSR & 0x01))
      ;
#endif
#if defined ( POWER_SAVING )
    U1CSR &= ~0x40;
    P1SEL &= ~BV(7);
    P1DIR &= ~BV(7);
    P1IFG &= ~BV(7);
    P1IF = 0;
    P1IEN |= BV(7);
    osal_pwrmgr_task_state(blueBasic_TaskID, PWRMGR_CONSERVE);
#endif
    return (events ^ BLUEBASIC_SLEEP_EVT);
  }

  if ( events & BLUEBASIC_WAKE_CLEAR_EVT )
  {
    serial_waking = 0;
    OS_prompt_reset();
    return (events ^ BLUEBASIC_WAKE_CLEAR_EVT);
  }

  // Discard unknown events
  return 0;
}

static void blueBasic_deviceFound( gapDeviceInfoEvent_t* device )
{
  interpreter_devicefound(device->addrType, device->addr, device->rssi, device->eventType, device->dataLen, device->pEvtData);
}

static void blueBasic_HandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
  if (connHandle == LOOPBACK_CONNHANDLE)
  {
    return;
  }
#ifdef ENABLE_BLE_CONSOLE
  if (changeType == LINKDB_STATUS_UPDATE_REMOVED || (changeType == LINKDB_STATUS_UPDATE_STATEFLAGS && !linkDB_Up(connHandle)))
  {
    GATTServApp_InitCharCfg(connHandle, consoleProfileCharCfg);
    uint8 i;
    for (i = 0; i < GATT_MAX_NUM_CONN; i++)
    {
      if (consoleProfileCharCfg[i].value == 1)
      {
        goto done;
      }
    }
    ble_console_enabled = 0;
  done:;
  }
#endif
  ble_connection_status(connHandle, changeType, 0);
}

static void blueBasic_RSSIUpdate(int8 rssi)
{
  ble_connection_status(0, LINKDB_STATUS_UPDATE_RSSI, rssi);
}

#ifdef ENABLE_BLE_CONSOLE

uint8 ble_console_write(uint8 ch)
{
  if (ble_console_enabled)
  {
    while (io.writein - io.writeout + 1 == 0 || io.writein - io.writeout + 1 == sizeof(io.write))
    {
      osal_run_system();
    }
    
    if (io.writein == io.writeout)
    {
      osal_set_event( blueBasic_TaskID, BLUEBASIC_CONNECTION_EVENT );
    }
 
    *io.writein++ = ch;
    if (io.writein == &io.write[sizeof(io.write)])
    {
      io.writein = io.write;
    }
  }
  return 1;
}

static bStatus_t consoleProfile_ReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen)
{
  uint8 len;
  for (len = 0; io.writein != io.writeout && len < maxLen; len++)
  {
    *pValue++ = *io.writeout++;
    if (io.writeout == &io.write[sizeof(io.write)])
    {
      io.writeout = io.write;
    }
  }
  *pLen = len;

  return SUCCESS;
}

static bStatus_t consoleProfile_WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 len, uint16 offset)
{
  unsigned char i;
  bStatus_t status;

  if (pAttr->type.len == ATT_BT_UUID_SIZE && BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]) == GATT_CLIENT_CHAR_CFG_UUID)
  {
    status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len, offset, GATT_CLIENT_CFG_NOTIFY);
    // Setup console if we're connected, otherwise disable
    for (i = 0; i < GATT_MAX_NUM_CONN; i++)
    {
      if (consoleProfileCharCfg[i].value == 1)
      {
        io.writein = io.write;
        io.writeout = io.write;
        ble_console_enabled = 1;
        OS_timer_stop(DELAY_TIMER);
        interpreter_banner();
        goto done;
      }
    }
    ble_console_enabled = 0;
  done:
    return status;
  }
  for (i = 0; i < len; i++)
  {
    OS_type(pValue[i]);
  }
  return SUCCESS;
}

#endif

HAL_ISR_FUNCTION(port0Isr, P0INT_VECTOR)
{
  unsigned char status;
  unsigned char i;

  HAL_ENTER_ISR();

  status = P0IFG;
  status &= P0IEN;
  if (status)
  {
    P0IFG = ~status;
    P0IF = 0;
    
#if HAL_UART_DMA == 1
    extern uint8 Hal_TaskID;
    extern volatile uint8 dmaRdyIsr;
    dmaRdyIsr = 1;
    CLEAR_SLEEP_MODE();
    osal_pwrmgr_task_state(Hal_TaskID, PWRMGR_HOLD);
#if HAL_UART_TX_BY_ISR
    if (dmaCfg.txHead == dmaCfg.txTail)
    {
      HAL_UART_DMA_CLR_RDY_OUT();
    }
#endif
#endif // HAL_UART_DMA

    for (i = 0; i < OS_MAX_INTERRUPT; i++)
    {
      if (PIN_MAJOR(blueBasic_interrupts[i].pin) == 0 && (status & (1 << PIN_MINOR(blueBasic_interrupts[i].pin))))
      {
        if (blueBasic_interrupts[i].debounce_ms)
        {
          if (!blueBasic_interrupts[i].debouncing)
          {
            blueBasic_interrupts[i].debouncing = 1;
            P0IEN &= ~(1 << PIN_MINOR(blueBasic_interrupts[i].pin));
            osal_start_timerEx(blueBasic_TaskID, BLUEBASIC_EVENT_INTERRUPT << i, blueBasic_interrupts[i].debounce_ms);
          }
        }
        else
        {
          osal_set_event(blueBasic_TaskID, BLUEBASIC_EVENT_INTERRUPT << i);
        }
      }
    }
  }

  HAL_EXIT_ISR();
}

HAL_ISR_FUNCTION(port1Isr, P1INT_VECTOR)
{
  unsigned char status;
  unsigned char i;

  HAL_ENTER_ISR();

  status = P1IFG;
  status &= P1IEN;
  if (status)
  {
    P1IFG = ~status;
    P1IF = 0;

    if (status & BV(7))
    {
      P1IEN &= ~BV(7);
      P1SEL |= BV(7);
      U1CSR |= 0x40;
      CLEAR_SLEEP_MODE();
#if defined ( POWER_SAVING )
      osal_stop_timerEx(blueBasic_TaskID, BLUEBASIC_SLEEP_EVT);
      osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
      osal_pwrmgr_task_state(blueBasic_TaskID, PWRMGR_HOLD);
#endif
      serial_waking = 1;
      osal_start_timerEx(blueBasic_TaskID, BLUEBASIC_WAKE_CLEAR_EVT, 15);
    }

    for (i = 0; i < OS_MAX_INTERRUPT; i++)
    {
      if (PIN_MAJOR(blueBasic_interrupts[i].pin) == 1 && (status & (1 << PIN_MINOR(blueBasic_interrupts[i].pin))))
      {
        if (blueBasic_interrupts[i].debounce_ms)
        {
          if (!blueBasic_interrupts[i].debouncing)
          {
            blueBasic_interrupts[i].debouncing = 1;
            P1IEN &= ~(1 << PIN_MINOR(blueBasic_interrupts[i].pin));
            osal_start_timerEx(blueBasic_TaskID, BLUEBASIC_EVENT_INTERRUPT << i, blueBasic_interrupts[i].debounce_ms);
          }
        }
        else
        {
          osal_set_event(blueBasic_TaskID, BLUEBASIC_EVENT_INTERRUPT << i);
        }
      }
    }
  }

  HAL_EXIT_ISR();
}

HAL_ISR_FUNCTION(port2Isr, P2INT_VECTOR)
{
  unsigned char status;
  unsigned char i;

  HAL_ENTER_ISR();

  status = P2IFG;
  status &= P2IEN;
  if (status)
  {
    P2IFG = ~status;
    P2IF = 0;

    for (i = 0; i < OS_MAX_INTERRUPT; i++)
    {
      if (PIN_MAJOR(blueBasic_interrupts[i].pin) == 2 && (status & (1 << PIN_MINOR(blueBasic_interrupts[i].pin))))
      {
        if (blueBasic_interrupts[i].debounce_ms)
        {
          if (!blueBasic_interrupts[i].debouncing)
          {
            blueBasic_interrupts[i].debouncing = 1;
            P2IEN &= ~(1 << PIN_MINOR(blueBasic_interrupts[i].pin));
            osal_start_timerEx(blueBasic_TaskID, BLUEBASIC_EVENT_INTERRUPT << i, blueBasic_interrupts[i].debounce_ms);
          }
        }
        else
        {
          osal_set_event(blueBasic_TaskID, BLUEBASIC_EVENT_INTERRUPT << i);
        }
      }
    }
  }

  HAL_EXIT_ISR();
}

/*********************************************************************
*********************************************************************/
