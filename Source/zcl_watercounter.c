/**************************************************************************************************
  Filename:       zcl_watercounter.c
  Revised:        $Date: 2015-08-19 17:11:00 -0700 (Wed, 19 Aug 2015) $
  Revision:       $Revision: 44460 $

  Description:    Zigbee Cluster Library - sample switch application.


  Copyright 2006-2013 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
  This application implements a ZigBee Water counter, based on Z-Stack 3.0.

  This application is based on the common sample-application user interface. Please see the main
  comment in zcl_sampleapp_ui.c. The rest of this comment describes only the content specific for
  this sample applicetion.
  
  Application-specific UI peripherals being used:

  - none (LED1 is currently unused by this application).

  Application-specific menu system:

    <TOGGLE LIGHT> Send an On, Off or Toggle command targeting appropriate devices from the binding table.
      Pressing / releasing [OK] will have the following functionality, depending on the value of the 
      zclWaterCounter_OnOffSwitchActions attribute:
      - OnOffSwitchActions == 0: pressing [OK] will send ON command, releasing it will send OFF command;
      - OnOffSwitchActions == 1: pressing [OK] will send OFF command, releasing it will send ON command;
      - OnOffSwitchActions == 2: pressing [OK] will send TOGGLE command, releasing it will not send any command.

*********************************************************************/

//#if ! defined ZCL_ON_OFF
//#error ZCL_ON_OFF must be defined for this project.
//#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "MT_SYS.h"
#if defined MT_DEBUG_FUNC
#include "MT_DEBUG.h"
#endif

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_watercounter.h"
#include "zcl_diagnostic.h"

#include "onboard.h"

/* HAL */
//#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_adc.h"

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
#include "zcl_ota.h"
#include "hal_ota.h"
#endif

#include "bdb.h"
#include "bdb_interface.h"

//#include "zcl_sampleapps_ui.h"

/*********************************************************************
 * MACROS
 */
// #define UI_STATE_TOGGLE_LIGHT 1 //UI_STATE_BACK_FROM_APP_MENU is item #0, so app item numbers should start from 1

#define APP_TITLE "TI Water counter"

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclWC_TaskID;

uint8 zclWC_SeqNum;

uint8 zclWC_HourCounter;        // Hour counter for Time synchronization every 24 hours 

zclReadCmd_t readCmdTimeCluster = {4, {ATTRID_TIME_TIME, ATTRID_TIME_TIME_STATUS, ATTRID_TIME_TIME_ZONE, ATTRID_TIME_LOCAL_TIME}};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
afAddrType_t zclWC_DstAddr;
uint8 zclWC_LongPushCounter;

CONST zclReportCmd_t zclWC_ReportCmd =
{
  1,
  {
    ATTRID_METER_4HISTORY_INSTANTDEMAND,
    ZCL_DATATYPE_INT24,
    (void*)&zclWC_Flow1InstDemandPrev
  },
};

CONST zclReportCmd_t zclWC_ReportCmd2 =
{
  1,
  {
    ATTRID_METER_4HISTORY_INSTANTDEMAND,
    ZCL_DATATYPE_INT24,
    (void*)&zclWC_Flow2InstDemandPrev
  },
};

CONST zclReportCmd_t zclWC_ReportCmdEveryHour =
{
  3,
  {
    {
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48,
      (void*)&zclWC_Flow1Value
    },
    {
      ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24,
      (void*)&zclWC_Flow1CurrDay
    },
    {
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24,
      (void*)&zclWC_Flow1PrevDay
    },
  },
};

// Endpoint to allow SYS_APP_MSGs
static endPointDesc_t waterCounter_TestEp =
{
  WC_ENDPOINT,                  // endpoint
  0,
  &zclWC_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

static endPointDesc_t waterCounter_TestEp2 =
{
  WC_ENDPOINT2,                  // endpoint
  0,
  &zclWC_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

//static uint8 aProcessCmd[] = { 1, 0, 0, 0 }; // used for reset command, { length + cmd0 + cmd1 + data }

devStates_t zclWC_NwkState = DEV_INIT;

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
#define DEVICE_POLL_RATE                 8000   // Poll rate for end device
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclWC_HandleKeys(byte shift, byte keys);
static void zclWC_BasicResetCB(void);

static void zclWC_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclWC_ProcessIncomingMsg(zclIncomingMsg_t *msg);
#ifdef ZCL_READ
static uint8 zclWC_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg);
#endif
#ifdef ZCL_WRITE
static uint8 zclWC_ProcessInWriteRspCmd(zclIncomingMsg_t *pInMsg);
#endif
static uint8 zclWC_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg);
#ifdef ZCL_DISCOVER
static uint8 zclWC_ProcessInDiscCmdsRspCmd(zclIncomingMsg_t *pInMsg);
static uint8 zclWC_ProcessInDiscAttrsRspCmd(zclIncomingMsg_t *pInMsg);
static uint8 zclWC_ProcessInDiscAttrsExtRspCmd(zclIncomingMsg_t *pInMsg);
#endif

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
static void zclWC_ProcessOTAMsgs(zclOTA_CallbackMsg_t* pMsg);
#endif

static void zclWC_BatteryWarningCB(uint8 voltLevel);
static void zclWC_UpdateBatteryAttributes(void);

/*********************************************************************
 * CONSTANTS
 */
  
/*********************************************************************
 * REFERENCED EXTERNALS
 */
extern int16 zdpExternalStateTaskID;

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclWC_CmdCallbacks =
{
  zclWC_BasicResetCB,               // Basic Cluster Reset command
  NULL,                                   // Identify Trigger Effect command
  NULL,                                   // On/Off cluster commands
  NULL,                                   // On/Off cluster enhanced command Off with Effect
  NULL,                                   // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                   // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
  NULL,                                   // Level Control Move to Level command
  NULL,                                   // Level Control Move command
  NULL,                                   // Level Control Step command
  NULL,                                   // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
  NULL,                                   // Group Response commands
#endif
#ifdef ZCL_SCENES
  NULL,                                   // Scene Store Request command
  NULL,                                   // Scene Recall Request command
  NULL,                                   // Scene Response command
#endif
#ifdef ZCL_ALARMS
  NULL,                                   // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
  NULL,                                   // Get Event Log command
  NULL,                                   // Publish Event Log command
#endif
  NULL,                                   // RSSI Location command
  NULL                                    // RSSI Location Response command
};

/*********************************************************************
 * @fn          zclWC_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       task_id
 *
 * @return      none
 */
void zclWC_Init(byte task_id)
{
  zclWC_TaskID = task_id;
  zclWC_SeqNum = 0;
  uint8 status;

  // Set destination address to indirect
  zclWC_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zclWC_DstAddr.endPoint = 0;
  zclWC_DstAddr.addr.shortAddr = 0;

  // Register the Simple Descriptor for this application
  bdb_RegisterSimpleDescriptor(&zclWC_SimpleDesc);
  bdb_RegisterSimpleDescriptor(&zclWC_SimpleDesc2);

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks(WC_ENDPOINT, &zclWC_CmdCallbacks);

  zclWC_NVInitItems();
  zclWC_ResetAttributesToDefaultValues();
  zclWC_UpdateBatteryAttributes();
  
  // Register the application's attribute list
  zcl_registerAttrList(WC_ENDPOINT, zclWC_NumAttributes, zclWC_Attrs);
  zcl_registerAttrList(WC_ENDPOINT2, zclWC_NumAttributes2, zclWC_Attrs2);

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg(zclWC_TaskID);
  
  // Register low voltage NV memory protection application callback
  RegisterVoltageWarningCB(zclWC_BatteryWarningCB);

  // Register for all key events - This app will handle all key events
  RegisterForKeys(zclWC_TaskID);
  
  bdb_RegisterCommissioningStatusCB(zclWC_ProcessCommissioningStatus);

  // Register for a test endpoint
  afRegister(&waterCounter_TestEp);
  afRegister(&waterCounter_TestEp2);
/*  
#ifdef BDB_REPORTING
#if (BDBREPORTING_MAX_ANALOG_ATTR_SIZE < 4)
#error BDBREPORTING_MAX_ANALOG_ATTR_SIZE less then sizeof uint32 datatype
#endif
  uint32 Flow = WC_REPORT_CHANGE_FLOW;
  uint8 reportChange[BDBREPORTING_MAX_ANALOG_ATTR_SIZE];
  
  osal_memset(reportChange, 0, BDBREPORTING_MAX_ANALOG_ATTR_SIZE);
  reportChange[0] = WC_REPORT_CHANGE_VOLTAGE;
  status = bdb_RepAddAttrCfgRecordDefaultToList(WC_ENDPOINT, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE, \
                                       zclWC_FlowReportInterval*60, zclWC_FlowReportInterval*60*6, reportChange);
  
  osal_memset(reportChange, 0, BDBREPORTING_MAX_ANALOG_ATTR_SIZE);
  osal_memcpy(reportChange, (void*)&Flow, sizeof(uint32));
  status = bdb_RepAddAttrCfgRecordDefaultToList(WC_ENDPOINT, ZCL_CLUSTER_ID_SE_METERING, ATTRID_METER_0READINGSET_CURRSUMDELIVERED, \
                                       zclWC_FlowReportInterval*60, zclWC_FlowReportInterval*60*6, reportChange);
#endif
*/  
#ifdef ZCL_DIAGNOSTIC
  // Register the application's callback function to read/write attribute data.
  // This is only required when the attribute data format is unknown to ZCL.
  zcl_registerReadWriteCB(WC_ENDPOINT, zclDiagnostic_ReadWriteAttrCB, NULL);

  if (zclDiagnostic_InitStats() == ZSuccess)
  {
    // Here the user could start the timer to save Diagnostics to NV
  }
#endif

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
  // Register for callback events from the ZCL OTA
  zclOTA_Register(zclWC_TaskID);
#endif

  zdpExternalStateTaskID = zclWC_TaskID;

  // Configure pins P1_0, P1_0 as inputs to count flows
  P1SEL &= ~BV(0);         // Set Pin as GPIO
  P1SEL &= ~BV(1);
  P1DIR &= ~BV(0);         // Select direction (input)
  P1DIR &= ~BV(1);
  P1INP &= ~BV(0);         // Set Input mode to Pullup or pulldown
  P1INP &= ~BV(1);
  P2INP &= ~BV(6);         // Set Input mode to Pullup for Port1 
  
  // Configure pin interrupts
  PICTL |= BV(1);          // Set Falling edge gives interrupt for Port1 pins 0-3
  P1IEN |= BV(0);          // Eneble interrupt for pin P1_0  
  P1IEN |= BV(1);          // Eneble interrupt for pin P1_1   
  P1IFG &= ~BV(0);         // Clear interrupt status flag
  P1IFG &= ~BV(1);
  IEN2 |= BV(4);           // Enable interrupt Port1
  
  zclWC_HourCounter = 24;   // Initialize Hour counter for time synchronization every 24 hours
  status = osal_start_timerEx(zclWC_TaskID, WC_EVT_EVERYHOUR, 10000);
  if (zclWC_FlowUpdatePeriod != 0xFF ) status = osal_start_timerEx(zclWC_TaskID, WC_EVT_UPDATEINSTDEMAND, zclWC_FlowUpdatePeriod*1000L);
  
  //bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);
}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
uint16 zclWC_event_loop(uint8 task_id, uint16 events)
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter
  uint8 status;

  if (events & SYS_EVENT_MSG)
  {
    while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclWC_TaskID)))
    {
      switch (MSGpkt->hdr.event)
      {
        case ZCL_INCOMING_MSG: // Incoming ZCL Foundation command/response messages
          zclWC_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
          break;

        case KEY_CHANGE:
          zclWC_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
          break;

        case ZDO_STATE_CHANGE:
          // UI_DeviceStateUpdated((devStates_t)(MSGpkt->hdr.status));
          break;

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
        case ZCL_OTA_CALLBACK_IND:
          zclWC_ProcessOTAMsgs((zclOTA_CallbackMsg_t*)MSGpkt);
          break;
#endif
          
        default:
          break;
      }
      osal_msg_deallocate((uint8 *)MSGpkt); // Release the memory
    }
    return (events ^ SYS_EVENT_MSG); // return unprocessed events
  }
  // ------------------------------------------------------------------
#if ZG_BUILD_ENDDEVICE_TYPE    
  if (events & WC_EVT_END_DEVICE_REJOIN)
  {
    bdb_ZedAttemptRecoverNwk();
    return (events ^ WC_EVT_END_DEVICE_REJOIN);
  }
#endif
  // ------------------------------------------------------------------
  if(events & WC_EVT_EVERYHOUR) // Every hour event. To check month change
  {
    UTCTimeStruct time;
    osal_ConvertUTCTime(&time, osal_getClock());
    
    // Battery voltage check
    zclWC_UpdateBatteryAttributes();
    // Every 24 hours attribute update
    if (time.hour == 0 && time.minutes == 0)
    {
      zclWC_Flow1PrevDay = zclWC_Flow1CurrDay;
      zclWC_Flow2PrevDay = zclWC_Flow2CurrDay;
      zclWC_Flow1CurrDay = 0;
      zclWC_Flow2CurrDay = 0;
      // Send report
    }
    // Every hour attribute update
    if ((time.minutes == 0) && (bdbAttributes.bdbNodeIsOnANetwork))
    {
        zclWC_DstAddr.addrMode = afAddr16Bit;
        zclWC_DstAddr.addr.shortAddr = 0;
        zclWC_DstAddr.endPoint = 1;
        zcl_SendReportCmd(WC_ENDPOINT, &zclWC_DstAddr, ZCL_CLUSTER_ID_SE_METERING, (zclReportCmd_t*)&zclWC_ReportCmdEveryHour, ZCL_FRAME_CLIENT_SERVER_DIR, false, zclWC_SeqNum++);  
    }
    // Try time sync with coodinator every 24 hours or more
    if ((zclWC_HourCounter > 23) && (bdbAttributes.bdbNodeIsOnANetwork))
    {
      zclDiscoverAttrsCmd_t discoverAttr;
      
      zclWC_DstAddr.addrMode = afAddrBroadcast;
      discoverAttr.startAttr = ATTRID_TIME_TIME;
      discoverAttr.maxAttrIDs = ATTRID_TIME_VALID_UNTIL_TIME;
      status = zcl_SendRead(WC_ENDPOINT, &zclWC_DstAddr, ZCL_CLUSTER_ID_GEN_TIME, &readCmdTimeCluster, ZCL_FRAME_CLIENT_SERVER_DIR, true, zclWC_SeqNum++);
      if (status) // if SendRead failure, try to discover device to sync the time
        status = zcl_SendDiscoverAttrsCmd(WC_ENDPOINT, &zclWC_DstAddr, ZCL_CLUSTER_ID_GEN_TIME, &discoverAttr, ZCL_FRAME_CLIENT_SERVER_DIR, true, zclWC_SeqNum++);
    }

    zclWC_HourCounter++;
    if (zclWC_HourCounter == 0) zclWC_HourCounter = 24; // if HourCounter was 255, time sync was not completed more than 255 hours
    
    status = osal_start_timerEx(zclWC_TaskID, WC_EVT_EVERYHOUR, (3600L - time.minutes*60 - time.seconds)*1000L);
    
    return (events ^ WC_EVT_EVERYHOUR); // return unprocessed events
  }
  // ------------------------------------------------------------------
  if(events & WC_EVT_UPDATEINSTDEMAND) // InstantDemand update period
  {
    if (zclWC_FlowUpdatePeriod < WC_METER_INSTDEMAND_UPDATEPERIOD) zclWC_FlowUpdatePeriod = WC_METER_INSTDEMAND_UPDATEPERIOD; // set minimum update intervel
    
    if ((zclWC_FlowUpdatePeriod != 0xFF) && (bdbAttributes.bdbNodeIsOnANetwork)) // if 0xFF update is disabled
    {
      if ((zclWC_Flow1InstDemandPrev != 0) || (zclWC_Flow1InstDemand != 0))
      {
        zclWC_DstAddr.addrMode = afAddr16Bit;
        zclWC_DstAddr.addr.shortAddr = 0;
        zclWC_DstAddr.endPoint = 1;
        zcl_SendReportCmd(WC_ENDPOINT, &zclWC_DstAddr, ZCL_CLUSTER_ID_SE_METERING, (zclReportCmd_t*)&zclWC_ReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, zclWC_SeqNum++);
      }
      if ((zclWC_Flow2InstDemandPrev != 0) || (zclWC_Flow2InstDemand != 0))
      {
        zclWC_DstAddr.addrMode = afAddr16Bit;
        zclWC_DstAddr.addr.shortAddr = 0;
        zclWC_DstAddr.endPoint = 1;
        zcl_SendReportCmd(WC_ENDPOINT2, &zclWC_DstAddr, ZCL_CLUSTER_ID_SE_METERING, (zclReportCmd_t*)&zclWC_ReportCmd2, ZCL_FRAME_CLIENT_SERVER_DIR, false, zclWC_SeqNum++);
      }
    }
    
    zclWC_Flow1InstDemandPrev = zclWC_Flow1InstDemand;
    zclWC_Flow2InstDemandPrev = zclWC_Flow2InstDemand;
    zclWC_Flow1InstDemand = 0;
    zclWC_Flow2InstDemand = 0;
    status = osal_start_timerEx(zclWC_TaskID, WC_EVT_UPDATEINSTDEMAND, zclWC_FlowUpdatePeriod*1000L);
    
    return (events ^ WC_EVT_UPDATEINSTDEMAND); // return unprocessed events
  }
  // ------------------------------------------------------------------ 
  if (events & WC_EVT_IMPULSE1) // Event from ISR function to recive impulse from counter
  {
    if (POLARITY_IMPULSE(P1_0))
    {
      zclWC_Flow1Value.dw.lowDW ++;
      zclWC_Flow1InstDemand ++;
      HalLedBlink(HAL_LED_4, 1, 50, 100);
    }
    return (events ^ WC_EVT_IMPULSE1);
  }
  // ------------------------------------------------------------------
  if (events & WC_EVT_IMPULSE2)
  {
    if (POLARITY_IMPULSE(P1_1))
    {
      zclWC_Flow2Value.dw.lowDW ++;
      zclWC_Flow2InstDemand ++;
      HalLedBlink(HAL_LED_5, 1, 50, 100);
    }    
    return (events ^ WC_EVT_IMPULSE2);
  }
  // ------------------------------------------------------------------  
  if (events & WC_EVT_LONGPUSH)
  {
    if (HAL_PUSH_BUTTON()) // Button is still pressed between 100ms interval
    {
      zclWC_LongPushCounter++;
      if (zclWC_LongPushCounter > 50) // Key is pressed more than 5 sec, preform LocalReset and recommission
      {
        HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_4, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_5, HAL_LED_MODE_OFF);
        //bdb_resetLocalAction();
        //ZDApp_LeaveReset();
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_INITIATOR_TL | BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);
        //SystemReset();
      }
      else
      {
        if (zclWC_LongPushCounter > 5)
          HalLedSet(HAL_LED_3, HAL_LED_MODE_TOGGLE);
        
        status = osal_start_timerEx(zclWC_TaskID, WC_EVT_LONGPUSH, WC_LONGPUSH_INTERVAL);
      }
    }
    return (events ^ WC_EVT_LONGPUSH);
  }
  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zclWC_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_3
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *                 HAL_KEY_SW_0
 *
 * @return  none
 */
static void zclWC_HandleKeys(byte shift, byte keys)
{
  HalLedBlink(HAL_LED_3, 1, 50, WC_LONGPUSH_INTERVAL);
  if (HAL_PUSH_BUTTON()) // Button is still pressed, try to determine long press 
  {
    osal_start_timerEx(zclWC_TaskID, WC_EVT_LONGPUSH, WC_LONGPUSH_INTERVAL);
    zclWC_LongPushCounter = 0;
  }
}

/*********************************************************************
 * @fn      zclWC_ProcessCommissioningStatus
 *
 * @brief   Callback in which the status of the commissioning process are reported
 *
 * @param   bdbCommissioningModeMsg - Context message of the status of a commissioning process
 *
 * @return  none
 */
static void zclWC_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
  switch(bdbCommissioningModeMsg->bdbCommissioningMode)
  {
    case BDB_COMMISSIONING_FORMATION:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //After formation, perform nwk steering again plus the remaining commissioning modes that has not been processed yet
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
      }
      else
      {
        //Want to try other channels?
        //try with bdb_setChannelAttribute
      }
    break;
    case BDB_COMMISSIONING_NWK_STEERING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
        //We are on the nwk, what now?
        HalLedSet(HAL_LED_4, HAL_LED_MODE_ON);
      }
      else
      {
        //See the possible errors for nwk steering procedure
        //No suitable networks found
        //Want to try other channels?
        //try with bdb_setChannelAttribute
      }
    break;
    case BDB_COMMISSIONING_FINDING_BINDING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
        HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_4, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_5, HAL_LED_MODE_OFF);
      }
      else
      {
        //YOUR JOB:
        //retry?, wait for user interaction?
      }
    break;
    case BDB_COMMISSIONING_INITIALIZATION:
      //Initialization notification can only be successful. Failure on initialization 
      //only happens for ZED and is notified as BDB_COMMISSIONING_PARENT_LOST notification      
      //YOUR JOB:
      //We are on a network, what now?     
    break;
#if ZG_BUILD_ENDDEVICE_TYPE    
    case BDB_COMMISSIONING_PARENT_LOST:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED)
      {
        //We did recover from losing parent
        HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_4, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_5, HAL_LED_MODE_OFF);
      }
      else
      {
        //Parent not found, attempt to rejoin again after a fixed delay
        osal_start_timerEx(zclWC_TaskID, WC_EVT_END_DEVICE_REJOIN, WC_END_DEVICE_REJOIN_DELAY);
        HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_4, HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_5, HAL_LED_MODE_ON);

      }
    break;
#endif 
  }
  
  // UI_UpdateComissioningStatus(bdbCommissioningModeMsg);
}

/*********************************************************************
 * @fn      zclWC_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to  default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zclWC_BasicResetCB(void)
{
  zclWC_ResetAttributesToDefaultValues();
}

/*********************************************************************
 * @fn      zclWC_BatteryWarningCB
 *
 * @brief   Called to handle battery-low situation.
 *
 * @param   voltLevel - level of severity
 *
 * @return  none
 */
void zclWC_BatteryWarningCB(uint8 voltLevel)
{
  if (voltLevel != VOLT_LEVEL_GOOD)
  {
    zclWC_BatteryAlarmState |= (zclWC_BatteryAlarmMask & BAT_ALARM_MASK_VOLT_2_LOW) & BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1 |
                               (zclWC_BatteryAlarmMask & BAT_ALARM_MASK_BATTERY_ALARM_1) & BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
    zclWC_Flow1Status |= METER_2STATUS_LOWBATT;
    zclWC_Flow2Status |= METER_2STATUS_LOWBATT;
  }
  if (voltLevel == VOLT_LEVEL_CAUTIOUS)
  {
    // Send warning message to the gateway and blink LED
  }
  else if (voltLevel == VOLT_LEVEL_BAD)
  {
    // Shut down the system
  }
}

/*********************************************************************
 * @fn      zclWC_UpdateBatteryAttributes
 *
 * @brief   Called to update Battery voltage and level attributes of power cluster
 *
 * @param   volt, level - voltage and level attributes
 *
 * @return  none
 */
static void zclWC_UpdateBatteryAttributes(void)
{
  zclWC_BatteryVoltage = (uint8)VDD3TOVOLTAGE(HalAdcCheckVddRaw());

  if (zclWC_BatteryVoltage < zclWC_BatteryVoltageThresMin)
    zclWC_BatteryLevel = 0;
  else
    zclWC_BatteryLevel = (zclWC_BatteryVoltage - zclWC_BatteryVoltageThresMin)*200/(zclWC_BatteryVoltageRated - zclWC_BatteryVoltageThresMin);
  
  if (zclWC_BatteryVoltage <= zclWC_BatteryVoltageThres1)
  {
    zclWC_Flow1Status |= METER_2STATUS_LOWBATT;
    zclWC_Flow2Status |= METER_2STATUS_LOWBATT;
    
    if (zclWC_BatteryAlarmMask & BAT_ALARM_MASK_BATTERY_ALARM_1)
      zclWC_BatteryAlarmState |= BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
    else
      zclWC_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
  }
  else
  {
    zclWC_Flow1Status &= ~METER_2STATUS_LOWBATT;
    zclWC_Flow2Status &= ~METER_2STATUS_LOWBATT;
    zclWC_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
  }
  
  if (zclWC_BatteryVoltage <= zclWC_BatteryVoltageThresMin)
  {   
    if (zclWC_BatteryAlarmMask & BAT_ALARM_MASK_VOLT_2_LOW)
      zclWC_BatteryAlarmState |= BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1;
    else
      zclWC_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1;
  }
  else
  {
    zclWC_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1;
  } 
}

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zclWC_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zclWC_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg)
{
  switch (pInMsg->zclHdr.commandID)
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zclWC_ProcessInReadRspCmd(pInMsg);
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zclWC_ProcessInWriteRspCmd(pInMsg);
      break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
    case ZCL_CMD_CONFIG_REPORT:
      //zclWC_ProcessInConfigReportCmd(pInMsg);
      break;

    case ZCL_CMD_CONFIG_REPORT_RSP:
      //zclWC_ProcessInConfigReportRspCmd(pInMsg);
      break;

    case ZCL_CMD_READ_REPORT_CFG:
      //zclWC_ProcessInReadReportCfgCmd(pInMsg);
      break;

    case ZCL_CMD_READ_REPORT_CFG_RSP:
      //zclWC_ProcessInReadReportCfgRspCmd(pInMsg);
      break;

    case ZCL_CMD_REPORT:
      //zclWC_ProcessInReportCmd(pInMsg);
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      zclWC_ProcessInDefaultRspCmd(pInMsg);
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
      zclWC_ProcessInDiscCmdsRspCmd(pInMsg);
      break;

    case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
      zclWC_ProcessInDiscCmdsRspCmd(pInMsg);
      break;

    case ZCL_CMD_DISCOVER_ATTRS_RSP:
      zclWC_ProcessInDiscAttrsRspCmd(pInMsg);
      
      break;

    case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
      zclWC_ProcessInDiscAttrsExtRspCmd(pInMsg);
      break;
#endif
    default:
      break;
  }

  if (pInMsg->attrCmd)
    osal_mem_free(pInMsg->attrCmd);
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zclWC_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclWC_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;
  uint8 status = 0;
  uint32 time = 0;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  
  // Processing TIME read response command for time sync
  if (pInMsg->clusterId == ZCL_CLUSTER_ID_GEN_TIME)
  {
    for (i = 0; i < readRspCmd->numAttr; i++)
    {
      if (readRspCmd->attrList[i].status == ZCL_STATUS_SUCCESS)
      {
        if (readRspCmd->attrList[i].attrID == ATTRID_TIME_TIME_STATUS) status = *readRspCmd->attrList[i].data;
        if (readRspCmd->attrList[i].attrID == ATTRID_TIME_LOCAL_TIME)
        {
          time = osal_getClock();
          if ((status & TIME_STATUS_MASTER) && (status & TIME_STATUS_SYNCH) &&
              (*((uint32*)readRspCmd->attrList[i].data) > 0L) &&
              (ABS((int32)(*((uint32*)readRspCmd->attrList[i].data) - time)) > TIME_SYNC_DIFF))
          {
            osal_setClock(*((uint32*)readRspCmd->attrList[i].data));
            zclWC_HourCounter = 0;

#if defined MT_DEBUG_FUNC
            char str[] = "Time sync completed";
            mtDebugStr_t debugstr;
            debugstr.strLen = sizeof(str);
            debugstr.pString = (uint8*)str;            
            MT_ProcessDebugStr(&debugstr);
#endif
          }
        }
      }
    }
  }

  return TRUE;
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zclWC_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclWC_ProcessInWriteRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return TRUE;
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zclWC_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclWC_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg)
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
  // Device is notified of the Default Response command.
  (void)pInMsg;
  return TRUE;
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zclWC_ProcessInDiscCmdsRspCmd
 *
 * @brief   Process the Discover Commands Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclWC_ProcessInDiscCmdsRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
  for (i = 0; i < discoverRspCmd->numCmd; i++)
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return TRUE;
}

/*********************************************************************
 * @fn      zclWC_ProcessInDiscAttrsRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclWC_ProcessInDiscAttrsRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < discoverRspCmd->numAttr; i++)
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return TRUE;
}

/*********************************************************************
 * @fn      zclWC_ProcessInDiscAttrsExtRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Extended Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclWC_ProcessInDiscAttrsExtRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclDiscoverAttrsExtRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
  for (i = 0; i < discoverRspCmd->numAttr; i++)
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return TRUE;
}
#endif // ZCL_DISCOVER

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
/*********************************************************************
 * @fn      zclWC_ProcessOTAMsgs
 *
 * @brief   Called to process callbacks from the ZCL OTA.
 *
 * @param   none
 *
 * @return  none
 */
static void zclWC_ProcessOTAMsgs(zclOTA_CallbackMsg_t* pMsg)
{
  uint8 RxOnIdle;

  switch(pMsg->ota_event)
  {
  case ZCL_OTA_START_CALLBACK:
    if (pMsg->hdr.status == ZSuccess)
    {
      // Speed up the poll rate
      RxOnIdle = TRUE;
      ZMacSetReq(ZMacRxOnIdle, &RxOnIdle);
      NLME_SetPollRate(2000);
    }
    break;

  case ZCL_OTA_DL_COMPLETE_CALLBACK:
    if (pMsg->hdr.status == ZSuccess)
    {
      // Reset the CRC Shadow and reboot.  The bootloader will see the
      // CRC shadow has been cleared and switch to the new image
      HalOTAInvRC();
      SystemReset();
    }
    else
    {
#if (ZG_BUILD_ENDDEVICE_TYPE)    
      // slow the poll rate back down.
      RxOnIdle = FALSE;
      ZMacSetReq(ZMacRxOnIdle, &RxOnIdle);
      NLME_SetPollRate(DEVICE_POLL_RATE);
#endif
    }
    break;

  default:
    break;
  }
}
#endif // defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)

/*********************************************************************
 * @fn      halPort1Isr
 *
 * @brief   Port1 Isr. Get impulses from Counters connected to the Port1
 *          Isr for keys connected to the Port0 is placed in hal_key.c
 *
 * @param   none
 *
 * @return  none
 */
HAL_ISR_FUNCTION(halPort1Isr, P1INT_VECTOR)
{
  HAL_ENTER_ISR();
  
  if (P1IFG & BV(0))
  {
    osal_start_timerEx(zclWC_TaskID, WC_EVT_IMPULSE1, WC_DEBOUNCE);
  }
  if (P1IFG & BV(1))
  {
    osal_start_timerEx(zclWC_TaskID, WC_EVT_IMPULSE2, WC_DEBOUNCE);
  }
  P1IFG = 0;
  
  HAL_EXIT_ISR();
}
/****************************************************************************
****************************************************************************/

