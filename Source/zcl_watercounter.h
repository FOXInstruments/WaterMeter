/**************************************************************************************************
  Filename:       zcl_watercounter.h
  Revised:        $Date: 2015-08-19 17:11:00 -0700 (Wed, 19 Aug 2015) $
  Revision:       $Revision: 44460 $


  Description:    This file contains the Zigbee Cluster Library Home
                  Automation Sample Application.


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

#ifndef ZCL_WATERCOUNTER_H
#define ZCL_WATERCOUNTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl.h"

/*********************************************************************
 * CONSTANTS
 */
#define WC_ENDPOINT              8
#define WC_DEBOUNCE              50
#define WC_LONGPUSH_INTERVAL     100

// Events for the sample app
#define SAMPLEAPP_END_DEVICE_REJOIN_EVT   0x00E0

// Events
#define SAMPLEAPP_IMPULSE1_EVT            0x00E1  
#define SAMPLEAPP_IMPULSE2_EVT            0x00E2  
#define SAMPLEAPP_EVERYHOUR_EVT           0x00E3
#define SAMPLEAPP_LONGPUSH_EVT            0x00E4

#define SAMPLEAPP_END_DEVICE_REJOIN_DELAY 10000

// Vdd/3 / Internal Reference X ENOB --> (Vdd / 3) / 1.15 X 127
#define VDD3_2_0                74   // 2.0 V required to safely read/write internal flash.
#define VDD3_THRES_MIN          92   // 2.5 V minimum LeFePO4 voltage
#define VDD3_MIN_RUN            (VDD_2_0+4)  // VDD_MIN_RUN = VDD_MIN_NV
#define VDD3_MIN_NV             (VDD_2_0+4)  // 5% margin over minimum to survive a page erase and compaction.
#define VDD3_MIN_GOOD           (VDD_2_0+8)  // 10% margin over minimum to survive a page erase and compaction.
#define VDD_NORMAL_VOLTAGE      3.6
#define VDD3TOVOLTAGE(v)        (v / 127.0 * 1.15 * 3)   // Convert ADC value to Voltage
  
#define REPORT_CHANGE_VOLTAGE   0.1     // Change values for BDB_REPORTING
#define REPORT_CHANGE_FLOW      100
  
#define TIME_SYNC_DIFF          5L       // Difference between device time and gate time
/*********************************************************************
 * MACROS
 */
#define POLARITY_IMPULSE        ACTIVE_LOW

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern SimpleDescriptionFormat_t zclWC_SimpleDesc;

extern CONST zclAttrRec_t zclWC_Attrs[];

extern uint8    zclWC_Flow1Desc[];
extern uint32   zclWC_Flow1Value;
extern uint16   zclWC_Flow1Multiplyer;
extern uint16   zclWC_Flow1Unit;
extern uint8    zclWC_Flow1Status;

extern uint8    zclWC_Flow2Desc[];
extern uint32   zclWC_Flow2Value;
extern uint16   zclWC_Flow2Multiplyer;
extern uint16   zclWC_Flow2Unit;
extern uint8    zclWC_Flow2Status;

extern float zclWC_BatteryVoltage;
extern float zclWC_BatteryVoltageThresMin;    // 2.5V LiFePO4 T>0 degC
extern uint8 zclWC_BatteryAlarmMask;
extern uint8 zclWC_BatteryAlarmState;

extern uint16   zclWC_FlowReportInterval;     // Time interval in seconds for Reporting compatability

extern uint16   zclWC_IdentifyTime;

extern CONST uint8 zclWC_NumAttributes;

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void zclWC_Init(byte task_id);

/*
 *  Event Process for the task
 */
extern UINT16 zclWC_event_loop(byte task_id, UINT16 events);

/*
 *  Reset all writable attributes to their default values.
 */
extern void zclWC_ResetAttributesToDefaultValues(void); //implemented in zcl_watercounter_data.c
extern void zclWC_NVInitItems(void);
extern uint8 zclWC_NVItemCheck(uint16 id);
extern void zclWC_InitAttribute(uint16 id, uint16 len, const void *src, void *buf);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_SAMPLEAPP_H */
