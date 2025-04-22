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

#define WC_DEVICE_VERSION     1
#define WC_FLAGS              0

#define WC_HWVERSION          0
#define WC_ZCLVERSION         0

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
#define WC_NV_DESC1             0x0404
#define WC_NV_DESC2             0x0405
#define WC_NV_UNIT1             0x0406
#define WC_NV_UNIT2             0x0407
#define WC_NV_MULTIPLIER1       0x0408
#define WC_NV_MULTIPLIER2       0x0409
#define WC_NV_DIVISOR1          0x040A
#define WC_NV_DIVISOR2          0x040B
#define WC_NV_VOLUMEREPORT1     0x040C
#define WC_NV_VOLUMEREPORT2     0x040D
#define WC_NV_REPORTPERIOD      0x040E
#define WC_NV_VOLTAGERATED      0x040F
#define WC_NV_VALUES1           0x0400
#define WC_NV_VALUES2           0x0411
#define WC_NV_DATES             0x0412

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
const uint16 zclWC_clusterRevision_all = 0x0001; //currently all cluster implementations are according to ZCL6, which has revision #1. In the future it is possible that different clusters will have different revisions, so they will have to use separate attribute variables.

// Basic Cluster
const uint8 zclWC_HWRevision = WC_HWVERSION;
const uint8 zclWC_ZCLVersion = WC_ZCLVERSION;
const uint8 zclWC_ManufacturerName[] = { 15, 'F','o','x','.','I','n','s','t','r','u','m','e','n','t','s' };
const uint8 zclWC_ModelId[] = { 12, 'F','O','X','-','M','e','t','e','r','0','0','1',};
const uint8 zclWC_DateCode[] = { 10, '2','0','2','4','-','0','2','-','0','2'};

const uint8 zclWC_DeviceType = 0x02;    // Water meter
const uint8 zclWC_PowerSource = POWER_SOURCE_BATTERY;

const uint8 zclWC_cDesc1[WC_METER_SITEID_SIZE + 1] = {WC_METER_SITEID_SIZE, 'C', 'o', 'l', 'd', ' ', ' ', ' ', ' ', ' ', ' '};   // Constants to initialize default values
const uint8 zclWC_cDesc2[WC_METER_SITEID_SIZE + 1] = {WC_METER_SITEID_SIZE, 'H', 'o', 't', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

uint8 zclWC_Flow1Desc[WC_METER_SITEID_SIZE + 1];
uint48_t zclWC_Flow1Value;
int24 zclWC_Flow1InstDemand;
int24 zclWC_Flow1InstDemandPrev;
uint24 zclWC_Flow1Multiplier;
uint24 zclWC_Flow1Divisor;
uint8 zclWC_Flow1Unit;
uint16 zclWC_Flow1VolumePerReport;
uint24 zclWC_Flow1CurrDay;
uint24 zclWC_Flow1PrevDay;
uint8 zclWC_Flow1Status;

uint8 zclWC_Flow2Desc[WC_METER_SITEID_SIZE + 1];
uint48_t zclWC_Flow2Value;
int24 zclWC_Flow2InstDemand;
int24 zclWC_Flow2InstDemandPrev;
uint24 zclWC_Flow2Multiplier;
uint24 zclWC_Flow2Divisor;
uint16 zclWC_Flow2Unit;
uint16 zclWC_Flow2VolumePerReport;
uint24 zclWC_Flow2CurrDay;
uint24 zclWC_Flow2PrevDay;
uint8 zclWC_Flow2Status;

// Power cluster variable
uint8 zclWC_BatteryVoltage;            // 0.1V unit
uint8 zclWC_BatteryVoltageRated;        // 0.1V unit, 3.6V LiFePO4
uint8 zclWC_BatteryVoltageThresMin;    // 0.1V unit, 2.5V LiFePO4 T>0 degC
uint8 zclWC_BatteryVoltageThres1;
uint8 zclWC_BatteryLevel;              // 0 - 100%
uint8 zclWC_BatteryAlarmMask;
uint32 zclWC_BatteryAlarmState;

// Time cluster variables
/* uint32 zclWC_Time;
uint8 zclWC_TimeStatus;
uint32 zclWC_TimeLocal; */

uint8 zclWC_FlowUpdatePeriod;   // InstDemand update time period in seconds
uint16 zclWC_FlowReportInterval; // Time interval in minutes
uint24 zclWC_Flow1HoursInOperation;
uint24 zclWC_Flow2HoursInOperation;

uint8 zclWC_LocationDescription[16];
uint8 zclWC_PhysicalEnvironment;
uint8 zclWC_DeviceEnable = DEVICE_ENABLED;

// Diagnostic cluster
uint16 zclWC_DiagNumOfResets;
uint16 zclWC_DiagNVMemWrites;
uint16 zclWC_DiagNVMemWriteFails;
uint16 zclWC_DiagMemAllocatedBlocks;
uint16 zclWC_DiagMemFreeBlocks;
uint16 zclWC_DiagMemUsed;       // Used memory in bytes
uint16 zclWC_DiagMemHighWater;  // Maximum memory allocated
uint16 zclWC_DiagRebootReason;

uint16 zclWC_NVItemsInitStatus;
uint16 zclWC_NVItemsToWrite;       // Bitmap of Items to write into NV memory

// Identify Cluster
uint16 zclWC_IdentifyTime = 0;
// Constants set Attribute value limitations
const uint16 zclWC_cReportIntervals[] = {5, 10, 15, 20, 30, 60, 2*60, 3*60, 4*60, 6*60, 8*60, 12*60, 24*60};
CONST uint8 zclWC_cReportIntervalsSize_1 = sizeof(zclWC_cReportIntervals) / sizeof(zclWC_cReportIntervals[0]) - 1;
const uint8 zclWC_cRatedVoltages[] = {VDD_VOLTAGE_RATED30, VDD_VOLTAGE_RATED33, VDD_VOLTAGE_RATED36, VDD_VOLTAGE_RATED42};
CONST uint8 zclWC_cRatedVoltagesSize_1 = sizeof(zclWC_cRatedVoltages) / sizeof(zclWC_cRatedVoltages[0]) - 1;
const uint8 zclWC_cRatedVoltageThres[] = {28, 28, VDD_VOLTAGE36_THRES1, 30};
const uint8 zclWC_cRatedVoltageThresMin[] = {27, 27, VDD_VOLTAGE36_MIN, 26};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
 
  // NOTE: The attributes listed in the AttrRec must be in ascending order 
  // per cluster to allow right function of the Foundation discovery commands
 
CONST zclAttrRec_t zclWC_cAttrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //1
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclWC_ZCLVersion
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  //2
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclWC_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //3
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWC_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //4
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWC_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //5
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWC_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //6
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8, ACCESS_CONTROL_READ,
      (void *)&zclWC_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //7
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclWC_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //8
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //9
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_DeviceEnable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { //10
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zclWC_clusterRevision_all
    }
  },
  
  // ***** Battery status cluster ***** //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //11
      ATTRID_POWER_CFG_BATTERY_VOLTAGE,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
      (void *)&zclWC_BatteryVoltage
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //12
      ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_AUTH_READ | ACCESS_REPORTABLE,
      (void *)&zclWC_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //13
      ATTRID_POWER_CFG_BAT_RATED_VOLTAGE,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&zclWC_BatteryVoltageRated
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //14
      ATTRID_POWER_CFG_BAT_ALARM_MASK,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&zclWC_BatteryAlarmMask
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //15
      ATTRID_POWER_CFG_BAT_VOLT_MIN_THRES,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclWC_BatteryVoltageThresMin
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //16
      ATTRID_POWER_CFG_BAT_VOLT_THRES_1,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclWC_BatteryVoltageThres1
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //17
      ATTRID_POWER_CFG_BAT_ALARM_STATE,
      ZCL_DATATYPE_BITMAP32, ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
      (void *)&zclWC_BatteryAlarmState
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    { //18
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Identify Cluster Attribute ********* //
  // ********************************************** //
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { //19
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_IdentifyTime
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  //20
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_GLOBAL),
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Groups Cluster ********* //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_GROUPS,
    { //21
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Time Cluster ********* //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    { //22
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Flow measure Cluster ********* //
  // **************************************** //
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //23
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow1Value
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //24
      ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowUpdatePeriod
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //25
      ATTRID_METER_0READINGSET_INTERVALREPORTING,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowReportInterval
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //26
      ATTRID_METER_0READINGSET_VOLUMEPERREPORT,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1VolumePerReport
    }
  },
    {
    ZCL_CLUSTER_ID_SE_METERING,
    { //27
      ATTRID_METER_2STATUS_STATUS,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow1Status
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //28
      ATTRID_METER_2STATUS_REMAININGBATT,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //29
      ATTRID_METER_2STATUS_HOURSINOPERATION,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow1HoursInOperation
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //30
      ATTRID_METER_3FORMATTING_UNIT,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Unit
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //31
      ATTRID_METER_3FORMATTING_MULTIPLIER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Multiplier
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //32
      ATTRID_METER_3FORMATTING_DIVISOR,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Divisor
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //33
      ATTRID_METER_3FORMATTING_METERDEVICETYPE,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DeviceType
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //34
      ATTRID_METER_3FORMATTING_SITEID,
      ZCL_DATATYPE_OCTET_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Desc
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //35
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24, (ACCESS_CONTROL_READ | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow1InstDemand
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //36
      ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow1CurrDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //37
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow1PrevDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //38
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //39
      ATTRID_DIAG_0NUMOFRESETS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DiagNumOfResets
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //40
      ATTRID_DIAG_1PERSISTMEMORYWRITES,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DiagNVMemWrites
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //41
      ATTRID_DIAG_2PERSISTMEMORYWRITEFAILS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DiagNVMemWriteFails
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //42
      ATTRID_DIAG_3MEMALLOCATEDBLOCKS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_AUTH_READ),
      (void *)&zclWC_DiagMemAllocatedBlocks
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //43
      ATTRID_DIAG_4MEMFREEBLOCKS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_AUTH_READ),
      (void *)&zclWC_DiagMemFreeBlocks
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //44
      ATTRID_DIAG_5MEMUSED,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_AUTH_READ),
      (void *)&zclWC_DiagMemUsed
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //45
      ATTRID_DIAG_6MEMHIGHWATER,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_AUTH_READ),
      (void *)&zclWC_DiagMemHighWater
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //46
      ATTRID_DIAG_7REBOOTREASON,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DiagRebootReason
    }
  },
  {
    ZCL_CLUSTER_ID_HA_DIAGNOSTIC,
    { //
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  }
};

CONST uint8 zclWC_cNumAttributes = ( sizeof(zclWC_cAttrs) / sizeof(zclWC_cAttrs[0]) );

CONST zclAttrRec_t zclWC_cAttrs2[] =
{
  // ********* Flow measure Cluster ********* //
  // **************************************** //
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //1
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow2Value
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //2
      ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowUpdatePeriod
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //3
      ATTRID_METER_0READINGSET_INTERVALREPORTING,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowReportInterval
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //4
      ATTRID_METER_0READINGSET_VOLUMEPERREPORT,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2VolumePerReport
    }
  },
    {
    ZCL_CLUSTER_ID_SE_METERING,
    { //5
      ATTRID_METER_2STATUS_STATUS,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow2Status
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //6
      ATTRID_METER_2STATUS_REMAININGBATT,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //7
      ATTRID_METER_2STATUS_HOURSINOPERATION,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow2HoursInOperation
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //8
      ATTRID_METER_3FORMATTING_UNIT,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Unit
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //9
      ATTRID_METER_3FORMATTING_MULTIPLIER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Multiplier
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //10
      ATTRID_METER_3FORMATTING_DIVISOR,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Divisor
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //11
      ATTRID_METER_3FORMATTING_METERDEVICETYPE,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DeviceType
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //12
      ATTRID_METER_3FORMATTING_SITEID,
      ZCL_DATATYPE_OCTET_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Desc
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //13
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24, (ACCESS_CONTROL_READ | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow2InstDemand
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //14
      ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow2CurrDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //15
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow2PrevDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { //16
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  }
};

CONST uint8 zclWC_cNumAttributes2 = ( sizeof(zclWC_cAttrs2) / sizeof(zclWC_cAttrs2[0]) );

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t zclWC_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_POWER_CFG,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_SE_METERING
};

#define ZCLWC_MAX_INCLUSTERS    ( sizeof( zclWC_InClusterList ) / sizeof( zclWC_InClusterList[0] ))

const cId_t zclWC_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_GROUPS
};

#define ZCLWC_MAX_OUTCLUSTERS   ( sizeof( zclWC_OutClusterList ) / sizeof( zclWC_OutClusterList[0] ))

SimpleDescriptionFormat_t zclWC_SimpleDesc =
{
  WC_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                    //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_METER_INTERFACE,      //  uint16 AppDeviceId[2];
  WC_DEVICE_VERSION,            //  int   AppDevVer:4;
  WC_FLAGS,                     //  int   AppFlags:4;
  ZCLWC_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zclWC_InClusterList, //  byte *pAppInClusterList;
  ZCLWC_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zclWC_OutClusterList //  byte *pAppInClusterList;
};

const cId_t zclWC_InClusterList2[] =
{
  ZCL_CLUSTER_ID_SE_METERING
};

#define ZCLWC_MAX_INCLUSTERS2    ( sizeof( zclWC_InClusterList2 ) / sizeof( zclWC_InClusterList2[0] ))

SimpleDescriptionFormat_t zclWC_SimpleDesc2 =
{
  WC_ENDPOINT2,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                    //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_METER_INTERFACE,      //  uint16 AppDeviceId[2];
  WC_DEVICE_VERSION,            //  int   AppDevVer:4;
  WC_FLAGS,                     //  int   AppFlags:4;
  ZCLWC_MAX_INCLUSTERS2,         //  byte  AppNumInClusters;
  (cId_t *)zclWC_InClusterList2, //  byte *pAppInClusterList;
  0,                            //  byte  AppNumInClusters;
  NULL                          //  byte *pAppInClusterList;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zclWC_InitNVItems
 *
 * @brief   Initialize items in NV for Attribute values.
 *
 * @param   none
 *
 * @return  SUCCESS, FAILURE
 */
void zclWC_NVInitItems(void)
{
  zclWC_NVItemsInitStatus = 0;
  
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_DESC1, WC_METER_SITEID_SIZE, NULL) == NV_OPER_FAILED ? 1 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_DESC2, WC_METER_SITEID_SIZE, NULL) == NV_OPER_FAILED ? 1<<1 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_UNIT1, sizeof(zclWC_Flow1Unit), NULL) == NV_OPER_FAILED ? 1<<2 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_UNIT2, sizeof(zclWC_Flow2Unit), NULL) == NV_OPER_FAILED ? 1<<3 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_MULTIPLIER1, sizeof(zclWC_Flow1Multiplier), NULL) == NV_OPER_FAILED ? 1<<4 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_MULTIPLIER2, sizeof(zclWC_Flow2Multiplier), NULL) == NV_OPER_FAILED ? 1<<5 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_DIVISOR1, sizeof(zclWC_Flow1Divisor), NULL) == NV_OPER_FAILED ? 1<<6 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_DIVISOR2, sizeof(zclWC_Flow2Divisor), NULL) == NV_OPER_FAILED ? 1<<7 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_VOLUMEREPORT1, sizeof(zclWC_Flow1VolumePerReport), NULL) == NV_OPER_FAILED ? 1<<8 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_VOLUMEREPORT2, sizeof(zclWC_Flow2VolumePerReport), NULL) == NV_OPER_FAILED ? 1<<9 : 0;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_REPORTPERIOD, sizeof(zclWC_FlowReportInterval), NULL) == NV_OPER_FAILED ? 1<<10 : 0;  
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_VALUES1, sizeof(zclWC_Flow1Value.dw.lowDW), NULL) == NV_OPER_FAILED ? 1<<11 : 0;; 
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_VALUES2, sizeof(zclWC_Flow2Value.dw.lowDW), NULL) == NV_OPER_FAILED ? 1<<12 : 0;;
  zclWC_NVItemsInitStatus |= osal_nv_item_init(WC_NV_DATES, sizeof(uint32), NULL) == NV_OPER_FAILED ? 1<<13 : 0;;
}

/*********************************************************************
 * @fn      zclWC_CheckNVItem
 *
 * @brief   Check NV item whether it was stored in NV memory.
 *
 * @param   id - NV item id number
 *
 * @return  SUCCESS, FAILURE
 */
uint8 zclWC_NVCheckItem(uint16 id, uint16 len)
{
  uint32 buf;
  
  if (len > 4)
    len = 4;
  
  if ((zclWC_NVItemsInitStatus & (1 << (id - WC_NV_DESC1))) == 0)
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
 * @fn      zclWC_InitAttrValue
 *
 * @brief   Init attribute from NV stored item or Default value.
 *
 * @param   none
 *
 * @return  none
 */
void zclWC_InitAttrValue(uint16 id, uint16 len, const void *src, void *buf)
{
  if (zclWC_NVCheckItem(id, len) == SUCCESS)
  {
    if (osal_nv_read(id, 0, len, buf) == SUCCESS)
      return;
  }
  
  osal_memcpy(buf, src, len);
}

/*********************************************************************
 * @fn      zclWC_UpdateAttrIntervalReporting
 *
 * @brief   Update value according to array of valid values
 *
 * @param   checked value
 *
 * @return  valid value
 */
void zclWC_UpdateAttrIntervalReporting(uint16 *data)
{
  uint8 l, r, m;

  l = 0;
  r = zclWC_cReportIntervalsSize_1;
  m = 5;
  while (l <= r)
  {
    m = l + (r - l) / 2;
    if (*data < zclWC_cReportIntervals[m] - (zclWC_cReportIntervals[m] - zclWC_cReportIntervals[m - 1])/2)
    {
      if (m == 1) { m = 0; break; }
      r = m - 1;
    }
    else if (*data > zclWC_cReportIntervals[m] + (zclWC_cReportIntervals[m + 1] - zclWC_cReportIntervals[m])/2)
    {
      if (m == zclWC_cReportIntervalsSize_1 - 1) { m = zclWC_cReportIntervalsSize_1; break; }
      l = m + 1;
    }
    else
      break;
  }
  *data = zclWC_cReportIntervals[m];
}

/*********************************************************************
 * @fn      zclWC_UpdateAttrRatedVaoltage
 *
 * @brief   Update value according to array of valid values
 *
 * @param   checked value
 *
 * @return  valid value
 */
void zclWC_UpdateAttrRatedVoltage(uint8 *data)
{
  uint8 i;
  
  for (i = 0; i <= zclWC_cRatedVoltagesSize_1; i++)
  {
    if (*data <= zclWC_cRatedVoltages[i])
    {
      *data = zclWC_cRatedVoltages[i];
      zclWC_BatteryVoltageThres1 = zclWC_cRatedVoltageThres[i];
      zclWC_BatteryVoltageThresMin = zclWC_cRatedVoltageThresMin[i];
      break;
    }
  }
  if (i > zclWC_cRatedVoltagesSize_1)
  {
    *data = zclWC_cRatedVoltages[zclWC_cRatedVoltagesSize_1];
    zclWC_BatteryVoltageThres1 = zclWC_cRatedVoltageThres[zclWC_cRatedVoltagesSize_1];
    zclWC_BatteryVoltageThresMin = zclWC_cRatedVoltageThresMin[zclWC_cRatedVoltagesSize_1];
  }
}
/*********************************************************************
 * @fn      zclWC_ResetAttributesToDefaultValues
 *
 * @brief   Reset all writable attributes to their default values.
 *
 * @param   none
 *
 * @return  none
 */
void zclWC_ResetAttributesToDefaultValues(void)
{
  int i;
  uint32 src;
 // uint8 result;
  
  zclWC_LocationDescription[0] = 15;
  for (i = 1; i <= 15; i++)
  {
    zclWC_LocationDescription[i] = ' ';
  }
  
  zclWC_PhysicalEnvironment = DEFAULT_PHYSICAL_ENVIRONMENT;
  zclWC_DeviceEnable = DEFAULT_DEVICE_ENABLE_STATE;
  
  zclWC_IdentifyTime = DEFAULT_IDENTIFY_TIME;
  
  //osal_memcpy(zclWC_Flow1Desc, zclWC_cDesc1, WC_METER_SITEID_SIZE + 1);
  //osal_memcpy(zclWC_Flow2Desc, zclWC_cDesc2, WC_METER_SITEID_SIZE + 1);
  
  zclWC_InitAttrValue(WC_NV_DESC1, WC_METER_SITEID_SIZE, zclWC_cDesc1, zclWC_Flow1Desc);
  zclWC_InitAttrValue(WC_NV_DESC2, WC_METER_SITEID_SIZE, zclWC_cDesc2, zclWC_Flow2Desc);
  
  src = 0;
  zclWC_InitAttrValue(WC_NV_VALUES1, sizeof(zclWC_Flow1Value.dw.lowDW), &src, &zclWC_Flow1Value.dw.lowDW);
  zclWC_InitAttrValue(WC_NV_VALUES2, sizeof(zclWC_Flow1Value.dw.lowDW), &src, &zclWC_Flow2Value.dw.lowDW);
  
  //zclWC_Flow1Value.dw.lowDW = 0;
  zclWC_Flow1Value.dw.hiW = 0;
  //zclWC_Flow2Value.dw.lowDW = 0;
  zclWC_Flow2Value.dw.hiW = 0;
  zclWC_Flow1InstDemand = 0;
  zclWC_Flow2InstDemand = 0;
  zclWC_Flow1InstDemandPrev = 0;
  zclWC_Flow2InstDemandPrev = 0;
  zclWC_Flow1PrevDay = 0;
  zclWC_Flow2PrevDay = 0;
  zclWC_Flow1CurrDay = 0;
  zclWC_Flow2CurrDay = 0;
  zclWC_Flow1HoursInOperation = 0;
  zclWC_Flow2HoursInOperation = 0;
  
  src = WC_METER_MULTIPLIER;
  zclWC_InitAttrValue(WC_NV_MULTIPLIER1, sizeof(zclWC_Flow1Multiplier), &src, &zclWC_Flow1Multiplier);
  zclWC_InitAttrValue(WC_NV_MULTIPLIER2, sizeof(zclWC_Flow2Multiplier), &src, &zclWC_Flow2Multiplier);
  //zclWC_Flow1Multiplier = WC_METER_MULTIPLIER;
  //zclWC_Flow2Multiplier = WC_METER_MULTIPLIER;
  src = WC_METER_DIVISOR;
  zclWC_InitAttrValue(WC_NV_DIVISOR1, sizeof(zclWC_Flow1Divisor), &src, &zclWC_Flow1Divisor);
  zclWC_InitAttrValue(WC_NV_DIVISOR2, sizeof(zclWC_Flow2Divisor), &src, &zclWC_Flow2Divisor);
  //zclWC_Flow1Divisor = WC_METER_DIVISOR;
  //zclWC_Flow2Divisor = WC_METER_DIVISOR;
  src = ENGINEERING_UNIT_METER_M3;
  zclWC_InitAttrValue(WC_NV_UNIT1, sizeof(zclWC_Flow1Unit), &src, &zclWC_Flow1Unit);
  zclWC_InitAttrValue(WC_NV_UNIT2, sizeof(zclWC_Flow2Unit), &src, &zclWC_Flow2Unit);
  //zclWC_Flow1Unit = ENGINEERING_UNIT_METER_M3;
  //zclWC_Flow2Unit = ENGINEERING_UNIT_METER_M3;
  src = WC_VOLUME_PER_REPORT;
  zclWC_InitAttrValue(WC_NV_VOLUMEREPORT1, sizeof(zclWC_Flow1VolumePerReport), &src, &zclWC_Flow1VolumePerReport);
  zclWC_InitAttrValue(WC_NV_VOLUMEREPORT2, sizeof(zclWC_Flow2VolumePerReport), &src, &zclWC_Flow2VolumePerReport);
  //zclWC_Flow1VolumePerReport = WC_VOLUME_PER_REPORT;
  //zclWC_Flow2VolumePerReport = WC_VOLUME_PER_REPORT;
  zclWC_Flow1Status = 0;
  zclWC_Flow2Status = 0;
  
  zclWC_FlowUpdatePeriod = WC_METER_INSTDEMAND_UPDATEPERIOD_MAX;   // InstDemand update interval
  src = WC_METER_REPORT_INTERVAL;
  zclWC_InitAttrValue(WC_NV_REPORTPERIOD, sizeof(zclWC_FlowReportInterval), &src, &zclWC_FlowReportInterval);
  //zclWC_FlowReportInterval = WC_METER_REPORT_INTERVAL; // Report interal in minutes
  
  //zclWC_BatteryVoltageRated = VDD_VOLTAGE_RATED36;
  src = VDD_VOLTAGE_RATED36;
  zclWC_InitAttrValue(WC_NV_VOLTAGERATED, sizeof(zclWC_BatteryVoltageRated), &src, &zclWC_BatteryVoltageRated);
  zclWC_BatteryVoltageThresMin = VDD_VOLTAGE36_MIN;
  zclWC_BatteryVoltageThres1 = VDD_VOLTAGE36_THRES1;
  zclWC_BatteryAlarmMask = BAT_ALARM_MASK_VOLT_2_LOW | BAT_ALARM_MASK_BATTERY_ALARM_1;
  zclWC_BatteryAlarmState = 0;
  
  zclWC_DiagNumOfResets = 0;
  zclWC_DiagNVMemWrites = 0;
  zclWC_DiagNVMemWriteFails = 0;
  zclWC_DiagMemAllocatedBlocks = osal_heap_block_cnt();
  zclWC_DiagMemFreeBlocks = osal_heap_block_free();
  zclWC_DiagMemUsed = osal_heap_mem_used();
  zclWC_DiagMemHighWater = osal_heap_high_water();
  //zclWC_DiagRebootReason = 0;  
}

/****************************************************************************
****************************************************************************/


