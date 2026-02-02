/**************************************************************************************************
  Filename:       zcl_watercounter_data.c
  Revised:        $Date: 2014-07-30 12:57:37 -0700 (Wed, 30 Jul 2014) $
  Revision:       $Revision: 39591 $


  Description:    Zigbee Cluster Library - sample device application.


  Copyright 2006-2014 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_poll_control.h"
#include "zcl_electrical_measurement.h"
#ifdef ZCL_DIAGNOSTIC
#include "zcl_diagnostic.h"
#endif
#include "zcl_meter_identification.h"
#include "zcl_appliance_identification.h"
#include "zcl_appliance_events_alerts.h"
#include "zcl_power_profile.h"
#include "zcl_appliance_control.h"
#include "zcl_appliance_statistics.h"
#include "zcl_hvac.h"

#include "zcl_watercounter.h"

/*********************************************************************
 * CONSTANTS
 */

#define ZAPP_DEVICE_VERSION     1
#define ZAPP_FLAGS              0

#define ZAPP_HWVERSION          0
#define ZAPP_ZCLVERSION         0

#define DEFAULT_PHYSICAL_ENVIRONMENT 0
#define DEFAULT_DEVICE_ENABLE_STATE DEVICE_ENABLED
#define DEFAULT_IDENTIFY_TIME 0

#define ENGINEERING_UNIT_METER_KWH    0x00
#define ENGINEERING_UNIT_METER_M3     0x01
#define ENGINEERING_UNIT_METER_FT3    0x02
#define ENGINEERING_UNIT_METER_CCF    0x03      // (100 or Centum) Cubic Feet
#define ENGINEERING_UNIT_METER_USGL   0x04      // US Gallons
#define ENGINEERING_UNIT_METER_IMPGL  0x05      // Imperial Gallons
#define ENGINEERING_UNIT_METER_BTU    0x06
#define ENGINEERING_UNIT_METER_L      0x07
#define ENGINEERING_UNIT_METER_KPA    0x08      // gauge
#define ENGINEERING_UNIT_METER_KPAA   0x09      // absolute
#define ENGINEERING_UNIT_METER_MCF    0x0A      // 1000 Cubic Feet
#define ENGINEERING_UNIT_METER_UNITLESS  0x0B   // Unitless
#define ENGINEERING_UNIT_METER_MJ        0x0C   // Mega Joule
#define ENGINEERING_UNIT_TIME_SEC      73
#define ENGINEERING_UNIT_TIME_MIN      72
#define ENGINEERING_UNIT_TIME_HOUR     71
#define ENGINEERING_UNIT_TIME_DAY      70

// 0x0401 - 0x0FFF application NV ids
#define ZAPP_NV_SITEID1           0x0404
#define ZAPP_NV_SITEID2           0x0405
#define ZAPP_NV_UNIT1             0x0406
#define ZAPP_NV_UNIT2             0x0407
#define ZAPP_NV_MULTIPLIER1       0x0408
#define ZAPP_NV_MULTIPLIER2       0x0409
#define ZAPP_NV_DIVISOR1          0x040A
#define ZAPP_NV_DIVISOR2          0x040B
#define ZAPP_NV_VOLUMEREPORT1     0x040C
#define ZAPP_NV_VOLUMEREPORT2     0x040D
#define ZAPP_NV_REPORTPERIOD      0x040E
#define ZAPP_NV_VOLTAGERATED      0x040F
#define ZAPP_NV_VALUES1           0x0410
#define ZAPP_NV_DATE1             0x0411
#define ZAPP_NV_VALUES2           0x0412
#define ZAPP_NV_DATE2             0x0413
#define ZAPP_NV_DIAGDEBOUNCE      0x0414

#define ZAPP_NV_FIRST             ZAPP_NV_SITEID1
#define ZAPP_NV_FIRST_1           (ZAPP_NV_FIRST - 1)

#define ZAPP_STOREQUEUE_LEN          15

#pragma message(__DATE__)
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

//global attributes
const uint16 zapp_clusterRevision_all = 0x0001; //currently all cluster implementations are according to ZCL6, which has revision #1. In the future it is possible that different clusters will have different revisions, so they will have to use separate attribute variables.

// Basic Cluster
const uint8 zapp_HWRevision = ZAPP_HWVERSION;
const uint8 zapp_ZCLVersion = ZAPP_ZCLVERSION;
const uint8 zapp_ManufacturerName[] = { 15, 'F','o','x','.','I','n','s','t','r','u','m','e','n','t','s' };
const uint8 zapp_ModelId[] = { 12, 'F','O','X','-','M','e','t','e','r','0','0','1',};
const uint8 zapp_DateCode[] = { 11, __DATE__[0], __DATE__[1], __DATE__[2], __DATE__[3], __DATE__[4], __DATE__[5], __DATE__[6], __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10]};
const uint8 zapp_SWBuildID[] = { 11, 'b', 'u', 'i', 'l', 'd', ' ', 48 + __BUILD_NUMBER__/10000, 48 + ((__BUILD_NUMBER__/1000)%10), 48 + ((__BUILD_NUMBER__/100)%10), 48 + ((__BUILD_NUMBER__/10)%10), 48 + (__BUILD_NUMBER__%10)};

const uint8 zapp_DeviceType = 0x02;    // Water meter
const uint8 zapp_PowerSource = POWER_SOURCE_BATTERY;

const uint8 zapp_cDesc1[ZAPP_METER_SITEID_SIZE + 1] = {ZAPP_METER_SITEID_SIZE, 'C', 'o', 'l', 'd', ' ', ' ', ' ', ' ', ' ', ' '};   // Constants to initialize default values
const uint8 zapp_cDesc2[ZAPP_METER_SITEID_SIZE + 1] = {ZAPP_METER_SITEID_SIZE, 'H', 'o', 't', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

// SE Metering cluster attribute
uint8 zapp_Flow1SiteId[ZAPP_METER_SITEID_SIZE + 1];
uint48_t zapp_Flow1Value;
int24 zapp_Flow1InstDemand;
int24 zapp_Flow1InstDemandPrev;
uint24 zapp_Flow1Multiplier;
uint24 zapp_Flow1Divisor;
uint8 zapp_Flow1Unit;
uint16 zapp_Flow1VolumePerReport;
uint24 zapp_Flow1CurrDay;
uint24 zapp_Flow1PrevDay;
uint8 zapp_Flow1Status;

uint8 zapp_Flow2SiteId[ZAPP_METER_SITEID_SIZE + 1];
uint48_t zapp_Flow2Value;
int24 zapp_Flow2InstDemand;
int24 zapp_Flow2InstDemandPrev;
uint24 zapp_Flow2Multiplier;
uint24 zapp_Flow2Divisor;
uint16 zapp_Flow2Unit;
uint16 zapp_Flow2VolumePerReport;
uint24 zapp_Flow2CurrDay;
uint24 zapp_Flow2PrevDay;
uint8 zapp_Flow2Status;

// Power cluster attributes
uint8 zapp_BatteryVoltage;            // 0.1V unit
uint8 zapp_BatteryVoltageRated;        // 0.1V unit, 3.6V LiFePO4
uint8 zapp_BatteryVoltageThresMin;    // 0.1V unit, 2.5V LiFePO4 T>0 degC
uint8 zapp_BatteryVoltageThres1;
uint8 zapp_BatteryLevel;              // 0 - 100%
uint8 zapp_BatteryAlarmMask;
uint32 zapp_BatteryAlarmState;

// Alarm cluster attributes
uint16 zapp_AlarmCount;

// Time cluster attributes
/* uint32 zapp_Time;
uint8 zapp_TimeStatus;
uint32 zapp_TimeLocal; */

uint8 zapp_FlowUpdatePeriod;   // InstDemand update time period in seconds
uint16 zapp_FlowIntervalReporting; // Time interval in minutes
uint24 zapp_Flow1HoursInOperation;
uint24 zapp_Flow2HoursInOperation;

uint8 zapp_LocationDescription[16];
uint8 zapp_PhysicalEnvironment;
uint8 zapp_DeviceEnable = DEVICE_ENABLED;

// Diagnostic cluster
uint16 zapp_DiagNumOfResets;
uint16 zapp_DiagNVMemWrites;
uint16 zapp_DiagNVMemErasedPages;
uint16 zapp_DiagNVMemWriteFails;
uint32 zapp_DiagNVMemFailItems;
uint16 zapp_DiagMemAllocatedBlocks;
uint16 zapp_DiagMemFreeBlocks;
uint16 zapp_DiagMemUsed;       // Used memory in bytes
uint16 zapp_DiagMemHighWater;  // Maximum memory allocated
uint16 zapp_DiagCPUStatus;     // CPU status - Clock, Power mode, Boot reason
uint32 zapp_DiagSystemUpTime;  // System up time in seconds
uint8 zapp_DiagReport;         // Send Diag data every meter update period
uint16 zapp_DiagDebounce;        // ms
uint16 zapp_DiagDebounceFlow1;
uint16 zapp_DiagDebounceFlow2;

uint8 zapp_StoreQueueItems[ZAPP_STOREQUEUE_LEN];
uint8 zapp_StoreQueueSizes[ZAPP_STOREQUEUE_LEN];
void* zapp_StoreQueueSources[ZAPP_STOREQUEUE_LEN];

// Identify Cluster
uint16 zapp_IdentifyTime = 0;
// Constants set Attribute value limitations
const uint16 zapp_cReportIntervals[] = {5, 10, 15, 20, 30, 60, 2*60, 3*60, 4*60, 6*60, 8*60, 12*60, 24*60};
CONST uint8 zapp_cReportIntervalsSize_1 = sizeof(zapp_cReportIntervals) / sizeof(zapp_cReportIntervals[0]) - 1;
const uint8 zapp_cRatedVoltages[] = {VDD_VOLTAGE_RATED30, VDD_VOLTAGE_RATED33, VDD_VOLTAGE_RATED36, VDD_VOLTAGE_RATED42};
CONST uint8 zapp_cRatedVoltagesSize_1 = sizeof(zapp_cRatedVoltages) / sizeof(zapp_cRatedVoltages[0]) - 1;
const uint8 zapp_cRatedVoltageThres[] = {28, 28, VDD_VOLTAGE36_THRES1, 29};
const uint8 zapp_cRatedVoltageThresMin[] = {27, 27, VDD_VOLTAGE36_MIN, 26};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
 
  // NOTE: The attributes listed in the AttrRec must be in ascending order 
  // per cluster to allow right function of the Foundation discovery commands
 
CONST zclAttrRec_t zapp_cAttrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //1
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zapp_ZCLVersion
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  //2
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zapp_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //3
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zapp_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //4
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zapp_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //5
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zapp_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //6
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8, ACCESS_CONTROL_READ,
      (void *)&zapp_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //7
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zapp_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //8
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //9
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_DeviceEnable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //10
      ATTRID_BASIC_SW_BUILD_ID,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)&zapp_SWBuildID
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //10
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zapp_clusterRevision_all
    }
  },
  
  // ***** Battery status cluster ***** //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //11
      ATTRID_POWER_CFG_BATTERY_VOLTAGE,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zapp_BatteryVoltage
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //12
      ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_CONTROL_AUTH_READ,
      (void *)&zapp_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //13
      ATTRID_POWER_CFG_BAT_RATED_VOLTAGE,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&zapp_BatteryVoltageRated
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //14
      ATTRID_POWER_CFG_BAT_ALARM_MASK,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&zapp_BatteryAlarmMask
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //15
      ATTRID_POWER_CFG_BAT_VOLT_MIN_THRES,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zapp_BatteryVoltageThresMin
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //16
      ATTRID_POWER_CFG_BAT_VOLT_THRES_1,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zapp_BatteryVoltageThres1
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //17
      ATTRID_POWER_CFG_BAT_ALARM_STATE,
      ZCL_DATATYPE_BITMAP32, ACCESS_CONTROL_READ,
      (void *)&zapp_BatteryAlarmState
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //18
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zapp_clusterRevision_all
    }
  },
  // ********* Identify Cluster Attribute ********* //
  // ********************************************** //
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { //19
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_IdentifyTime
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  //20
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_GLOBAL),
      (void *)&zapp_clusterRevision_all
    }
  },
  // ********* Groups Cluster ********* //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_GROUPS,
    { //21
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zapp_clusterRevision_all
    }
  },
  // ********* Alarm Cluster ********* //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_ALARMS,
    { //22
      ATTRID_ALARM_COUNT,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zapp_AlarmCount
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ALARMS,
    { //22
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zapp_clusterRevision_all
    }
  },
  // ********* Time Cluster ********* //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    { //22
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zapp_clusterRevision_all
    }
  },
  // ********* Flow measure Cluster ********* //
  // **************************************** //
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //23
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow1Value
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //24
      ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_FlowUpdatePeriod
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //25
      ATTRID_METER_0READINGSET_INTERVALREPORTING,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_FlowIntervalReporting
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //26
      ATTRID_METER_0READINGSET_VOLUMEPERREPORT,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow1VolumePerReport
    }
  },
    {
    ZCL_CLUSTER_ID_SE_METERING,
    { //27
      ATTRID_METER_2STATUS_STATUS,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow1Status
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //28
      ATTRID_METER_2STATUS_REMAININGBATT,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ),
      (void *)&zapp_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //29
      ATTRID_METER_2STATUS_HOURSINOPERATION,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow1HoursInOperation
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //30
      ATTRID_METER_3FORMATTING_UNIT,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow1Unit
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //31
      ATTRID_METER_3FORMATTING_MULTIPLIER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow1Multiplier
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //32
      ATTRID_METER_3FORMATTING_DIVISOR,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow1Divisor
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //33
      ATTRID_METER_3FORMATTING_METERDEVICETYPE,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zapp_DeviceType
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //34
      ATTRID_METER_3FORMATTING_SITEID,
      ZCL_DATATYPE_OCTET_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow1SiteId
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //35
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow1InstDemand
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //36
      ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow1CurrDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //37
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow1PrevDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //38
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zapp_clusterRevision_all
    }
  },
  // *********  Diagnostic Cluster  ********* //
  // **************************************** //
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //39
      ATTRID_DIAG_0NUMOFRESETS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zapp_DiagNumOfResets
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //40
      ATTRID_DIAG_1NVWRITES,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zapp_DiagNVMemWrites
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //41
      ATTRID_DIAG_2NVWRITEFAILS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zapp_DiagNVMemWriteFails
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //42
      ATTRID_DIAG_3NVFAILITEMS,
      ZCL_DATATYPE_UINT32, (ACCESS_CONTROL_READ),
      (void *)&zapp_DiagNVMemFailItems
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //43
      ATTRID_DIAG_4MEMALLOCATEDBLOCKS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_AUTH_READ),
      (void *)&zapp_DiagMemAllocatedBlocks
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //44
      ATTRID_DIAG_5MEMFREEBLOCKS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_AUTH_READ),
      (void *)&zapp_DiagMemFreeBlocks
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //45
      ATTRID_DIAG_6MEMUSED,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_AUTH_READ),
      (void *)&zapp_DiagMemUsed
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //46
      ATTRID_DIAG_7MEMHIGHWATER,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_AUTH_READ),
      (void *)&zapp_DiagMemHighWater
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //47
      ATTRID_DIAG_8CPUSTATUS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_AUTH_READ),
      (void *)&zapp_DiagCPUStatus
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //48
      ATTRID_DIAG_9SYSTEMUPTIME,
      ZCL_DATATYPE_UINT32, (ACCESS_CONTROL_READ | ACCESS_CONTROL_AUTH_READ),
      (void *)&zapp_DiagSystemUpTime
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //49
      ATTRID_DIAG_10REPORT,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_DiagReport
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //50
      ATTRID_DIAG_11ERASEDPAGES,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zapp_DiagNVMemErasedPages
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //51
      ATTRID_DIAG_12DEBOUNCE,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_DiagDebounce
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //52
      ATTRID_DIAG_13DEBOUNCE_FLOW1,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_DiagDebounceFlow1
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //53
      ATTRID_DIAG_14DEBOUNCE_FLOW2,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_DiagDebounceFlow2
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //54
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zapp_clusterRevision_all
    }
  }
};

CONST uint8 zapp_cNumAttributes = ( sizeof(zapp_cAttrs) / sizeof(zapp_cAttrs[0]) );

CONST zclAttrRec_t zapp_cAttrs2[] =
{
  // ********* Flow measure Cluster ********* //
  // **************************************** //
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //1
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow2Value
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //2
      ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_FlowUpdatePeriod
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //3
      ATTRID_METER_0READINGSET_INTERVALREPORTING,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_FlowIntervalReporting
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //4
      ATTRID_METER_0READINGSET_VOLUMEPERREPORT,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow2VolumePerReport
    }
  },
    {
    ZCL_CLUSTER_ID_SE_METERING,
    { //5
      ATTRID_METER_2STATUS_STATUS,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow2Status
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //6
      ATTRID_METER_2STATUS_REMAININGBATT,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ),
      (void *)&zapp_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //7
      ATTRID_METER_2STATUS_HOURSINOPERATION,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow2HoursInOperation
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //8
      ATTRID_METER_3FORMATTING_UNIT,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow2Unit
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //9
      ATTRID_METER_3FORMATTING_MULTIPLIER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow2Multiplier
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //10
      ATTRID_METER_3FORMATTING_DIVISOR,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow2Divisor
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //11
      ATTRID_METER_3FORMATTING_METERDEVICETYPE,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zapp_DeviceType
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //12
      ATTRID_METER_3FORMATTING_SITEID,
      ZCL_DATATYPE_OCTET_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zapp_Flow2SiteId
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //13
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow2InstDemand
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //14
      ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow2CurrDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //15
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zapp_Flow2PrevDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //16
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zapp_clusterRevision_all
    }
  }
};

CONST uint8 zapp_cNumAttributes2 = ( sizeof(zapp_cAttrs2) / sizeof(zapp_cAttrs2[0]) );

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t zapp_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_POWER_CFG,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_TIME,
  ZCL_CLUSTER_ID_SE_METERING,
  ZCL_CLUSTER_ID_HA_DIAGNOSTIC
};

#define zapp_MAX_INCLUSTERS    ( sizeof( zapp_InClusterList ) / sizeof( zapp_InClusterList[0] ))

const cId_t zapp_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_GROUPS,
  ZCL_CLUSTER_ID_GEN_ALARMS
};

#define zapp_MAX_OUTCLUSTERS   ( sizeof( zapp_OutClusterList ) / sizeof( zapp_OutClusterList[0] ))

SimpleDescriptionFormat_t zapp_SimpleDesc =
{
  ZAPP_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                    //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_METER_INTERFACE,      //  uint16 AppDeviceId[2];
  ZAPP_DEVICE_VERSION,            //  int   AppDevVer:4;
  ZAPP_FLAGS,                     //  int   AppFlags:4;
  zapp_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zapp_InClusterList, //  byte *pAppInClusterList;
  zapp_MAX_OUTCLUSTERS,        //  byte  AppNumOutClusters;
  (cId_t *)zapp_OutClusterList //  byte *pAppOutClusterList;
};

const cId_t zapp_InClusterList2[] =
{
  ZCL_CLUSTER_ID_SE_METERING
};

#define zapp_MAX_INCLUSTERS2    ( sizeof( zapp_InClusterList2 ) / sizeof( zapp_InClusterList2[0] ))

SimpleDescriptionFormat_t zapp_SimpleDesc2 =
{
  ZAPP_ENDPOINT2,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                    //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_METER_INTERFACE,      //  uint16 AppDeviceId[2];
  ZAPP_DEVICE_VERSION,            //  int   AppDevVer:4;
  ZAPP_FLAGS,                     //  int   AppFlags:4;
  zapp_MAX_INCLUSTERS2,         //  byte  AppNumInClusters;
  (cId_t *)zapp_InClusterList2, //  byte *pAppInClusterList;
  0,                            //  byte  AppNumOutClusters;
  NULL                          //  byte *pAppOutClusterList;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zapp_fNVInitItems
 *
 * @brief   Initialize items in NV for Attribute values.
 *
 * @param   none
 *
 * @return  SUCCESS, FAILURE
 */
uint8 zapp_fNVInitItems(void)
{
  uint8 st;
  zapp_DiagNVMemFailItems = 0;
  // NV_ITEM_UNINIT - Id did not exist and was created successfully.
  // SUCCESS        - Id already existed, no action taken 
  st = osal_nv_item_init(ZAPP_NV_SITEID1, ZAPP_METER_SITEID_SIZE + 1, NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_SITEID1 : 0;
  st = osal_nv_item_init(ZAPP_NV_SITEID2, ZAPP_METER_SITEID_SIZE + 1, NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_SITEID2 : 0;
  st = osal_nv_item_init(ZAPP_NV_UNIT1, sizeof(zapp_Flow1Unit), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_UNIT1 : 0;
  st = osal_nv_item_init(ZAPP_NV_UNIT2, sizeof(zapp_Flow2Unit), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_UNIT2 : 0;
  st = osal_nv_item_init(ZAPP_NV_MULTIPLIER1, sizeof(zapp_Flow1Multiplier), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_MULTIPLIER1 : 0;
  st = osal_nv_item_init(ZAPP_NV_MULTIPLIER2, sizeof(zapp_Flow2Multiplier), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_MULTIPLIER2 : 0;
  st = osal_nv_item_init(ZAPP_NV_DIVISOR1, sizeof(zapp_Flow1Divisor), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_DIVISOR1 : 0;
  st = osal_nv_item_init(ZAPP_NV_DIVISOR2, sizeof(zapp_Flow2Divisor), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_DIVISOR2 : 0;
  st = osal_nv_item_init(ZAPP_NV_VOLUMEREPORT1, sizeof(zapp_Flow1VolumePerReport), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_VOLUMEREPORT1 : 0;
  st = osal_nv_item_init(ZAPP_NV_VOLUMEREPORT2, sizeof(zapp_Flow2VolumePerReport), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_VOLUMEREPORT2 : 0;
  st = osal_nv_item_init(ZAPP_NV_REPORTPERIOD, sizeof(zapp_FlowIntervalReporting), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_REPORTPERIOD : 0;  
  st = osal_nv_item_init(ZAPP_NV_VOLTAGERATED, sizeof(zapp_BatteryVoltageRated), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_VOLTAGERATED : 0;  
  st = osal_nv_item_init(ZAPP_NV_VALUES1, sizeof(zapp_Flow1Value.dw.lowDW), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_VALUES1 : 0;;
  st = osal_nv_item_init(ZAPP_NV_DATE1, sizeof(uint32), NULL);
  zapp_DiagNVMemFailItems |= st != SUCCESS && st != NV_ITEM_UNINIT ? 1<<ZAPP_STOREID_DATE1 : 0;
  st = osal_nv_item_init(ZAPP_NV_VALUES2, sizeof(zapp_Flow2Value.dw.lowDW), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_VALUES2 : 0;
  st = osal_nv_item_init(ZAPP_NV_DATE2, sizeof(uint32), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1<<ZAPP_STOREID_DATE2 : 0;
  st = osal_nv_item_init(ZAPP_NV_DIAGDEBOUNCE, sizeof(zapp_DiagDebounce), NULL);
  zapp_DiagNVMemFailItems |= ((st != SUCCESS) && (st != NV_ITEM_UNINIT)) ? 1L<<ZAPP_STOREID_DIAGDEBOUNCE : 0;
  return (st);
}

/*********************************************************************
 * @fn      zapp_fNVCheckItem
 *
 * @brief   Check NV item whether it was stored in NV memory.
 *
 * @param   id - NV item id number
 *
 * @return  SUCCESS, FAILURE
 */
uint8 zapp_fNVCheckItem(uint16 id, uint16 len)
{
  uint32 buf;
  
  if (len > 4)
    len = 4;
  
  if ((zapp_DiagNVMemFailItems & (1 << (id - ZAPP_NV_FIRST))) == 0)
  {
    if (osal_nv_read(id, 0, len, &buf) == SUCCESS)
    {
      buf |= 0xFFFFFFFF << len*8;
      if (buf != 0xFFFFFFFF)
      {
        return SUCCESS;
      }
    }
  }
  return FAILURE;
}

/*********************************************************************
 * @fn      zapp_fInitAttrValue
 *
 * @brief   Init attribute from NV stored item or Default value.
 *
 * @param   none
 *
 * @return  none
 */
void zapp_fInitAttrValue(uint16 id, uint16 len, const void *src, void *buf)
{
  if (zapp_fNVCheckItem(id, len) == SUCCESS)
  {
    if (osal_nv_read(id, 0, len, buf) == SUCCESS)
      return;
  }
  
  osal_memcpy(buf, src, len);
}

/*********************************************************************
 * @fn      zapp_fUpdateAttrIntervalReporting
 *
 * @brief   Update value according to array of valid values
 *
 * @param   checked value
 *
 * @return  valid value
 */
void zapp_fUpdateAttrIntervalReporting(uint16 *data)
{
  uint8 l, r, m;

  l = 0;
  r = zapp_cReportIntervalsSize_1;
  m = 5;
  while (l <= r)
  {
    m = l + (r - l) / 2;
    if (*data < zapp_cReportIntervals[m] - (zapp_cReportIntervals[m] - zapp_cReportIntervals[m - 1])/2)
    {
      if (m == 1) { m = 0; break; }
      r = m - 1;
    }
    else if (*data > zapp_cReportIntervals[m] + (zapp_cReportIntervals[m + 1] - zapp_cReportIntervals[m])/2)
    {
      if (m == zapp_cReportIntervalsSize_1 - 1) { m = zapp_cReportIntervalsSize_1; break; }
      l = m + 1;
    }
    else
      break;
  }
  *data = zapp_cReportIntervals[m];
}

/*********************************************************************
 * @fn      zapp_fUpdateAttrRatedVoltage
 *
 * @brief   Update value according to array of valid values
 *
 * @param   checked value
 *
 * @return  valid value
 */
void zapp_fUpdateAttrRatedVoltage(uint8 *data)
{
  uint8 i;
  
  for (i = 0; i <= zapp_cRatedVoltagesSize_1; i++)
  {
    if (*data <= zapp_cRatedVoltages[i])
    {
      *data = zapp_cRatedVoltages[i];
      zapp_BatteryVoltageThres1 = zapp_cRatedVoltageThres[i];
      zapp_BatteryVoltageThresMin = zapp_cRatedVoltageThresMin[i];
      break;
    }
  }
  if (i > zapp_cRatedVoltagesSize_1)
  {
    *data = zapp_cRatedVoltages[zapp_cRatedVoltagesSize_1];
    zapp_BatteryVoltageThres1 = zapp_cRatedVoltageThres[zapp_cRatedVoltagesSize_1];
    zapp_BatteryVoltageThresMin = zapp_cRatedVoltageThresMin[zapp_cRatedVoltagesSize_1];
  }
}

/*********************************************************************
 * @fn      zapp_fStoreQueueAdd
 *
 * @brief   Store attributes to NV memory.
 *
 * @param   NV item, Size, pointer to attribute variable
 *
 * @return  none
 */
Status_t zapp_fStoreQueueAdd(uint8 idx, uint8 size, void *src)
{
  uint8 i;
  
  idx++;
  
  for (i = 0; i < ZAPP_STOREQUEUE_LEN; i++)
  {
    if (zapp_StoreQueueItems[i] == idx) break;
    if (zapp_StoreQueueItems[i] == 0)
    {
      zapp_StoreQueueItems[i] = idx;
      zapp_StoreQueueSizes[i] = size;
      zapp_StoreQueueSources[i] = src;
      break;
    }
  }
  if (i == ZAPP_STOREQUEUE_LEN) return FAILURE;
  
  return SUCCESS;
}

/*********************************************************************
 * @fn      zapp_fStoreAttrToNV
 *
 * @brief   Store attributes to NV memory.
 *
 * @param   none
 *
 * @return  bitmask with failed itmes
 */
uint32 zapp_fStoreAttrToNV(void)
{
  uint8 i;
  uint32 st = 0;
  
  for (i = 0; i < ZAPP_STOREQUEUE_LEN; i++)
  {
    if (zapp_StoreQueueItems[i] != 0)
    {
      if (osal_nv_write(ZAPP_NV_FIRST_1 + zapp_StoreQueueItems[i], 0, zapp_StoreQueueSizes[i], zapp_StoreQueueSources[i]) == SUCCESS)
        zapp_DiagNVMemWrites = osal_nv_get_writtenbytes();
      else
      {
        zapp_DiagNVMemWriteFails++;
        st |=  1 << (zapp_StoreQueueItems[i] - 1);
      }
      
      zapp_StoreQueueItems[i] = 0;
      zapp_StoreQueueSizes[i] = 0;
      zapp_StoreQueueSources[i] = 0;
    }
    else
      break;
  }
  
  if (st != 0)
    zapp_DiagNVMemFailItems |= st; // add New failed items to existed ones
  
  return (st); // return New failed items
}

/*********************************************************************
 * @fn      zapp_fResetAttributesToDefaultValues
 *
 * @brief   Reset all writable attributes to their default values.
 *
 * @param   none
 *
 * @return  none
 */
void zapp_fResetAttributesToDefaultValues(void)
{
  int i;
  uint32 src;
 // uint8 result;
  
  zapp_LocationDescription[0] = 15;
  for (i = 1; i <= 15; i++)
  {
    zapp_LocationDescription[i] = ' ';
  }
  
  zapp_PhysicalEnvironment = DEFAULT_PHYSICAL_ENVIRONMENT;
  zapp_DeviceEnable = DEFAULT_DEVICE_ENABLE_STATE;
  
  zapp_IdentifyTime = DEFAULT_IDENTIFY_TIME;
  
  //osal_memcpy(zapp_Flow1SiteId, zapp_cDesc1, ZAPP_METER_SITEID_SIZE + 1);
  //osal_memcpy(zapp_Flow2SiteId, zapp_cDesc2, ZAPP_METER_SITEID_SIZE + 1);
  
  zapp_fInitAttrValue(ZAPP_NV_SITEID1, ZAPP_METER_SITEID_SIZE + 1, zapp_cDesc1, zapp_Flow1SiteId);
  zapp_fInitAttrValue(ZAPP_NV_SITEID2, ZAPP_METER_SITEID_SIZE + 1, zapp_cDesc2, zapp_Flow2SiteId);
  
  src = 0;
  zapp_fInitAttrValue(ZAPP_NV_VALUES1, sizeof(zapp_Flow1Value.dw.lowDW), &src, &zapp_Flow1Value.dw.lowDW);
  zapp_fInitAttrValue(ZAPP_NV_VALUES2, sizeof(zapp_Flow1Value.dw.lowDW), &src, &zapp_Flow2Value.dw.lowDW);
  
  //zapp_Flow1Value.dw.lowDW = 0;
  zapp_Flow1Value.dw.hiW = 0;
  //zapp_Flow2Value.dw.lowDW = 0;
  zapp_Flow2Value.dw.hiW = 0;
  zapp_Flow1InstDemand = 0;
  zapp_Flow2InstDemand = 0;
  zapp_Flow1InstDemandPrev = 0;
  zapp_Flow2InstDemandPrev = 0;
  zapp_Flow1PrevDay = 0;
  zapp_Flow2PrevDay = 0;
  zapp_Flow1CurrDay = 0;
  zapp_Flow2CurrDay = 0;
  zapp_Flow1HoursInOperation = 0;
  zapp_Flow2HoursInOperation = 0;
  
  src = ZAPP_METER_MULTIPLIER;
  zapp_fInitAttrValue(ZAPP_NV_MULTIPLIER1, sizeof(zapp_Flow1Multiplier), &src, &zapp_Flow1Multiplier);
  zapp_fInitAttrValue(ZAPP_NV_MULTIPLIER2, sizeof(zapp_Flow2Multiplier), &src, &zapp_Flow2Multiplier);
  //zapp_Flow1Multiplier = ZAPP_METER_MULTIPLIER;
  //zapp_Flow2Multiplier = ZAPP_METER_MULTIPLIER;
  src = ZAPP_METER_DIVISOR;
  zapp_fInitAttrValue(ZAPP_NV_DIVISOR1, sizeof(zapp_Flow1Divisor), &src, &zapp_Flow1Divisor);
  zapp_fInitAttrValue(ZAPP_NV_DIVISOR2, sizeof(zapp_Flow2Divisor), &src, &zapp_Flow2Divisor);
  //zapp_Flow1Divisor = ZAPP_METER_DIVISOR;
  //zapp_Flow2Divisor = ZAPP_METER_DIVISOR;
  src = ENGINEERING_UNIT_METER_M3;
  zapp_fInitAttrValue(ZAPP_NV_UNIT1, sizeof(zapp_Flow1Unit), &src, &zapp_Flow1Unit);
  zapp_fInitAttrValue(ZAPP_NV_UNIT2, sizeof(zapp_Flow2Unit), &src, &zapp_Flow2Unit);
  //zapp_Flow1Unit = ENGINEERING_UNIT_METER_M3;
  //zapp_Flow2Unit = ENGINEERING_UNIT_METER_M3;
  src = ZAPP_VOLUME_PER_REPORT;
  zapp_fInitAttrValue(ZAPP_NV_VOLUMEREPORT1, sizeof(zapp_Flow1VolumePerReport), &src, &zapp_Flow1VolumePerReport);
  zapp_fInitAttrValue(ZAPP_NV_VOLUMEREPORT2, sizeof(zapp_Flow2VolumePerReport), &src, &zapp_Flow2VolumePerReport);
  //zapp_Flow1VolumePerReport = ZAPP_VOLUME_PER_REPORT;
  //zapp_Flow2VolumePerReport = ZAPP_VOLUME_PER_REPORT;
  zapp_Flow1Status = 0;
  zapp_Flow2Status = 0;
  
  zapp_FlowUpdatePeriod = ZAPP_METER_INSTDEMAND_UPDATEPERIOD_MAX;   // InstDemand update interval
  src = ZAPP_METER_REPORT_INTERVAL;
  zapp_fInitAttrValue(ZAPP_NV_REPORTPERIOD, sizeof(zapp_FlowIntervalReporting), &src, &zapp_FlowIntervalReporting);
  //zapp_FlowIntervalReporting = ZAPP_METER_REPORT_INTERVAL; // Report interal in minutes
  
  //zapp_BatteryVoltageRated = VDD_VOLTAGE_RATED36;
  src = VDD_VOLTAGE_RATED36;
  zapp_fInitAttrValue(ZAPP_NV_VOLTAGERATED, sizeof(zapp_BatteryVoltageRated), &src, &zapp_BatteryVoltageRated);
  zapp_BatteryVoltageThresMin = VDD_VOLTAGE36_MIN;
  zapp_BatteryVoltageThres1 = VDD_VOLTAGE36_THRES1;
  zapp_BatteryAlarmMask = BAT_ALARM_MASK_VOLT_2_LOW | BAT_ALARM_MASK_BATTERY_ALARM_1;
  zapp_BatteryAlarmState = 0;
  
  zapp_DiagNumOfResets = 0;
  zapp_DiagNVMemWrites = osal_nv_get_writtenbytes();
  zapp_DiagNVMemErasedPages = osal_nv_get_erasedpages();
  zapp_DiagNVMemWriteFails = 0;
  zapp_DiagMemAllocatedBlocks = osal_heap_block_cnt();
  zapp_DiagMemFreeBlocks = osal_heap_block_free();
  zapp_DiagMemUsed = osal_heap_mem_used();
  zapp_DiagMemHighWater = osal_heap_high_water();
  zapp_DiagSystemUpTime = osal_GetSystemClockSec();
  zapp_DiagReport = ZAPP_DIAG_REPORT_DONT;
  src = ZAPP_TIMEOUT_DEBOUNCE;
  zapp_fInitAttrValue(ZAPP_NV_DIAGDEBOUNCE, sizeof(zapp_DiagDebounce), &src, &zapp_DiagDebounce);
  zapp_DiagDebounceFlow1 = 0;
  zapp_DiagDebounceFlow2 = 0;
  
  zapp_AlarmCount = 0;
  
  osal_memset(zapp_StoreQueueItems, 0x00, sizeof(zapp_StoreQueueItems[0]) * ZAPP_STOREQUEUE_LEN);
  osal_memset(zapp_StoreQueueSizes, 0x00, sizeof(zapp_StoreQueueSizes[0]) * ZAPP_STOREQUEUE_LEN);
  osal_memset(zapp_StoreQueueSources, 0x00, sizeof(zapp_StoreQueueSources[0]) * ZAPP_STOREQUEUE_LEN);
}

/****************************************************************************
****************************************************************************/


