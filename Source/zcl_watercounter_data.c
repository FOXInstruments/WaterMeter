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
#include "zcl_diagnostic.h"
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

#define ENGINEERING_UNIT_VOLUME_L      0x87
#define ENGINEERING_UNIT_VOLUME_M3     0x81
#define ENGINEERING_UNIT_TIME_SEC      73
#define ENGINEERING_UNIT_TIME_MIN      72
#define ENGINEERING_UNIT_TIME_HOUR     71
#define ENGINEERING_UNIT_TIME_DAY      70

// 0x0401 - 0x0FFF application NV ids
#define WC_NV_ITEM_DESC1        0x0404
#define WC_NV_ITEM_DESC2        WC_NV_ITEM_DESC1 + WC_DESCSIZE
#define WC_NV_ITEM_BLOCK1       WC_NV_ITEM_DESC2 + 4
#define WC_NV_ITEM_BLOCK2       WC_NV_ITEM_BLOCK1 + 4
#define WC_NV_ITEM_REPORT       WC_NV_ITEM_BLOCK2 + 4
#define WC_NV_ITEM_VALUE1       WC_NV_ITEM_REPORT + 4
#define WC_NV_ITEM_VALUE2       WC_NV_ITEM_VALUE1 + sizeof(zclWC_Flow1Value.dw.lowDW)
#define WC_NV_ITEM_VALUEX       WC_NV_ITEM_VALUE2 + sizeof(zclWC_Flow2Value.dw.lowDW)

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
const uint8 zclWC_ModelId[] = { 15, 'F','O','X','0','0','1',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclWC_DateCode[] = { 15, '2','0','2','4','0','2','0','2',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclWC_PowerSource = POWER_SOURCE_BATTERY;

const uint8 zclWC_Desc1[WC_DESCSIZE] = {WC_DESCSIZE - 1, 'C', 'o', 'l', 'd', ' ', ' ', ' ' };   // Constants to initialize default values
const uint8 zclWC_Desc2[WC_DESCSIZE] = {WC_DESCSIZE - 1, 'H', 'o', 't', ' ', ' ', ' ', ' ' };

const uint8 zclWC_DeviceType = 0x02;    // Water meter

uint8 zclWC_Flow1Desc[WC_DESCSIZE];
uint48_t zclWC_Flow1Value;
int24 zclWC_Flow1InstDemand;
int24 zclWC_Flow1InstDemandPrev;
uint16 zclWC_Flow1Multiplier;
uint16 zclWC_Flow1Divisor;
uint8 zclWC_Flow1Unit;
uint16 zclWC_Flow1VolumePerReport;
uint16 zclWC_Flow1PrevDay;
uint8 zclWC_Flow1Status;

uint8 zclWC_Flow2Desc[WC_DESCSIZE];
uint48_t zclWC_Flow2Value;
int24 zclWC_Flow2InstDemand;
int24 zclWC_Flow2InstDemandPrev;
uint16 zclWC_Flow2Multiplier;
uint16 zclWC_Flow2Divisor;
uint16 zclWC_Flow2Unit;
uint16 zclWC_Flow2VolumePerReport;
uint16 zclWC_Flow2PrevDay;
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

uint8 zclWC_NVItemsInitStatus;

// Identify Cluster
uint16 zclWC_IdentifyTime = 0;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
 
  // NOTE: The attributes listed in the AttrRec must be in ascending order 
  // per cluster to allow right function of the Foundation discovery commands
 
CONST zclAttrRec_t zclWC_Attrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclWC_ZCLVersion
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclWC_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWC_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWC_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWC_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8, ACCESS_CONTROL_READ,
      (void *)&zclWC_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclWC_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_DeviceEnable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zclWC_clusterRevision_all
    }
  },
  
  // ***** Battery status cluster ***** //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_POWER_CFG_BATTERY_VOLTAGE,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
      (void *)&zclWC_BatteryVoltage
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
      (void *)&zclWC_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_POWER_CFG_BAT_RATED_VOLTAGE,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&zclWC_BatteryVoltageRated
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_POWER_CFG_BAT_ALARM_MASK,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
      (void *)&zclWC_BatteryAlarmMask
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_POWER_CFG_BAT_VOLT_MIN_THRES,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclWC_BatteryVoltageThresMin
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_POWER_CFG_BAT_VOLT_THRES_1,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclWC_BatteryVoltageThres1
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_POWER_CFG_BAT_ALARM_STATE,
      ZCL_DATATYPE_BITMAP32, ACCESS_CONTROL_READ | ACCESS_REPORTABLE,
      (void *)&zclWC_BatteryAlarmState
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_POWER_CFG,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Identify Cluster Attribute ********* //
  // ********************************************** //
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_IdentifyTime
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_GLOBAL),
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Groups Cluster ********* //
  // ********************************** //
  {
    ZCL_CLUSTER_ID_GEN_GROUPS,
    {  
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Time Cluster ********* //
  // ********************************** //
/*  {
    ZCL_CLUSTER_ID_GEN_TIME,
    {  
      ATTRID_TIME_TIME,
      ZCL_DATATYPE_UTC, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_Time
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    {  
      ATTRID_TIME_TIME_STATUS,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_TimeStatus
    }
  },
   {
    ZCL_CLUSTER_ID_GEN_TIME,
    {  
      ATTRID_TIME_LOCAL_TIME,
      ZCL_DATATYPE_UINT32, (ACCESS_CONTROL_READ),
      (void *)&zclWC_TimeLocal
    }
  },*/
  {
    ZCL_CLUSTER_ID_GEN_TIME,
    {  
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  },
  // ********* Flow measure Cluster ********* //
  // **************************************** //
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow1Value
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowUpdatePeriod
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_INTERVALREPORTING,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowReportInterval
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_VOLUMEPERREPORT,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1VolumePerReport
    }
  },
    {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_2STATUS_STATUS,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow1Status
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_2STATUS_REMAININGBATT,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_2STATUS_HOURSINOPERATION,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow1HoursInOperation
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_UNIT,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Unit
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_MULTIPLIER,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Multiplier
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_DIVISOR,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Divisor
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_METERDEVICETYPE,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DeviceType
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_SITEID,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow1Desc
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24, (ACCESS_CONTROL_READ | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow1InstDemand
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow1PrevDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    {  
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  }
};

uint8 CONST zclWC_NumAttributes = ( sizeof(zclWC_Attrs) / sizeof(zclWC_Attrs[0]) );

CONST zclAttrRec_t zclWC_Attrs2[] =
{
  // ********* Flow measure Cluster ********* //
  // **************************************** //
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_CURRSUMDELIVERED,
      ZCL_DATATYPE_UINT48, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow2Value
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowUpdatePeriod
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_INTERVALREPORTING,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_FlowReportInterval
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_0READINGSET_VOLUMEPERREPORT,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2VolumePerReport
    }
  },
    {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_2STATUS_STATUS,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow2Status
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_2STATUS_REMAININGBATT,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_BatteryLevel
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_2STATUS_HOURSINOPERATION,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow2HoursInOperation
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_UNIT,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Unit
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_MULTIPLIER,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Multiplier
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_DIVISOR,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Divisor
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_METERDEVICETYPE,
      ZCL_DATATYPE_BITMAP8, (ACCESS_CONTROL_READ),
      (void *)&zclWC_DeviceType
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_3FORMATTING_SITEID,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWC_Flow2Desc
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_4HISTORY_INSTANTDEMAND,
      ZCL_DATATYPE_INT24, (ACCESS_CONTROL_READ | ACCESS_REPORTABLE),
      (void *)&zclWC_Flow2InstDemand
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    { 
      ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER,
      ZCL_DATATYPE_UINT24, (ACCESS_CONTROL_READ),
      (void *)&zclWC_Flow2PrevDay
    }
  },
  {
    ZCL_CLUSTER_ID_SE_METERING,
    {  
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWC_clusterRevision_all
    }
  }
};

uint8 CONST zclWC_NumAttributes2 = ( sizeof(zclWC_Attrs2) / sizeof(zclWC_Attrs2[0]) );

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
  uint8 result = 0;
  
  result |= osal_nv_item_init(WC_NV_ITEM_DESC1, WC_DESCSIZE, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_DESC2, WC_DESCSIZE, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_VALUE1, sizeof(zclWC_Flow1Value.dw.lowDW), NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_VALUE2, sizeof(zclWC_Flow2Value.dw.lowDW), NULL);
/*  result |= osal_nv_item_init(WC_NV_ITEM_BLOCK1, 4, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_BLOCK2, 4, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_REPORT, 4, NULL); */
  
  if (result == SUCCESS) { // SUCCESS only if all init calls were SUCCESS
    zclWC_NVItemsInitStatus = SUCCESS;
  } else {
    zclWC_NVItemsInitStatus = FAILURE;
  }
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
uint8 zclWC_NVItemCheck(uint16 id)
{
  uint32 buf;
  
  if (zclWC_NVItemsInitStatus == SUCCESS)
  {
    if (osal_nv_read(id, 0, 4, &buf) == SUCCESS)
    {
      if (buf != 0xFFFFFFFF)
      {
        return SUCCESS;
      }
    }
  }
  return FAILURE;
}

/*********************************************************************
 * @fn      zclWC_InitAttribute
 *
 * @brief   Init attribute from NV stored item or Default value.
 *
 * @param   none
 *
 * @return  none
 */
void zclWC_InitAttribute(uint16 id, uint16 len, const void *src, void *buf)
{
  uint8 result;
  
  result = zclWC_NVItemCheck(id);
  if (result == SUCCESS)
  {
      result |= osal_nv_read(id, 0, len, buf);
  }
  
  if (result != SUCCESS)
  {
    osal_memcpy(buf, src, len);
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
 // uint32 src, dst;
 // uint8 result;
  
  zclWC_LocationDescription[0] = 15;
  for (i = 1; i <= 15; i++)
  {
    zclWC_LocationDescription[i] = ' ';
  }
  
  zclWC_PhysicalEnvironment = DEFAULT_PHYSICAL_ENVIRONMENT;
  zclWC_DeviceEnable = DEFAULT_DEVICE_ENABLE_STATE;
  
  zclWC_IdentifyTime = DEFAULT_IDENTIFY_TIME;
  
  osal_memcpy(zclWC_Flow1Desc, zclWC_Desc1, WC_DESCSIZE);
  osal_memcpy(zclWC_Flow2Desc, zclWC_Desc2, WC_DESCSIZE);
  
  /*zclWC_InitAttribute(WC_NV_ITEM_DESC1, WC_DESCSIZE, zclWC_Desc1, zclWC_Flow1Desc);
  zclWC_InitAttribute(WC_NV_ITEM_DESC2, WC_DESCSIZE, zclWC_Desc2, zclWC_Flow2Desc);
  
  src = WC_MULTIPLYER;
  src = (src << 16) | ENGINEERING_UNIT_VOLUME_L;
  zclWC_InitAttribute(WC_NV_ITEM_BLOCK1, 4, &src, &dst);
  zclWC_Flow1Multiplier = (dst >> 16) & 0xFFFF;
  zclWC_Flow1Unit = dst & 0xFFFF;
    
  zclWC_InitAttribute(WC_NV_ITEM_BLOCK2, 4, &src, &dst);
  zclWC_Flow2Multiplier = (dst >> 16) & 0xFFFF;
  zclWC_Flow2Unit = dst & 0xFFFF;

  src = WC_REPORT_INTERVAL;
  zclWC_InitAttribute(WC_NV_ITEM_REPORT, 4, &src, &zclWC_FlowReportInterval);

  src = 0;
  zclWC_InitAttribute(WC_NV_ITEM_VALUE1, 4, &src, &zclWC_Flow1Value);
  zclWC_InitAttribute(WC_NV_ITEM_VALUE2, 4, &src, &zclWC_Flow2Value);*/
  
  zclWC_Flow1Value.dw.lowDW = 0;
  zclWC_Flow1Value.dw.hiW = 0;
  zclWC_Flow2Value.dw.lowDW = 0;
  zclWC_Flow2Value.dw.hiW = 0;
  zclWC_Flow1InstDemand = 0;
  zclWC_Flow2InstDemand = 0;
  zclWC_Flow1InstDemandPrev = 0;
  zclWC_Flow2InstDemandPrev = 0;
  zclWC_Flow1PrevDay = 0;
  zclWC_Flow2PrevDay = 0;
  
  zclWC_Flow1Multiplier = 10;
  zclWC_Flow2Multiplier = 10;
  zclWC_Flow1Divisor = 1000;
  zclWC_Flow2Divisor = 1000;
  zclWC_Flow1HoursInOperation = 0;
  zclWC_Flow2HoursInOperation = 0;
  zclWC_Flow1Unit = ENGINEERING_UNIT_VOLUME_M3;
  zclWC_Flow2Unit = ENGINEERING_UNIT_VOLUME_M3;
  zclWC_Flow1VolumePerReport = WC_REPORT_CHANGE_FLOW;
  zclWC_Flow2VolumePerReport = WC_REPORT_CHANGE_FLOW;
  zclWC_Flow1Status = 0;
  zclWC_Flow2Status = 0;
  
  zclWC_FlowUpdatePeriod = 60;   // InstDemand update interval
  zclWC_FlowReportInterval = WC_REPORT_INTERVAL; // Report interal in minutes
  
  zclWC_BatteryVoltageRated = VDD_VOLTAGE_RATED;
  zclWC_BatteryVoltageThresMin = VDD_VOLTAGE_MIN;
  zclWC_BatteryVoltageThres1 = VDD_VOLTAGE_THRES1;
  zclWC_BatteryAlarmMask = BAT_ALARM_MASK_VOLT_2_LOW | BAT_ALARM_MASK_BATTERY_ALARM_1;
  zclWC_BatteryAlarmState = 0;
}

/****************************************************************************
****************************************************************************/


