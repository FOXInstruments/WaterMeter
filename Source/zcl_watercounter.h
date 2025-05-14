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
#define ZAPP_ENDPOINT              8
#define ZAPP_ENDPOINT2             (ZAPP_ENDPOINT + 1)
// Timeouts
#define ZAPP_TIMEOUT_DEBOUNCE             50
#define ZAPP_TIMEOUT_LONGPUSH             100
#define ZAPP_TIMEOUT_STOREATTR            60000
#define ZAPP_TIMEOUT_END_DEVICE_REJOIN    60000        // msec

// Events for the sample app
#define ZAPP_EVT_END_DEVICE_REJOIN   0x0001        // event_flag is a 2-byte bitmap with each bit specifying an event

// Events
#define ZAPP_EVT_IMPULSE1            0x0002
#define ZAPP_EVT_IMPULSE2            0x0004  
#define ZAPP_EVT_UPDATE              0x0008
#define ZAPP_EVT_LONGPUSH            0x0010
#define ZAPP_EVT_UPDATEINSTDEMAND    0x0020       // InstanteniousDemand update event
#define ZAPP_EVT_BATTERY             0x0040
#define ZAPP_EVT_TIMESYNC            0x0080
#define ZAPP_EVT_COMMISSION          0x0100       // Init commission if device was connected to network
#define ZAPP_EVT_STOREATTR           0x0200       // Store changed attributes into NV memory

// Vdd/3 / Internal Reference X ENOB --> (Vdd / 3) / 1.15 X 127
#define VDD3_2_0                74   // 2.0 V required to safely read/write internal flash.
#define VDD3_THRES_MIN          92   // 2.5 V minimum LeFePO4 voltage
#define VDD3_MIN_RUN            (VDD_2_0+4)  // VDD_MIN_RUN = VDD_MIN_NV
#define VDD3_MIN_NV             (VDD_2_0+4)  // 5% margin over minimum to survive a page erase and compaction.
#define VDD3_MIN_GOOD           (VDD_2_0+8)  // 10% margin over minimum to survive a page erase and compaction.
#define VDD_VOLTAGE_RATED42     42           // 0.1V unit
#define VDD_VOLTAGE_RATED36     36           // 0.1V unit
#define VDD_VOLTAGE_RATED33     33           // 0.1V unit
#define VDD_VOLTAGE_RATED30     30           // 0.1V unit
#define VDD_VOLTAGE36_MIN         26           // 0.1V unit
#define VDD_VOLTAGE36_THRES1      30           // 0.1V unit
#define VDD3TOVOLTAGE(v)        (v * 11.5 * 3 / 127.0)   // Convert ADC value to Voltage 0.1V unit
  
#define ZAPP_METER_SITEID_SIZE              10
#define ZAPP_METER_MULTIPLIER               10    // Weight of one count
#define ZAPP_METER_DIVISOR                  1000  // Divider to calculate flow rate
#define ZAPP_METER_REPORT_INTERVAL          60    // Reporting interval in minutes
#define ZAPP_METER_INSTDEMAND_UPDATEPERIOD_MIN  60   // instDemand update period in seconds 60-254, 255 - disable
#define ZAPP_METER_INSTDEMAND_UPDATEPERIOD_MAX  255
#define ZAPP_REPORT_CHANGE_VOLTAGE          1     // Change values for BDB_REPORTING (0.1V unit)
#define ZAPP_VOLUME_PER_REPORT             10
  
#define TIME_SYNC_DIFF          5L       // Difference between device time and gate time
  
#define ATTRID_METER_0READINGSET_CURRSUMDELIVERED        0x0000  // uint48, CurrentSummationDelivered
#define ATTRID_METER_0READINGSET_CURRSUMRECIVED          0x0001  // uint48, CurrentSummationRecived
#define ATTRID_METER_0READINGSET_DFT_SUM                 0x0004  // uint48, Snapshot of attribute CurrentSummationDelivered captured at the time indicated by attribute DailyFreezeTime.
#define ATTRID_METER_0READINGSET_DAILYFREEZETIME         0x0005  // uint16, Time of day when DFTSummation is captured. Bits 0 to 7: minute, Bits 8 to 15: hour
#define ATTRID_METER_0READINGSET_POWERFACTOR             0x0006  // int8, Average Power Factor ratio in 1/100ths. Valid values are 0 to 99
#define ATTRID_METER_0READINGSET_READINGSNAPSHOTTIME     0x0007  // UTC, Last time all of the CurrentSummationDelivered, CurrentSummationReceived, CurrentMaxDemandDelivered, and CurrentMaxDemandReceived attributes that are supported by the device were updated
#define ATTRID_METER_0READINGSET_DEFAULTUPDATEPERIOD     0x000A  // uint8, Interval (seconds) at which the InstantaneousDemand attribute is updated when not in fast poll mode
#define ATTRID_METER_0READINGSET_FASTPOLLUPDATEPERIOD    0x000B  // uint8, Interval (seconds) at which the InstantaneousDemand attribute is updated when in fast poll mode
#define ATTRID_METER_0READINGSET_DAILYCONSUMPTIONTARGET  0x000D  // uint24, Daily target consumption amount that can be displayed to the consumer on a HAN device
#define ATTRID_METER_0READINGSET_CURRBLOCK               0x000E  // enum8, When Block Tariffs are enabled, CurrentBlock is an 8-bit Enumeration which indicates the currently active block
                                                                 // 0-No Blocks in use
#define ATTRID_METER_0READINGSET_PROFILEINTERVALPERIOD   0x000F  // enum8
#define ATTRID_METER_0READINGSET_INTERVALREPORTING       0x0010  // uint16, How often (in minutes) the water or gas meter is to wake up and provide interval data
#define ATTRID_METER_0READINGSET_PRESETREADINGTIME       0x0011  // uint16, Time of day (in quarter hour increments) at which the meter will wake up and report a register reading even
                                                                 // if there has been no consumption for the previous 24 hours
                                                                 // Bits 0 to 7: minute, Bits 8 to 15: hour
#define ATTRID_METER_0READINGSET_VOLUMEPERREPORT         0x0012  // uint16, Volume per report increment from the water or gas meter. For a water meter it might report the register value every 10 liters of water usage
#define ATTRID_METER_0READINGSET_FLOWRESTRICTION         0x0013  // uint8, Represents the volume per minute limit set in the flow restrictor, 0xFF indicates this feature is disabled
#define ATTRID_METER_0READINGSET_SUPPLYSTATUS            0x0014  // enum8, 0-Supply OFF, 1-Supply OFF/ARMED, 2-Supply ON 
  
#define ATTRID_METER_2STATUS_STATUS                      0x0200  // map8, 7-Reverse flow 6-Service disconnected 5-Leak detect 4-Low press 3-Pipe empty 2-Tamper detect 1-Low batt 0-Check meter
#define ATTRID_METER_2STATUS_REMAININGBATT               0x0201  // uint8, 0 - 100, 0xFF - feature disabled
#define ATTRID_METER_2STATUS_HOURSINOPERATION            0x0202  // uint24
#define ATTRID_METER_2STATUS_HOURSINFAULT                0x0203  // uint24
  
#define ATTRID_METER_3FORMATTING_UNIT                    0x0300  // enum8, 0x81 m3, 0x80 kWh, 0x87 liter
#define ATTRID_METER_3FORMATTING_MULTIPLIER              0x0301  // uint24, Summation = Summation received * Multiplier / Divisor
#define ATTRID_METER_3FORMATTING_DIVISOR                 0x0302  // uint24, Summation = Summation received * Multiplier / Divisor
#define ATTRID_METER_3FORMATTING_SUMFORMATTING           0x0303  // map8, Bits 0 to 2: Number of Digits to the right of the Decimal Point
                                                                 // Bits 3 to 6: Number of Digits to the left of the Decimal Point 
                                                                 // Bit 7: If set, suppress leading zeros
#define ATTRID_METER_3FORMATTING_DEMANDFORMATTING        0x0304  // See above
#define ATTRID_METER_3FORMATTING_HISTORYFORMATTING       0x0305  // See above
#define ATTRID_METER_3FORMATTING_METERDEVICETYPE         0x0306  // map8, 0 Electric Metering, 1 Gas Metering, 2 Water Metering ,3 Thermal Metering (deprecated)
                                                                 // 4 Pressure Metering, 5 Heat Metering, 6 Cooling Metering, 128.. Mirrored Gas Metering
#define ATTRID_METER_3FORMATTING_SITEID                  0x0307  // octstr, ZCL Octet String field capable of storing a 32 character string (the first octet indicates length) encoded in UTF-8 format
#define ATTRID_METER_3FORMATTING_METERSERIALNUM          0x0308  // octstr, ZCL Octet String field capable of storing a 24 character string (the first octet indicates length) encoded in UTF-8 format
  
#define ATTRID_METER_4HISTORY_INSTANTDEMAND              0x0400  // int24
#define ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONDELIVER  0x0401  // uint24
#define ATTRID_METER_4HISTORY_CURRDAYCONSUMPTIONRECIVE   0x0402  // uint24
#define ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONDELIVER  0x0403  // uint24
#define ATTRID_METER_4HISTORY_PREVDAYCONSUMPTIONRECIVE   0x0404  // uint24
#define ATTRID_METER_4HISTORY_CURRPROFSTARTTIMEDELIVER   0x0405  // UTC
#define ATTRID_METER_4HISTORY_CURRPROFSTARTTIMERECIVE    0x0406  // UTC
  
#define ATTRID_METER_5PROFILE_MAXNUMPERIODSDELIVER       0x0500  // uint8, represents the maximum number of intervals the device is capable of returning in one Get Profile Response command.
  
#define ATTRID_METER_8ALARMS_GENERICALARMMASK            0x0800  // map16
#define ATTRID_METER_8ALARMS_ELECTRICITYALARMMASK        0x0801  // map32
#define ATTRID_METER_8ALARMS_FLOWPRESSALARMMASK          0x0802  // map16
#define ATTRID_METER_8ALARMS_WATERALARMMASK              0x0803  // map16
#define ATTRID_METER_8ALARMS_HEATCOOLALARMMASK           0x0804  // map16
#define ATTRID_METER_8ALARMS_GASALARMMASK                0x0805  // map16

#define METER_2STATUS_CHECKMETER        0x01
#define METER_2STATUS_LOWBATT           0x02
#define METER_2STATUS_TAMPERDETECT      0x04
   
#define ATTRID_DIAG_0NUMOFRESETS                0x0000  // uint16
#define ATTRID_DIAG_1PERSISTMEMORYWRITES        0x0001  // uint16
#define ATTRID_DIAG_2PERSISTMEMORYWRITEFAILS    0x0002  // uint16
#define ATTRID_DIAG_3PERSISTMEMORYFAILITEMS     0x0003  // uint16
#define ATTRID_DIAG_4MEMALLOCATEDBLOCKS         0x0004  // uint16
#define ATTRID_DIAG_5MEMFREEBLOCKS              0x0005  // uint16
#define ATTRID_DIAG_6MEMUSED                    0x0006  // uint16
#define ATTRID_DIAG_7MEMHIGHWATER               0x0007  // uint16
#define ATTRID_DIAG_8REBOOTREASON               0x0008  // uint16

/*********************************************************************
 * MACROS
 */
#define POLARITY_IMPULSE        ACTIVE_LOW

/*********************************************************************
 * TYPEDEFS
 */
enum
{
  ZAPP_STOREID_SITEID1,
  ZAPP_STOREID_SITEID2,
  ZAPP_STOREID_UNIT1,
  ZAPP_STOREID_UNIT2,
  ZAPP_STOREID_MULTIPLIER1,
  ZAPP_STOREID_MULTIPLIER2,
  ZAPP_STOREID_DIVISOR1,
  ZAPP_STOREID_DIVISOR2,
  ZAPP_STOREID_VOLUMEREPORT1,
  ZAPP_STOREID_VOLUMEREPORT2,
  ZAPP_STOREID_REPORTPERIOD,
  ZAPP_STOREID_VOLTAGERATED,
  ZAPP_STOREID_VALUES1,
  ZAPP_STOREID_DATE1,
  ZAPP_STOREID_VALUES2,
  ZAPP_STOREID_DATE2,
};

typedef union
{
  uint8       array[5];
  struct
  {
    uint32      lowDW;
    uint16      hiW;
  } dw;
} uint48_t;

/*********************************************************************
 * VARIABLES
 */
extern SimpleDescriptionFormat_t zapp_SimpleDesc;
extern SimpleDescriptionFormat_t zapp_SimpleDesc2;

extern CONST zclAttrRec_t zapp_cAttrs[];
extern CONST zclAttrRec_t zapp_cAttrs2[];
extern CONST uint8 zapp_cNumAttributes;
extern CONST uint8 zapp_cNumAttributes2;

extern uint8 zapp_Flow1SiteId[];
extern uint48_t zapp_Flow1Value;
extern int24 zapp_Flow1InstDemand;
extern int24 zapp_Flow1InstDemandPrev;
extern uint24 zapp_Flow1Multiplier;
extern uint24 zapp_Flow1Divisor;
extern uint8 zapp_Flow1Unit;
extern uint16 zapp_Flow1VolumePerReport;
extern uint24 zapp_Flow1CurrDay;
extern uint24 zapp_Flow1PrevDay;
extern uint8 zapp_Flow1Status;

extern uint8 zapp_Flow2SiteId[];
extern uint48_t zapp_Flow2Value;
extern int24 zapp_Flow2InstDemand;
extern int24 zapp_Flow2InstDemandPrev;
extern uint24 zapp_Flow2Multiplier;
extern uint24 zapp_Flow2Divisor;
extern uint16 zapp_Flow2Unit;
extern uint16 zapp_Flow2VolumePerReport;
extern uint24 zapp_Flow2CurrDay;
extern uint24 zapp_Flow2PrevDay;
extern uint8 zapp_Flow2Status;

extern uint8 zapp_BatteryVoltage;             // 0.1V unit
extern uint8 zapp_BatteryVoltageRated;        // 0.1V unit, 3.6V LiFePO4
extern uint8 zapp_BatteryVoltageThresMin;     // 0.1V unit, 2.5V LiFePO4 T>0 degC
extern uint8 zapp_BatteryVoltageThres1;
extern uint8 zapp_BatteryLevel;               // 0 - 100%
extern uint8 zapp_BatteryAlarmMask;
extern uint32 zapp_BatteryAlarmState;

extern uint8 zapp_FlowUpdatePeriod;        // InstDemand update time period in seconds
extern uint16 zapp_FlowIntervalReporting;     // Time interval in minutes for Reporting compatability
extern uint24 zapp_Flow1HoursInOperation;
extern uint24 zapp_Flow2HoursInOperation;
// Diagnostic cluster
extern uint16 zapp_DiagNumOfResets;
extern uint16 zapp_DiagNVMemWrites;
extern uint16 zapp_DiagNVMemWriteFails;
extern uint32 zapp_DiagNVMemFailItems;
extern uint16 zapp_DiagMemAllocatedBlocks;
extern uint16 zapp_DiagMemFreeBlocks;
extern uint16 zapp_DiagMemUsed;       // Used memory in bytes
extern uint16 zapp_DiagMemHighWater;  // Maximum memory allocated
extern uint16 zapp_DiagRebootReason;

extern uint16 zapp_IdentifyTime;

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void zapp_Init(byte task_id);

/*
 *  Event Process for the task
 */
extern UINT16 zapp_event_loop(byte task_id, UINT16 events);

/*
 *  Reset all writable attributes to their default values.
 */
extern void zapp_fResetAttributesToDefaultValues(void); //implemented in zcl_watercounter_data.c
extern void zapp_fNVInitItems(void);
extern uint8 zapp_fNVCheckItem(uint16 id, uint16 len);
extern Status_t zapp_fStoreQueueAdd(uint8 idx, uint8 size, void *src);
extern uint32 zapp_fStoreAttrToNV(void);
extern void zapp_fInitAttrValue(uint16 id, uint16 len, const void *src, void *buf);
extern void zapp_fUpdateAttrIntervalReporting(uint16 *data);
extern void zapp_fUpdateAttrRatedVoltage(uint8 *data);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_SAMPLEAPP_H */
