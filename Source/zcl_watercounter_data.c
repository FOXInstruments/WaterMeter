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

#define WATERCOUNTER_DEVICE_VERSION     1
#define WATERCOUNTER_FLAGS              0

#define WATERCOUNTER_HWVERSION          0
#define WATERCOUNTER_ZCLVERSION         0

#define WATERCOUNTER_DESCSIZE           8
#define WATERCOUNTER_UNITSIZE           4

#define DEFAULT_PHYSICAL_ENVIRONMENT 0
#define DEFAULT_DEVICE_ENABLE_STATE DEVICE_ENABLED
#define DEFAULT_IDENTIFY_TIME 0

#define ENGINEERING_UNIT_VOLUME_L      82
#define ENGINEERING_UNIT_VOLUME_M3     80
#define ENGINEERING_UNIT_TIME_SEC      73
#define ENGINEERING_UNIT_TIME_MIN      72
#define ENGINEERING_UNIT_TIME_HOUR     71
#define ENGINEERING_UNIT_TIME_DAY      70

#define WC_NV_ITEM_DESC1        0x0404
#define WC_NV_ITEM_DESC2        WC_NV_ITEM_DESC1 + WATERCOUNTER_DESCSIZE
#define WC_NV_ITEM_BLOCK1       WC_NV_ITEM_DESC2 + 4
#define WC_NV_ITEM_BLOCK2       WC_NV_ITEM_BLOCK1 + 4
#define WC_NV_ITEM_REPORT       WC_NV_ITEM_BLOCK2 + 4
#define WC_NV_ITEM_VALUE1       WC_NV_ITEM_REPORT + 4
#define WC_NV_ITEM_VALUE2       WC_NV_ITEM_VALUE1 + 4
#define WC_NV_ITEM_VALUEX       WC_NV_ITEM_VALUE2 + 4

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
const uint16 zclWaterCounter_clusterRevision_all = 0x0001; //currently all cluster implementations are according to ZCL6, which has revision #1. In the future it is possible that different clusters will have different revisions, so they will have to use separate attribute variables.

// Basic Cluster
const uint8 zclWaterCounter_HWRevision = WATERCOUNTER_HWVERSION;
const uint8 zclWaterCounter_ZCLVersion = WATERCOUNTER_ZCLVERSION;
const uint8 zclWaterCounter_ManufacturerName[] = { 15, 'F','o','x','.','I','n','s','t','r','u','m','e','n','t','s' };
const uint8 zclWaterCounter_ModelId[] = { 15, 'F','O','X','0','0','1',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclWaterCounter_DateCode[] = { 15, '2','0','2','4','0','2','0','2',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclWaterCounter_PowerSource = POWER_SOURCE_BATTERY;

uint8 zclWaterCounter_Flow1Desc[WATERCOUNTER_DESCSIZE];
uint32 zclWaterCounter_Flow1Value;
uint16 zclWaterCounter_Flow1Multiplyer;
uint16 zclWaterCounter_Flow1Unit;
uint8 zclWaterCounter_Flow1Status;

uint8 zclWaterCounter_Flow2Desc[WATERCOUNTER_DESCSIZE];
uint32 zclWaterCounter_Flow2Value;
uint16 zclWaterCounter_Flow2Multiplyer;
uint16 zclWaterCounter_Flow2Unit;
uint8 zclWaterCounter_Flow2Status;

uint16 zclWaterCounter_FlowReportInterval;

uint8 zclWaterCounter_LocationDescription[16];
uint8 zclWaterCounter_PhysicalEnvironment;
uint8 zclWaterCounter_DeviceEnable = DEVICE_ENABLED;

uint8 zclWaterCounter_NVItemsInitStatus;

// Identify Cluster
uint16 zclWaterCounter_IdentifyTime = 0;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
 
  // NOTE: The attributes listed in the AttrRec must be in ascending order 
  // per cluster to allow right function of the Foundation discovery commands
 
CONST zclAttrRec_t zclWaterCounter_Attrs[] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclWaterCounter_ZCLVersion
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclWaterCounter_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWaterCounter_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWaterCounter_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclWaterCounter_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_ENUM8, ACCESS_CONTROL_READ,
      (void *)&zclWaterCounter_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclWaterCounter_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_DeviceEnable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zclWaterCounter_clusterRevision_all
    }
  },  

  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_IdentifyTime
    }
  },  
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    {  // Attribute record
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_GLOBAL),
      (void *)&zclWaterCounter_clusterRevision_all
    }
  },
  // ********* Flow measure Cluster ********* //
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_DESCRIPTION,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow1Desc
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_PRESENT_VALUE,
      ZCL_DATATYPE_UINT32, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_REPORTABLE),
      (void *)&zclWaterCounter_Flow1Value
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_RESOLUTION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow1Multiplyer
    }
  },
    {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_ENGINEERING_UNITS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow1Unit
    }
  },
{
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_STATUS_FLAG,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow1Status
    }
  },
  { // ********* Flow2 *********
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_DESCRIPTION,
      ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow2Desc
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_PRESENT_VALUE,
      ZCL_DATATYPE_UINT32, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_REPORTABLE),
      (void *)&zclWaterCounter_Flow2Value
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_RESOLUTION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow2Multiplyer
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_ENGINEERING_UNITS,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow2Unit
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_STATUS_FLAG,
      ZCL_DATATYPE_UINT8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_Flow2Status
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    { 
      ATTRID_IOV_BASIC_APP_TYPE,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWaterCounter_FlowUpdateTime
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
    {  
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWaterCounter_clusterRevision_all
    }
  },
  // *** Groups Cluster *** //
  {
    ZCL_CLUSTER_ID_GEN_GROUPS,
    {  
      ATTRID_CLUSTER_REVISION,
      ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CLIENT),
      (void *)&zclWaterCounter_clusterRevision_all
    }
  }
};

uint8 CONST zclWaterCounter_NumAttributes = ( sizeof(zclWaterCounter_Attrs) / sizeof(zclWaterCounter_Attrs[0]) );

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t zclWaterCounter_InClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC
};

#define ZCLWATERCOUNTER_MAX_INCLUSTERS    ( sizeof( zclWaterCounter_InClusterList ) / sizeof( zclWaterCounter_InClusterList[0] ))

const cId_t zclWaterCounter_OutClusterList[] =
{
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
  ZCL_CLUSTER_ID_GEN_GROUPS,
};

#define ZCLWATERCOUNTER_MAX_OUTCLUSTERS   ( sizeof( zclWaterCounter_OutClusterList ) / sizeof( zclWaterCounter_OutClusterList[0] ))

SimpleDescriptionFormat_t zclWaterCounter_SimpleDesc =
{
  WATERCOUNTER_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                    //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_FLOW_SENSOR,          //  uint16 AppDeviceId[2];
  WATERCOUNTER_DEVICE_VERSION,            //  int   AppDevVer:4;
  WATERCOUNTER_FLAGS,                     //  int   AppFlags:4;
  ZCLWATERCOUNTER_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
  (cId_t *)zclWaterCounter_InClusterList, //  byte *pAppInClusterList;
  ZCLWATERCOUNTER_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zclWaterCounter_OutClusterList //  byte *pAppInClusterList;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zclWaterCounter_InitAttributesNV
 *
 * @brief   Initialize items in NV for Attribute values.
 *
 * @param   none
 *
 * @return  SUCCESS, FAILURE
 */
uint8 zclWaterCounter_ResetAttributesToDefaultValues(void)
{
  uint8 result = 0;
  
  result |= osal_nv_item_init(WC_NV_ITEM_DESC1, WATERCOUNTER_DESCSIZE, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_DESC2, WATERCOUNTER_DESCSIZE, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_BLOCK1, 4, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_BLOCK2, 4, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_REPORT, 4, NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_VALUE1, sizeof(zclWaterCounter_Flow1Value), NULL); 
  result |= osal_nv_item_init(WC_NV_ITEM_VALUE2, sizeof(zclWaterCounter_Flow2Value), NULL);
  
  if (result == SUCCESS) {
    zclWaterCounter_NVItemsInitStatus = SUCCESS
  } else {
    zclWaterCounter_NVItemsInitStatus = FAILURE
  }
}

/*********************************************************************
 * @fn      zclWaterCounter_ResetAttributesToDefaultValues
 *
 * @brief   Reset all writable attributes to their default values.
 *
 * @param   none
 *
 * @return  none
 */
void zclWaterCounter_ResetAttributesToDefaultValues(void)
{
  const uint8 Desc1[WATERCOUNTER_DESCSIZE] = {WATERCOUNTER_DESCSIZE - 1, 'C', 'o', 'l', 'd', ' ', ' ', ' ' };
  const uint8 Desc2[WATERCOUNTER_DESCSIZE] = {WATERCOUNTER_DESCSIZE - 1, 'H', 'o', 't', ' ', ' ', ' ', ' ' };
  int i;
  
  zclWaterCounter_LocationDescription[0] = 15;
  for (i = 1; i <= 15; i++)
  {
    zclWaterCounter_LocationDescription[i] = ' ';
  }
  
  zclWaterCounter_PhysicalEnvironment = DEFAULT_PHYSICAL_ENVIRONMENT;
  zclWaterCounter_DeviceEnable = DEFAULT_DEVICE_ENABLE_STATE;
  
  zclWaterCounter_IdentifyTime = DEFAULT_IDENTIFY_TIME;
  
  osal_memcpy(zclWaterCounter_Flow1Desc, Desc1, WATERCOUNTER_DESCSIZE);
  osal_memcpy(zclWaterCounter_Flow2Desc, Desc2, WATERCOUNTER_DESCSIZE);
  Flow1Unit = ENGINEERING_UNIT_VOLUME_L;
  Flow2Unit = ENGINEERING_UNIT_VOLUME_L;
  
  zclWaterCounter_Flow1Value = 0;
  zclWaterCounter_Flow2Value = 0;
  zclWaterCounter_Flow1Multiplyer = 10;
  zclWaterCounter_Flow2Multiplyer = 10;
  zclWaterCounter_Flow1Status = 1;
  zclWaterCounter_Flow2Status = 1;
}

/****************************************************************************
****************************************************************************/


