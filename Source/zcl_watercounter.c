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
#ifdef ZCL_DIAGNOSTIC
#include "zcl_diagnostic.h"
#endif

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
#define APP_TITLE       "TI Water counter"
   
#define HAL_LED_STATUS  HAL_LED_3
#define HAL_LED_IN1     HAL_LED_4
#define HAL_LED_IN2     HAL_LED_5   

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zapp_TaskID;

uint8 zapp_SeqNum;

uint8 zapp_TimeHasSynced;             // True if time has synced
uint16 zapp_TimeSynchElapsedMinutes;  // Minutes elapsed since Time synchronization
uint16 zapp_MinutesInOperation;       // Minutes to calculate InOperation attribute
uint16 zapp_AttrToStore;           // Bitmap of Items to write into NV memory

const zclReadCmd_t zapp_ReadCmdTime = {4, {ATTRID_TIME_TIME, ATTRID_TIME_TIME_STATUS, ATTRID_TIME_TIME_ZONE, ATTRID_TIME_LOCAL_TIME}};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
afAddrType_t zapp_DstAddr;
uint8 zapp_LongPushCounter;

const zclReportCmd_t zapp_ReportCmdBattery =
{
  3,
  {
    {
      ATTRID_POWER_CFG_BATTERY_VOLTAGE,
      ZCL_DATATYPE_UINT8,
      (void *)&zapp_BatteryVoltage
    },
    {
      ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING,
      ZCL_DATATYPE_UINT8,
      (void *)&zapp_BatteryLevel
    },
    {
      ATTRID_POWER_CFG_BAT_ALARM_STATE,
      ZCL_DATATYPE_BITMAP32,
      (void *)&zapp_BatteryAlarmState
    },
  },
};

const zclReportCmd_t zapp_ReportCmdInstDemand =
{
  1,
  {
    {
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24,
      (void*)&zapp_Flow1InstDemand
    },
  },
};

const zclReportCmd_t zapp_ReportCmdInstDemand2 =
{
  1,
  {
    {
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24,
      (void*)&zapp_Flow2InstDemand
    },
  },
};

const zclReportCmd_t zapp_ReportCmdEveryHour =
{
  3,
  {
    {
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48,
      (void*)&zapp_Flow1Value
    },
    {
      ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24,
      (void*)&zapp_Flow1CurrDay
    },
    {
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24,
      (void*)&zapp_Flow1PrevDay
    },
  },
};

const zclReportCmd_t zapp_ReportCmdEveryHour2 =
{
  3,
  {
    {
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48,
      (void*)&zapp_Flow2Value
    },
    {
      ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24,
      (void*)&zapp_Flow2CurrDay
    },
    {
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24,
      (void*)&zapp_Flow2PrevDay
    },
  },
};

// Endpoint to allow SYS_APP_MSGs
static endPointDesc_t waterCounter_TestEp =
{
  WC_ENDPOINT,                  // endpoint
  0,
  &zapp_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

static endPointDesc_t waterCounter_TestEp2 =
{
  WC_ENDPOINT2,                  // endpoint
  0,
  &zapp_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

//static uint8 aProcessCmd[] = { 1, 0, 0, 0 }; // used for reset command, { length + cmd0 + cmd1 + data }

devStates_t zapp_NwkState = DEV_INIT;

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
#define DEVICE_POLL_RATE                 8000   // Poll rate for end device
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zapp_fHandleKeys(byte shift, byte keys);
static void zapp_fBasicResetCB(void);

static void zapp_fProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);

// Functions to process ZCL Foundation incoming Command/Response messages
static void zapp_fProcessIncomingMsg(zclIncomingMsg_t *msg);
#ifdef ZCL_READ
static uint8 zapp_fProcessInReadRspCmd(zclIncomingMsg_t *pInMsg);
#endif
#ifdef ZCL_WRITE
static uint8 zapp_fProcessInWriteRspCmd(zclIncomingMsg_t *pInMsg);
static uint8 zapp_fValidateAddrDataCB(zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo);
#endif
ZStatus_t zapp_fAuthorizeCB( afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper );
static uint8 zapp_fProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg);
#ifdef ZCL_DISCOVER
static uint8 zapp_fProcessInDiscCmdsRspCmd(zclIncomingMsg_t *pInMsg);
static uint8 zapp_fProcessInDiscAttrsRspCmd(zclIncomingMsg_t *pInMsg);
static uint8 zapp_fProcessInDiscAttrsExtRspCmd(zclIncomingMsg_t *pInMsg);
#endif

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
static void zapp_fProcessOTAMsgs(zclOTA_CallbackMsg_t* pMsg);
#endif

static void zapp_fBatteryWarningCB(uint8 voltLevel);
static void zapp_fUpdateBatteryAttributes(void);

//#ifdef MT_DEBUG_FUNC
//void MT_ProcessDebugString(const char *str);
//#endif

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
static zclGeneral_AppCallbacks_t zapp_CmdCallbacks =
{
  zapp_fBasicResetCB,               // Basic Cluster Reset command
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
 * @fn          zapp_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       task_id
 *
 * @return      none
 */
void zapp_Init(byte task_id)
{
  zapp_TaskID = task_id;
  zapp_SeqNum = 0;
  uint8 status;

  // Set destination address to indirect
  zapp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zapp_DstAddr.endPoint = 0;
  zapp_DstAddr.addr.shortAddr = 0;

  // Register the Simple Descriptor for this application
  bdb_RegisterSimpleDescriptor(&zapp_SimpleDesc);
  bdb_RegisterSimpleDescriptor(&zapp_SimpleDesc2);

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks(WC_ENDPOINT, &zapp_CmdCallbacks);

  zapp_fNVInitItems();
  zapp_fResetAttributesToDefaultValues();
  zapp_DiagRebootReason = (SLEEPSTA & 0x18)>>3;  // Save reboot reason in Diagnostic cluster
  zapp_fUpdateBatteryAttributes();
  
  // Register the application's attribute list
  zcl_registerAttrList(WC_ENDPOINT, zapp_cNumAttributes, zapp_cAttrs);
  zcl_registerAttrList(WC_ENDPOINT2, zapp_cNumAttributes2, zapp_cAttrs2);
  
  zcl_registerValidateAttrData(zapp_fValidateAddrDataCB);
  zcl_registerReadWriteCB(WC_ENDPOINT, NULL, zapp_fAuthorizeCB);

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg(zapp_TaskID);
  
  // Register low voltage NV memory protection application callback
  RegisterVoltageWarningCB(zapp_fBatteryWarningCB);

  // Register for all key events - This app will handle all key events
  RegisterForKeys(zapp_TaskID);
  
  bdb_RegisterCommissioningStatusCB(zapp_fProcessCommissioningStatus);

  // Register for a test endpoint
  afRegister(&waterCounter_TestEp);
  afRegister(&waterCounter_TestEp2);
/*  
#ifdef BDB_REPORTING
#if (BDBREPORTING_MAX_ANALOG_ATTR_SIZE < 4)
#error BDBREPORTING_MAX_ANALOG_ATTR_SIZE less then sizeof uint32 datatype
#endif
  uint32 Flow = WC_VOLUME_PER_REPORT;
  uint8 reportChange[BDBREPORTING_MAX_ANALOG_ATTR_SIZE];
  
  osal_memset(reportChange, 0, BDBREPORTING_MAX_ANALOG_ATTR_SIZE);
  reportChange[0] = WC_REPORT_CHANGE_VOLTAGE;
  status = bdb_RepAddAttrCfgRecordDefaultToList(WC_ENDPOINT, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE, \
                                       zapp_FlowReportInterval*60, zapp_FlowReportInterval*60*6, reportChange);
  
  osal_memset(reportChange, 0, BDBREPORTING_MAX_ANALOG_ATTR_SIZE);
  osal_memcpy(reportChange, (void*)&Flow, sizeof(uint32));
  status = bdb_RepAddAttrCfgRecordDefaultToList(WC_ENDPOINT, ZCL_CLUSTER_ID_SE_METERING, ATTRID_METER_0READINGSET_CURRSUMDELIVERED, \
                                       zapp_FlowReportInterval*60, zapp_FlowReportInterval*60*6, reportChange);
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
  zclOTA_Register(zapp_TaskID);
#endif

  zdpExternalStateTaskID = zapp_TaskID;

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
  
  zapp_TimeSynchElapsedMinutes = 0;   // Initialize counter for time synchronization
  zapp_TimeHasSynced = false;
  zapp_MinutesInOperation = 0;        // Init to calculate HoursInOperation attribute
  
  zapp_AttrToStore = 0; // Bitmap of Items to write into NV memory
  
  if (zapp_FlowUpdatePeriod != 0xFF )
    status = osal_start_timerEx(zapp_TaskID, WC_EVT_UPDATEINSTDEMAND, zapp_FlowUpdatePeriod*1000L);
  
  HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_ON); // to show NoNetwork state
  HalLedSet(HAL_LED_IN1, HAL_LED_MODE_OFF);
  HalLedSet(HAL_LED_IN2, HAL_LED_MODE_OFF);
  
  status = 0;
  osal_nv_read(ZCD_NV_BDBNODEISONANETWORK, 0, sizeof(bdbAttributes.bdbNodeIsOnANetwork), &status);
  if ((status == true) && (status != 0xFF))  // Connect to network after reboot if device was commissioned OnNetwork
    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_INITIATOR_TL | BDB_COMMISSIONING_MODE_NWK_STEERING /*| BDB_COMMISSIONING_MODE_FINDING_BINDING*/);
  
  //status = osal_start_timerEx(zapp_TaskID, WC_EVT_UPDATE, 10000);
  status = osal_set_event(zapp_TaskID, WC_EVT_UPDATE);
#ifdef MT_DEBUG_FUNC
  MT_ProcessDebugString("Init completed");
#endif
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
uint16 zapp_event_loop(uint8 task_id, uint16 events)
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter
  uint8 status;

  if (events & SYS_EVENT_MSG)
  {
    while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zapp_TaskID)))
    {
      switch (MSGpkt->hdr.event)
      {
        case ZCL_INCOMING_MSG: // Incoming ZCL Foundation command/response messages
          zapp_fProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
          break;

        case KEY_CHANGE:
          zapp_fHandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
          break;

        case ZDO_STATE_CHANGE:
          // UI_DeviceStateUpdated((devStates_t)(MSGpkt->hdr.status));
#ifdef MT_DEBUG_FUNC
          MT_ProcessDebugString("ZDO_State");
#endif
          break;

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
        case ZCL_OTA_CALLBACK_IND:
          zapp_ProcessOTAMsgs((zclOTA_CallbackMsg_t*)MSGpkt);
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
#ifdef MT_DEBUG_FUNC
    MT_ProcessDebugMsg4(osal_heap_block_cnt(), osal_heap_block_free(), osal_heap_mem_used(), osal_heap_high_water());
#endif    
    bdb_ZedAttemptRecoverNwk();
#ifdef MT_DEBUG_FUNC
    MT_ProcessDebugMsg4(osal_heap_block_cnt(), osal_heap_block_free(), osal_heap_mem_used(), osal_heap_high_water());
#endif    
    return (events ^ WC_EVT_END_DEVICE_REJOIN);
  }
#endif
  // ------------------------------------------------------------------
  if (events & WC_EVT_BATTERY)
  {
    zapp_fUpdateBatteryAttributes();
    if (bdbAttributes.bdbNodeIsOnANetwork)
    {
        zapp_DstAddr.addrMode = afAddr16Bit;
        zapp_DstAddr.addr.shortAddr = 0;
        zapp_DstAddr.endPoint = 1;
        zcl_SendReportCmd(WC_ENDPOINT, &zapp_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, &zapp_ReportCmdBattery, ZCL_FRAME_SERVER_CLIENT_DIR, false, zapp_SeqNum++);
    }
    return (events ^ WC_EVT_BATTERY);
  }
  // ------------------------------------------------------------------
  if (events & WC_EVT_STOREATTR)
  {
    if (zapp_AttrToStore != 0)
    {
      zapp_fStoreAttrToNV(&zapp_AttrToStore);
    }
    return (events ^ WC_EVT_STOREATTR);
  }
  // ------------------------------------------------------------------
  if(events & WC_EVT_UPDATE) // Update event. To check month change
  {
    UTCTimeStruct time;
    osal_ConvertUTCTime(&time, osal_getClock());
#ifdef MT_DEBUG_FUNC
    MT_ProcessDebugMsg4(osal_heap_block_cnt(), osal_heap_block_free(), osal_heap_mem_used(), osal_heap_high_water());
#endif    
    // Battery voltage check
    zapp_fUpdateBatteryAttributes();
    // Calculate HoursInOperation attribute
    zapp_Flow1HoursInOperation += zapp_MinutesInOperation / 60;
    zapp_MinutesInOperation = zapp_MinutesInOperation % 60;
    zapp_Flow2HoursInOperation = zapp_Flow1HoursInOperation;
    // Every 24 hours attribute update
    if (time.hour == 0 && time.minutes <= 1)
    {
      zapp_Flow1PrevDay = zapp_Flow1CurrDay;
      zapp_Flow2PrevDay = zapp_Flow2CurrDay;
      zapp_Flow1CurrDay = 0;
      zapp_Flow2CurrDay = 0;
    }
    // Every interval attributes update
    if (bdbAttributes.bdbNodeIsOnANetwork && ((bdbAttributes.bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED) ||
                                              (bdbAttributes.bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)))
    {
      zapp_DstAddr.addrMode = afAddr16Bit;
      zapp_DstAddr.addr.shortAddr = 0;
      zapp_DstAddr.endPoint = 1;
      zcl_SendReportCmd(WC_ENDPOINT, &zapp_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, &zapp_ReportCmdBattery, ZCL_FRAME_SERVER_CLIENT_DIR, false, zapp_SeqNum++);
      zcl_SendReportCmd(WC_ENDPOINT, &zapp_DstAddr, ZCL_CLUSTER_ID_SE_METERING, &zapp_ReportCmdEveryHour, ZCL_FRAME_SERVER_CLIENT_DIR, false, zapp_SeqNum++);
      zcl_SendReportCmd(WC_ENDPOINT2, &zapp_DstAddr, ZCL_CLUSTER_ID_SE_METERING, &zapp_ReportCmdEveryHour2, ZCL_FRAME_SERVER_CLIENT_DIR, false, zapp_SeqNum++);
      
      // Try time sync with coodinator every 24 hours or more
      if ((zapp_TimeHasSynced == false) || (zapp_TimeSynchElapsedMinutes >= 24*60))
      {     
        zapp_DstAddr.addrMode = afAddr16Bit;
        zapp_DstAddr.addr.shortAddr = 0;
        zapp_DstAddr.endPoint = 1;
        
        status = zcl_SendRead(WC_ENDPOINT, &zapp_DstAddr, ZCL_CLUSTER_ID_GEN_TIME, &zapp_ReadCmdTime, ZCL_FRAME_CLIENT_SERVER_DIR, false, zapp_SeqNum++);
        if (status != ZSuccess) // if SendRead failure, try to discover device to sync the time
        {
          zclDiscoverAttrsCmd_t discoverAttr;
          zapp_DstAddr.addrMode = afAddrBroadcast;
          discoverAttr.startAttr = ATTRID_TIME_TIME;
          discoverAttr.maxAttrIDs = ATTRID_TIME_VALID_UNTIL_TIME;
          status = zcl_SendDiscoverAttrsCmd(WC_ENDPOINT, &zapp_DstAddr, ZCL_CLUSTER_ID_GEN_TIME, &discoverAttr, ZCL_FRAME_CLIENT_SERVER_DIR, false, zapp_SeqNum++);
        }
      }
    }

    uint16 timeRemain = (23 - time.hour)*60 + (59 - time.minutes);
    if (timeRemain <= zapp_FlowReportInterval)
    {
      status = osal_start_timerEx(zapp_TaskID, WC_EVT_UPDATE, (timeRemain*60 + (60 - time.seconds))*1000L);
      zapp_TimeSynchElapsedMinutes += timeRemain;
      zapp_MinutesInOperation += timeRemain;
    }
    else
    {
      if (zapp_FlowReportInterval < 60)
      {        
        timeRemain = (59 - time.minutes);
        if (timeRemain <= zapp_FlowReportInterval)
        {
          status = osal_start_timerEx(zapp_TaskID, WC_EVT_UPDATE, (timeRemain*60 + (60 - time.seconds))*1000L);
          zapp_TimeSynchElapsedMinutes += timeRemain;
          zapp_MinutesInOperation += timeRemain;
        }
        else
        {
          status = osal_start_timerEx(zapp_TaskID, WC_EVT_UPDATE, (zapp_FlowReportInterval*60 + (60 - time.seconds))*1000L);          
          zapp_TimeSynchElapsedMinutes += zapp_FlowReportInterval;
          zapp_MinutesInOperation += zapp_FlowReportInterval;
        }
      }
      else
      {          
        status = osal_start_timerEx(zapp_TaskID, WC_EVT_UPDATE, ((zapp_FlowReportInterval - 60)*60 + (59 - time.minutes)*60 + (60 - time.seconds))*1000L);                  
        zapp_TimeSynchElapsedMinutes += zapp_FlowReportInterval;
        zapp_MinutesInOperation += zapp_FlowReportInterval;
      }
    }
    if (zapp_TimeSynchElapsedMinutes > 32768) zapp_TimeSynchElapsedMinutes = 24*60; // if HourCounter was 255, time sync was not completed more than 255 hours
    
    //status = osal_start_timerEx(zapp_TaskID, WC_EVT_EVERYHOUR, /*(3600L - time.minutes*60 - time.seconds)*/ 120000L);
    
#warning For testing only
    osal_set_event(zapp_TaskID, WC_EVT_IMPULSE1);
    osal_set_event(zapp_TaskID, WC_EVT_IMPULSE2);
    
    return (events ^ WC_EVT_UPDATE); // return unprocessed events
  }
  // ------------------------------------------------------------------
  if(events & WC_EVT_UPDATEINSTDEMAND) // InstantDemand update period
  {
    if (zapp_FlowUpdatePeriod < WC_METER_INSTDEMAND_UPDATEPERIOD_MIN) zapp_FlowUpdatePeriod = WC_METER_INSTDEMAND_UPDATEPERIOD_MIN; // set minimum update intervel
    
    if (bdbAttributes.bdbNodeIsOnANetwork) // if 0xFF update is disabled
    {
      if ((zapp_Flow1InstDemandPrev != 0) || (zapp_Flow1InstDemand != 0))
      {
        zapp_DstAddr.addrMode = afAddr16Bit;
        zapp_DstAddr.addr.shortAddr = 0;
        zapp_DstAddr.endPoint = 1;
        zcl_SendReportCmd(WC_ENDPOINT, &zapp_DstAddr, ZCL_CLUSTER_ID_SE_METERING, &zapp_ReportCmdInstDemand, ZCL_FRAME_SERVER_CLIENT_DIR, false, zapp_SeqNum++);
      }
      if ((zapp_Flow2InstDemandPrev != 0) || (zapp_Flow2InstDemand != 0))
      {
        zapp_DstAddr.addrMode = afAddr16Bit;
        zapp_DstAddr.addr.shortAddr = 0;
        zapp_DstAddr.endPoint = 1;
        zcl_SendReportCmd(WC_ENDPOINT2, &zapp_DstAddr, ZCL_CLUSTER_ID_SE_METERING, &zapp_ReportCmdInstDemand2, ZCL_FRAME_SERVER_CLIENT_DIR, false, zapp_SeqNum++);
      }
    }
    
    zapp_Flow1InstDemandPrev = zapp_Flow1InstDemand;
    zapp_Flow2InstDemandPrev = zapp_Flow2InstDemand;
    zapp_Flow1InstDemand = 0;
    zapp_Flow2InstDemand = 0;
    
    if (zapp_FlowUpdatePeriod != WC_METER_INSTDEMAND_UPDATEPERIOD_MAX)
      status = osal_start_timerEx(zapp_TaskID, WC_EVT_UPDATEINSTDEMAND, zapp_FlowUpdatePeriod*1000L);
    
    return (events ^ WC_EVT_UPDATEINSTDEMAND); // return unprocessed events
  }
  // ------------------------------------------------------------------ 
  if (events & WC_EVT_IMPULSE1) // Event from ISR function to recive impulse from counter
  {
#warning Disabled For testing only
    //if (POLARITY_IMPULSE(P1_0))
    {
      zapp_Flow1Value.dw.lowDW++;
      zapp_Flow1CurrDay++;
      if (zapp_FlowUpdatePeriod != 0xFF) zapp_Flow1InstDemand++;
      HalLedBlink(HAL_LED_IN1, 1, 50, 100);
    }
    return (events ^ WC_EVT_IMPULSE1);
  }
  // ------------------------------------------------------------------
  if (events & WC_EVT_IMPULSE2)
  {
#warning Disabled For testing only
    //if (POLARITY_IMPULSE(P1_1))
    {
      zapp_Flow2Value.dw.lowDW++;
      zapp_Flow2CurrDay++;
      if (zapp_FlowUpdatePeriod != 0xFF) zapp_Flow2InstDemand++;
      HalLedBlink(HAL_LED_IN2, 1, 50, 100);
    }    
    return (events ^ WC_EVT_IMPULSE2);
  }
  // ------------------------------------------------------------------  
  // Button is pressed 0 sec -> 5 sec - Wakeup and Blink LED, if it is not released than LED Blink 100ms
  // Button is pressed 5 sec -> 10 sec - Start commissioning, if it is not released than LED Blink 25% faster
  // Button is pressed more 10 sec - Reset network settings
  if (events & WC_EVT_LONGPUSH)
  {
    if (HAL_PUSH_BUTTON()) // Button is still pressed between 100ms interval
    {
      zapp_LongPushCounter++;
      if (zapp_LongPushCounter > 100) // Key is pressed more than 10 sec, preform LocalReset and recommission
      {
        HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_IN1, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN2, HAL_LED_MODE_OFF);
        bdb_resetLocalAction();
        ZDApp_LeaveReset(TRUE);
        //bdb_StartCommissioning(BDB_COMMISSIONING_MODE_INITIATOR_TL | BDB_COMMISSIONING_MODE_NWK_STEERING /*| BDB_COMMISSIONING_MODE_FINDING_BINDING*/);
        //SystemReset();
      }
      else
      {
        HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_TOGGLE);
        if (zapp_LongPushCounter < 50)
          status = osal_start_timerEx(zapp_TaskID, WC_EVT_LONGPUSH, WC_TIMEOUT_LONGPUSH);
        else
          status = osal_start_timerEx(zapp_TaskID, WC_EVT_LONGPUSH, WC_TIMEOUT_LONGPUSH*5/4);        
      }
    }
    else
    { //Button is released
        if (zapp_LongPushCounter > 50)
          bdb_StartCommissioning(BDB_COMMISSIONING_MODE_INITIATOR_TL | BDB_COMMISSIONING_MODE_NWK_STEERING /*| BDB_COMMISSIONING_MODE_FINDING_BINDING*/);
    }
    return (events ^ WC_EVT_LONGPUSH);
  }
  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zapp_fHandleKeys
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
static void zapp_fHandleKeys(byte shift, byte keys)
{
  HalLedBlink(HAL_LED_STATUS, 1, 50, WC_TIMEOUT_LONGPUSH);
  if (HAL_PUSH_BUTTON()) // Button is still pressed, try to determine long press 
  {
    osal_start_timerEx(zapp_TaskID, WC_EVT_LONGPUSH, WC_TIMEOUT_LONGPUSH);
    zapp_LongPushCounter = 0;
  }
}

/*********************************************************************
 * @fn      zapp_fProcessCommissioningStatus
 *
 * @brief   Callback in which the status of the commissioning process are reported
 *
 * @param   bdbCommissioningModeMsg - Context message of the status of a commissioning process
 *
 * @return  none
 */
static void zapp_fProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
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
        //We are on the nwk, what now?
        HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN1, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN2, HAL_LED_MODE_OFF);
        
        if (zapp_TimeHasSynced == false)
        {
          uint32 timeout;
          // Timer's remain time substracts from InOperation time
          timeout = osal_get_timeoutEx(zapp_TaskID, WC_EVT_UPDATE);
          timeout = timeout / (60*1000L);
          if (zapp_MinutesInOperation < (uint24)timeout)
            zapp_MinutesInOperation = 0;
          else
            zapp_MinutesInOperation -= timeout;
          
          osal_set_event(zapp_TaskID, WC_EVT_UPDATE);
        }
      }
      else
      {
        //See the possible errors for nwk steering procedure
        //No suitable networks found. Want to try other channels? Try with bdb_setChannelAttribute
        HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_IN1, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN2, HAL_LED_MODE_OFF);
      }
    break;
    case BDB_COMMISSIONING_FINDING_BINDING:
      if(bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
      {
        //YOUR JOB:
        HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN1, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN2, HAL_LED_MODE_OFF);
      }
      else
      {
        //YOUR JOB:
        //HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN1, HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_IN2, HAL_LED_MODE_OFF);
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
        HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN1, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN2, HAL_LED_MODE_OFF);
      }
      else
      {
        //Parent not found, attempt to rejoin again after a fixed delay
        osal_start_timerEx(zapp_TaskID, WC_EVT_END_DEVICE_REJOIN, WC_TIMEOUT_END_DEVICE_REJOIN);
        HalLedSet(HAL_LED_STATUS, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN1, HAL_LED_MODE_OFF);
        HalLedSet(HAL_LED_IN2, HAL_LED_MODE_ON);

      }
    break;
#endif 
  }
  
  // UI_UpdateComissioningStatus(bdbCommissioningModeMsg);
}

/*********************************************************************
 * @fn      zapp_fBasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to  default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zapp_fBasicResetCB(void)
{
  zapp_fResetAttributesToDefaultValues();
  zapp_DiagRebootReason = (SLEEPSTA & 0x18)>>3;  // Save reboot reason in Diagnostic cluster
}

/*********************************************************************
 * @fn      zapp_fBatteryWarningCB
 *
 * @brief   Called to handle battery-low situation.
 *
 * @param   voltLevel - level of severity
 *
 * @return  none
 */
void zapp_fBatteryWarningCB(uint8 voltLevel)
{
  if (voltLevel != VOLT_LEVEL_GOOD)
  {
    zapp_BatteryAlarmState |= (zapp_BatteryAlarmMask & BAT_ALARM_MASK_VOLT_2_LOW) & BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1 |
                               (zapp_BatteryAlarmMask & BAT_ALARM_MASK_BATTERY_ALARM_1) & BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
    zapp_Flow1Status |= METER_2STATUS_LOWBATT;
    zapp_Flow2Status |= METER_2STATUS_LOWBATT;
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
 * @fn      zapp_fUpdateBatteryAttributes
 *
 * @brief   Called to update Battery voltage and level attributes of power cluster
 *
 * @param   volt, level - voltage and level attributes
 *
 * @return  none
 */
static void zapp_fUpdateBatteryAttributes(void)
{
  zapp_BatteryVoltage = (uint8)VDD3TOVOLTAGE(HalAdcCheckVddRaw());

  if (zapp_BatteryVoltage < zapp_BatteryVoltageThresMin)
    zapp_BatteryLevel = 0;
  else
    zapp_BatteryLevel = (zapp_BatteryVoltage - zapp_BatteryVoltageThresMin)*200/(zapp_BatteryVoltageRated - zapp_BatteryVoltageThresMin);
  
  if (zapp_BatteryVoltage <= zapp_BatteryVoltageThres1)
  {
    zapp_Flow1Status |= METER_2STATUS_LOWBATT;
    zapp_Flow2Status |= METER_2STATUS_LOWBATT;
    
    if (zapp_BatteryAlarmMask & BAT_ALARM_MASK_BATTERY_ALARM_1)
      zapp_BatteryAlarmState |= BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
    else
      zapp_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
  }
  else
  {
    zapp_Flow1Status &= ~METER_2STATUS_LOWBATT;
    zapp_Flow2Status &= ~METER_2STATUS_LOWBATT;
    zapp_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_THRES_1_BAT_SRC_1;
  }
  
  if (zapp_BatteryVoltage <= zapp_BatteryVoltageThresMin)
  {   
    if (zapp_BatteryAlarmMask & BAT_ALARM_MASK_VOLT_2_LOW)
      zapp_BatteryAlarmState |= BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1;
    else
      zapp_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1;
  }
  else
  {
    zapp_BatteryAlarmState &= ~BAT_ALARM_STATE_BAT_VOLT_MIN_THRES_BAT_SRC_1;
  } 
}

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zapp_fProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zapp_fProcessIncomingMsg(zclIncomingMsg_t *pInMsg)
{
  switch (pInMsg->zclHdr.commandID)
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zapp_fProcessInReadRspCmd(pInMsg);
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zapp_fProcessInWriteRspCmd(pInMsg);
      break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
    case ZCL_CMD_CONFIG_REPORT:
      //zapp_ProcessInConfigReportCmd(pInMsg);
#ifdef MT_DEBUG_FUNC
      MT_ProcessDebugString("ZCL_ConfigReportCmd");
#endif
      break;

    case ZCL_CMD_CONFIG_REPORT_RSP:
      //zapp_ProcessInConfigReportRspCmd(pInMsg);
#ifdef MT_DEBUG_FUNC
      MT_ProcessDebugString("ZCL_ConfigReportRsp");
#endif
      break;

    case ZCL_CMD_READ_REPORT_CFG:
      //zapp_ProcessInReadReportCfgCmd(pInMsg);
#ifdef MT_DEBUG_FUNC
      MT_ProcessDebugString("ZCL_ReadReportCfgCmd");
#endif
      break;

    case ZCL_CMD_READ_REPORT_CFG_RSP:
      //zapp_ProcessInReadReportCfgRspCmd(pInMsg);
      break;

    case ZCL_CMD_REPORT:
      //zapp_ProcessInReportCmd(pInMsg);
#ifdef MT_DEBUG_FUNC
      MT_ProcessDebugString("ZCL_ReportCmd");
#endif
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      zapp_fProcessInDefaultRspCmd(pInMsg);
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
      zapp_fProcessInDiscCmdsRspCmd(pInMsg);
      break;

    case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
      zapp_fProcessInDiscCmdsRspCmd(pInMsg);
      break;

    case ZCL_CMD_DISCOVER_ATTRS_RSP:
      zapp_fProcessInDiscAttrsRspCmd(pInMsg);
      
      break;

    case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
      zapp_fProcessInDiscAttrsExtRspCmd(pInMsg);
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
 * @fn      zapp_fProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zapp_fProcessInReadRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;
  uint8 status = 0;
  uint32 time;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  
  // Processing TIME read response command for time sync
  switch(pInMsg->clusterId)
  {
    case ZCL_CLUSTER_ID_GEN_TIME:
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
              zapp_TimeHasSynced = true;
              zapp_TimeSynchElapsedMinutes = 0;

              #if defined MT_DEBUG_FUNC
                MT_ProcessDebugString("ReadRsp.TimeSync");
              #endif
            }
          }
        }
      }
      break;
    default:
      break;
  }

  return TRUE;
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zapp_fProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zapp_fProcessInWriteRspCmd(zclIncomingMsg_t *pInMsg)
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

/*********************************************************************
 * @fn      zapp_fValidateAttrDataCB
 *
 * @brief   Process the "Profile" Write Command Validate Attr data
 *
 * @param   Attribute to write
 *
 * @return  return  TRUE if data valid. FALSE, otherwise.
 */
uint8 zapp_fValidateAddrDataCB(zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo)
{
  uint8 validate = TRUE;

  if (pAttr->clusterID == ZCL_CLUSTER_ID_SE_METERING)
  {
    switch (pAttrInfo->attrID)
    {
      case ATTRID_METER_0READINGSET_CURRSUMDELIVERED:
        break;
      case ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD:
        if (*pAttrInfo->attrData < WC_METER_INSTDEMAND_UPDATEPERIOD_MIN)
          *pAttrInfo->attrData = WC_METER_INSTDEMAND_UPDATEPERIOD_MIN;
        if ((*pAttrInfo->attrData != zapp_FlowUpdatePeriod) && (*pAttrInfo->attrData != WC_METER_INSTDEMAND_UPDATEPERIOD_MAX))
          osal_set_event(zapp_TaskID, WC_EVT_UPDATEINSTDEMAND);
        #ifdef MT_DEBUG_FUNC
          MT_ProcessDebugString("Write.UpdatePeriod");
        #endif
        break;
        
      case ATTRID_METER_0READINGSET_INTERVALREPORTING:
        zapp_fUpdateAttrIntervalReporting((uint16*)pAttrInfo->attrData);
        if (*(uint16*)pAttrInfo->attrData < zapp_FlowReportInterval) // If interval is decriced than update 
          osal_set_event(zapp_TaskID, WC_EVT_UPDATE);
        #ifdef MT_DEBUG_FUNC
          MT_ProcessDebugString("Write.IntervalReporting");
        #endif
        break;
        
      case ATTRID_METER_3FORMATTING_SITEID:
        if (*pAttrInfo->attrData > WC_METER_SITEID_SIZE)
        {
          validate = FALSE;
        }
        else
        {
        }
        #ifdef MT_DEBUG_FUNC
          MT_ProcessDebugString("Write.SiteID");
        #endif
        break;
        
      case ATTRID_METER_3FORMATTING_UNIT:
        if (*(uint8*)pAttrInfo->attrData != *(uint8*)pAttr->attr.dataPtr)
        {
          if (pAttr->attr.dataPtr == &zapp_Flow1Unit)
            zapp_AttrToStore |= WC_STOREID_UNIT1;
          else
            zapp_AttrToStore |= WC_STOREID_UNIT2;
          osal_start_timerEx(zapp_TaskID, WC_EVT_STOREATTR, WC_TIMEOUT_STOREATTR);
        }
        #ifdef MT_DEBUG_FUNC
          MT_ProcessDebugString("Write.Unit");
        #endif
        break;
        
      case ATTRID_METER_3FORMATTING_MULTIPLIER:
        if (*(uint24*)pAttrInfo->attrData != *(uint24*)pAttr->attr.dataPtr)
        {
          if (pAttr->attr.dataPtr == &zapp_Flow1Multiplier) // Endpoint8
            zapp_AttrToStore |= WC_STOREID_MULTIPLIER1;
          else
            zapp_AttrToStore |= WC_STOREID_MULTIPLIER2;  // Endpoint9
          osal_start_timerEx(zapp_TaskID, WC_EVT_STOREATTR, WC_TIMEOUT_STOREATTR);
        }
        #ifdef MT_DEBUG_FUNC
          MT_ProcessDebugString("Write.Multiplier");
        #endif
        break;
        
      case ATTRID_METER_3FORMATTING_DIVISOR:
        if (*(uint24*)pAttrInfo->attrData != *(uint24*)pAttr->attr.dataPtr) // Current and new values are diffrent
        {
          if (pAttr->attr.dataPtr == &zapp_Flow1Divisor) // Endpoint8
            zapp_AttrToStore |= WC_STOREID_DIVISOR1;
          else
            zapp_AttrToStore |= WC_STOREID_DIVISOR2;  // Endpoint9
          osal_start_timerEx(zapp_TaskID, WC_EVT_STOREATTR, WC_TIMEOUT_STOREATTR);
        }
        #ifdef MT_DEBUG_FUNC
          MT_ProcessDebugString("Write.Divisor");
        #endif
        break;
    }
  }
  else if (pAttr->clusterID == ZCL_CLUSTER_ID_GEN_POWER_CFG)
  {
    switch (pAttrInfo->attrID)
    {
      case ATTRID_POWER_CFG_BAT_RATED_VOLTAGE:
        zapp_fUpdateAttrRatedVoltage(pAttrInfo->attrData);
        if (*pAttrInfo->attrData != zapp_BatteryVoltageRated)
          osal_set_event(zapp_TaskID, WC_EVT_BATTERY);
        break;
    }
  }
  return validate;
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zapp_fAuthorizeCB
 *
 * @brief   Process the "Profile" Write Command Validate Attr data
 *
 * @param   Attribute to authorize
 *
 * @return  ZCL_STATUS_SUCCESS: Operation authorized
 *          ZCL_STATUS_NOT_AUTHORIZED: Operation not authorized
 */
ZStatus_t zapp_fAuthorizeCB( afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper )
{
  switch (pAttr->clusterID)
  {
    case ZCL_CLUSTER_ID_GEN_POWER_CFG:
      if (pAttr->attr.attrId == ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING)
        zapp_fUpdateBatteryAttributes();
      break;
    case ZCL_CLUSTER_ID_HA_DIAGNOSTIC:
      if (pAttr->attr.attrId == ATTRID_DIAG_3MEMALLOCATEDBLOCKS)
        zapp_DiagMemAllocatedBlocks = osal_heap_block_cnt();
      else if (pAttr->attr.attrId == ATTRID_DIAG_4MEMFREEBLOCKS)
        zapp_DiagMemFreeBlocks = osal_heap_block_free();
      else if (pAttr->attr.attrId == ATTRID_DIAG_5MEMUSED)
        zapp_DiagMemUsed = osal_heap_mem_used();
      else if (pAttr->attr.attrId == ATTRID_DIAG_6MEMHIGHWATER)
        zapp_DiagMemHighWater = osal_heap_high_water();
      break;
  }    
  return ZCL_STATUS_SUCCESS;
}
/*********************************************************************
 * @fn      zapp_fProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zapp_fProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg)
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
  // Device is notified of the Default Response command.
  (void)pInMsg;
  return TRUE;
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zapp_fProcessInDiscCmdsRspCmd
 *
 * @brief   Process the Discover Commands Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zapp_fProcessInDiscCmdsRspCmd(zclIncomingMsg_t *pInMsg)
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
 * @fn      zapp_fProcessInDiscAttrsRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zapp_fProcessInDiscAttrsRspCmd(zclIncomingMsg_t *pInMsg)
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
 * @fn      zapp_fProcessInDiscAttrsExtRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Extended Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zapp_fProcessInDiscAttrsExtRspCmd(zclIncomingMsg_t *pInMsg)
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
 * @fn      zapp_fProcessOTAMsgs
 *
 * @brief   Called to process callbacks from the ZCL OTA.
 *
 * @param   none
 *
 * @return  none
 */
static void zapp_fProcessOTAMsgs(zclOTA_CallbackMsg_t* pMsg)
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
    osal_start_timerEx(zapp_TaskID, WC_EVT_IMPULSE1, WC_TIMEOUT_DEBOUNCE);
  }
  if (P1IFG & BV(1))
  {
    osal_start_timerEx(zapp_TaskID, WC_EVT_IMPULSE2, WC_TIMEOUT_DEBOUNCE);
  }
  P1IFG = 0;
  
  HAL_EXIT_ISR();
}

/****************************************************************************
****************************************************************************/

