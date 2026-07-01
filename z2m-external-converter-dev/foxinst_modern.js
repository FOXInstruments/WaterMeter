// @ts-nocheck
/**
 * @typedef {import('zigbee-herdsman/dist/controller/model').Endpoint} ZhEndpoint
 * @typedef {import('zigbee-herdsman/dist/controller/model').Device} ZhDevice
 * @typedef {import('zigbee-herdsman/dist/controller/model').Group} ZhGroup
 * @typedef {{Endpoint: ZhEndpoint, Device: ZhDevice, Group: ZhGroup}} Zh
 * @typedef {{[s: string]: unknown}} KeyValue
 * @typedef {{[s: string]: any}} KeyValueAny
 */

/**
 * @typedef {{
 *   type: "stop";
 *   data: { ieeeAddr: string; };
 * } | {
 *   type: "deviceNetworkAddressChanged" | "deviceAnnounce" | "deviceJoined" | "start";
 *   data: {
 *     device: ZhDevice;
 *     deviceExposesChanged: () => void;
 *     state: KeyValue;
 *     options: KeyValue;
 *   };
 * } | {
 *   type: "deviceOptionsChanged";
 *   data: {
 *     device: ZhDevice;
 *     deviceExposesChanged: () => void;
 *     state: KeyValue;
 *     options: KeyValue;
 *     from: KeyValue;
 *     to: KeyValue;
 *   };
 * } | {
 *   type: "deviceInterview";
 *   data: {
 *     device: ZhDevice;
 *     deviceExposesChanged: () => void;
 *     state: KeyValue;
 *     options: KeyValue;
 *     status: "started" | "successful" | "failed";
 *   };
 * }} OnEventEvent
 */

/**
 * @typedef {Object} FzMessage
 * @property {Record<number, unknown> & Record<string, unknown>} data
 * @property {ZhEndpoint} endpoint
 * @property {ZhDevice} device
 * @property {{zclTransactionSequenceNumber?: number, manufacturerCode?: number, frameControl?: any, rawData: Buffer}} meta
 * @property {number} groupID
 * @property {string} type
 * @property {string | number} cluster
 * @property {number} linkquality
 */

/**
 * @typedef {Object} FzMeta
 * @property {KeyValue} state
 * @property {ZhDevice} device
 * @property {() => void} deviceExposesChanged
 */

/**
 * @typedef {Object} TzMeta
 * @property {KeyValue} message
 * @property {ZhDevice | undefined} device
 * @property {any} mapped
 * @property {KeyValue} options
 * @property {KeyValue} state
 * @property {string | undefined} endpoint_name
 * @property {{[s: string]: KeyValue} | undefined} membersState
 * @property {(payload: KeyValue) => void} publish
 * @property {KeyValue} [converterOptions]
 */

const fz = require("zigbee-herdsman-converters/converters/fromZigbee");
const exposes = require("zigbee-herdsman-converters/lib/exposes");
const reporting = require("zigbee-herdsman-converters/lib/reporting");
const utils = require("zigbee-herdsman-converters/lib/utils");
const me = require("zigbee-herdsman-converters/lib/modernExtend");
const enum_dt = require("zigbee-herdsman/dist/zspec/zcl/definition/enums");
const {logger} = require("zigbee-herdsman-converters/lib/logger");

const e = exposes.presets;
const ea = exposes.access;

const meteringSiteIdLen = 10;

const unitsZiPulses = [
    "kWh",
    "m³",
    "ft³",
    "ccf",
    "US gl",
    "IMP gl",
    "BTUs",
    "L (litre)",
    "kPA (jauge)",
    "kPA (absolu)",
    "kPA (absolu)",
    "sans unité",
    "MJ",
    "kVar",
];

const ReportingPeriodValues = [5, 10, 15, 20, 30, 60, 2 * 60, 3 * 60, 4 * 60, 6 * 60, 8 * 60, 12 * 60, 24 * 60];
const ReportingPeriodStrings = [
    "5 minutes",
    "10 minutes",
    "15 minutes",
    "20 minutes",
    "30 minutes",
    "1 hour",
    "2 hours",
    "3 hours",
    "4 hours",
    "6 hours",
    "8 hours",
    "12 hours",
    "1 day",
];

const meteringDeviceTypeValues = [0, 1, 2, 3, 4, 5, 6, 128, 129, 130, 131, 132, 133];
const meteringDeviceTypeStrings = [
    "Electric metering",
    "Gas Metering",
    "Water Metering",
    "Thermal Metering (deprecated)",
    "Pressure Metering",
    "Heat Metering",
    "Cooling Metering",
    "Mirrored Gas Metering",
    "Mirrored Water Metering",
    "Mirrored Thermal Metering (deprecated)",
    "Mirrored Pressure Metering",
    "Mirrored Heat Metering",
    "Mirrored Cooling Metering",
];

const meteringStatusStrings = [
    "Check meter",
    "Low battery",
    "Tamper detect",
    "Pipe empty",
    "Low pressure",
    "Leak detect",
    "Service disconnect",
    "Reverse flow",
];

const DiagPersistMemoryFailItemStrings = [
    "SITEID1",
    "SITEID2",
    "UNIT1",
    "UNIT2",
    "MULTIPLIER1",
    "MULTIPLIER2",
    "DIVISOR1",
    "DIVISOR2",
    "VOLUMEREPORT1",
    "VOLUMEREPORT2",
    "REPORTPERIOD",
    "VOLTAGERATED",
    "VALUES1",
    "VALUES2",
    "DEBOUNCE1",
    "DEBOUNCE2",
];

const DiagReportConfigValues = [0, 1, 2, 4, 5, 6];
const DiagReportConfigStrings = [
    "No report",
    "Report every update",
    "Report every 24h",
    "No report (Test)",
    "Report every update (Test)",
    "Report every 24h (Test)",
];

const DiagCPUResetReasonStrings = ["Power-on and brownout detection", "External reset", "Watchdog timer reset", "Clock loss reset"];
const DiagCPUPowerModeStrings = ["Active or Idle mode", "Power mode 1 (PM1)", "Power mode 2 (PM2)", "Power mode 3 (PM3)"];
const DiagCPUCurrentSpeed = [32, 16, 8, 4, 2, 1, 0.5, 0.25];
const DiagCPUCurrentSystemClockStrings = ["32 MHz XOSC", "16 MHz RCOSC"];
const DiagCPUCurrentSystemClockSrcStrings = ["32 kHz XOSC", "16 kHz RCOSC"];

const foxExtend = {
    foxBattery:() => {
        const BatteryRatedVoltageValues = [30, 33, 36, 42];
        const BatteryRatedVoltageStrings = ["DC3_0V", "DC3_3V", "DC3_6V", "DC4_2V"];
        //const BatteryRatedVoltageStrings = {30: 'DC3_0V', 33: 'DC3_3V', 36: 'DC3_6V', 42: 'DC4_2V'};
        const exposes = [
            e.battery().withAccess(ea.STATE_GET),
            e.voltage().withAccess(ea.STATE),
            e.enum("batteryRatedVoltageStr", ea.STATE_SET, BatteryRatedVoltageStrings)
                .withDescription("Press the Link button to upload value")
                .withLabel("Battery rated voltage")
                .withCategory("config"),
            e.battery_low().withAccess(ea.STATE),
        ];
        const fromZigbee = [
            {
                cluster: "genPowerCfg",
                type: ["attributeReport", "readResponse"],
                /**
                 * @param {any} model
                 * @param {FzMessage} msg
                 * @param {any} publish
                 * @param {KeyValue} options
                 * @param {FzMeta} meta
                 */
                convert: (model, msg, publish, options, meta) => {
                    logger.warning(`fzBattery Read: ${Object.keys(msg.data)} ${Object.values(msg.data)}`, msg.device.ieeeAddr);
                    const payload = {};
                    if (msg.data.batteryVoltage !== undefined) {
                        //const property = ('batteryVoltage', msg, model, meta);
                        payload.voltage = msg.data.batteryVoltage / 10; // Unit of 100mV, convert to V
                    }
                    if (msg.data.batteryRatedVoltage !== undefined) {
                        //const property = ('batteryRatedVoltage', msg, model, meta);
                        payload.batteryRatedVoltageStr = BatteryRatedVoltageStrings[BatteryRatedVoltageValues.indexOf(msg.data.batteryRatedVoltage)];
                        payload.batteryRatedVoltage = msg.data.batteryRatedVoltage / 10; // Unit of 100mV, don't convert to V
                    }
                    if (msg.data.batteryPercentageRemaining !== undefined) {
                        //const property = ('batteryPercentageRemaining', msg, model, meta);
                        payload.battery = msg.data.batteryPercentageRemaining / 2; // Value 0 - 200, convert to 0 - 100%
                    }
                    if (msg.data.batteryAlarmState !== undefined) {
                        const battery1Low =
                            (msg.data.batteryAlarmState & (1 << 0) ||
                                msg.data.batteryAlarmState & (1 << 1) ||
                                msg.data.batteryAlarmState & (1 << 2) ||
                                msg.data.batteryAlarmState & (1 << 3)) > 0;
                        const battery2Low =
                            (msg.data.batteryAlarmState & (1 << 10) ||
                                msg.data.batteryAlarmState & (1 << 11) ||
                                msg.data.batteryAlarmState & (1 << 12) ||
                                msg.data.batteryAlarmState & (1 << 13)) > 0;
                        const battery3Low =
                            (msg.data.batteryAlarmState & (1 << 20) ||
                                msg.data.batteryAlarmState & (1 << 21) ||
                                msg.data.batteryAlarmState & (1 << 22) ||
                                msg.data.batteryAlarmState & (1 << 23)) > 0;
                        payload.battery_low = battery1Low || battery2Low || battery3Low;
                    }
                    return payload;
                },
            },
        ];

        const toZigbee = [
            {
                key: ["batteryRatedVoltageStr", "battery", "voltage", "battery_low"],
                /**
                 * @param {Zh.Endpoint} entity
                 * @param {string} key
                 * @param {any} value
                 * @param {TzMeta} meta
                 */
                convertSet: async (entity, key, value, meta) => {
                    logger.warning(`tzBattery write: ${key} ${value}`, meta.device.ieeeAddr);
                    if (key === "batteryRatedVoltageStr") {
                        utils.assertString(value, "batteryRatedVoltageStr");
                        const val = BatteryRatedVoltageStrings.indexOf(value);
                        const payload = {batteryRatedVoltage: BatteryRatedVoltageValues[val]};
                        await entity.write("genPowerCfg", payload);
                        await entity.read("genPowerCfg", ["batteryRatedVoltage"]);
                        return {state: {batteryRatedVoltageStr: value}};
                    }
                    // biome-ignore lint/style/noUselessElse: <Because>
                    else {
                        await entity.write("genPowerCfg", {[key]: value});
                    }
                    return {state: {[key]: value}};
                },
                /**
                 * @param {Zh.Endpoint} entity
                 * @param {string} key
                 * @param {TzMeta} meta
                 */
                convertGet: async (entity, key, meta) => {
                    //const ep = determineEndpoint(entity, meta, 'genPowerCfg');
                    logger.warning(`tzBattery Get: ${key}`, meta.device.ieeeAddr);
                    if (key === "voltage") await entity.read("genPowerCfg", ["batteryVoltage", "batteryPercentageRemaining"]);
                    else if (key === "battery")
                        await entity.read("genPowerCfg", ["batteryVoltage", "batteryPercentageRemaining", "batteryAlarmState", "batteryRatedVoltage"]);
                    else if (key === "battery_low") await entity.read("genPowerCfg", ["batteryAlarmState"]);
                    else if (key === "batteryRatedVoltageStr") await entity.read("genPowerCfg", ["batteryRatedVoltage"]);
                    else await entity.read("genPowerCfg", [key]);
                },
            },
        ];
        return {
            exposes,
            fromZigbee,
            toZigbee,
            isModernExtend: true,
        };
    },

    foxMetering:() => {
        const fromZigbee = [
            {
                cluster: "seMetering",
                type: ["attributeReport", "readResponse"],
                /**
                 * @param {any} model
                 * @param {FzMessage} msg
                 * @param {any} publish
                 * @param {KeyValue} options
                 * @param {FzMeta} meta
                 */
                convert: (model, msg, publish, options, meta) => {
                    const payload = {};
                    logger.warning(`fzMeter[${msg.endpoint.ID}] read: ${Object.keys(msg.data)} ${Object.values(msg.data)}`, msg.device.ieeeAddr);
                    if (msg.data.multiplier !== undefined) {
                        const property = utils.postfixWithEndpointName("multiplier", msg, model, meta);
                        payload[property] = msg.data.multiplier;
                    }
                    if (msg.data.divisor !== undefined) {
                        const property = utils.postfixWithEndpointName("divisor", msg, model, meta);
                        payload[property] = msg.data.divisor;
                    }
                    if (msg.data.unitOfMeasure !== undefined) {
                        const property = utils.postfixWithEndpointName("unitOfMeasure", msg, model, meta);
                        let val = msg.data.unitOfMeasure;
                        if (val >= 0x80) val -= 0x80;
                        const unitValue = unitsZiPulses[val];
                        payload[property] = unitValue;
                    }
                    if (msg.data.volumePerReport !== undefined) {
                        const property = utils.postfixWithEndpointName("volumePerReport", msg, model, meta);
                        payload[property] = msg.data.volumePerReport;
                    }
                    if (msg.data.defaultUpdatePeriod !== undefined) {
                        payload.measurement_poll_interval = msg.data.defaultUpdatePeriod;
                    }
                    if (msg.data["16"] !== undefined) {
                        // intervalReadReportingPeriod
                        const val = ReportingPeriodValues.indexOf(msg.data["16"]);
                        payload.intervalReadReportingPeriod = ReportingPeriodStrings[val];
                    }
                    if (msg.data.hoursInOperation !== undefined) {
                        const property = utils.postfixWithEndpointName("hoursInOperation", msg, model, meta);
                        payload[property] = msg.data.hoursInOperation;
                    }
                    if (msg.data.meteringDeviceType !== undefined) {
                        const property = utils.postfixWithEndpointName("meteringDeviceType", msg, model, meta);
                        const val = meteringDeviceTypeValues.indexOf(msg.data.meteringDeviceType);
                        if (val >= 0) payload[property] = meteringDeviceTypeStrings[val];
                        else payload[property] = "unknown";
                    }
                    if (msg.data.status !== undefined) {
                        const property = utils.postfixWithEndpointName("status", msg, model, meta);
                        const val = msg.data.status;
                        let mask = 1;
                        let stat = "";
                        for (const i of meteringStatusStrings) {
                            if (val & mask)
                                if (stat.length > 0) stat += ` | ${i}`;
                                else stat += i;
                            mask <<= 1;
                        }
                        if (stat === "") stat = "OK";
                        payload[property] = stat;
                    }
                    if (msg.data.siteId !== undefined) {
                        const property = utils.postfixWithEndpointName("siteId", msg, model, meta);
                        payload[property] = msg.data.siteId.toString().trim();
                    }
                    //if ((msg.data.currentSummDelivered !== undefined) || (msg.data.instantaneousDemand !== undefined) ||
                    //    (msg.data.currentDayConsumpDelivered !== undefined) || (msg.data.previousDayConsumpDelivered !== undefined))
                    {
                        const multiplier = msg.endpoint.getClusterAttributeValue("seMetering", "multiplier");
                        const divisor = msg.endpoint.getClusterAttributeValue("seMetering", "divisor");
                        const factor = multiplier && divisor ? multiplier / divisor : 1;
                        if (msg.data.currentSummDelivered !== undefined) {
                            const val = utils.precisionRound(msg.data.currentSummDelivered * factor, Math.trunc(Math.log10(divisor ?? 1000)));
                            payload[utils.postfixWithEndpointName("water_consumed", msg, model, meta)] = val;
                            payload[utils.postfixWithEndpointName("currentSummDelivered_raw", msg, model, meta)] = val;
                        }
                        if (msg.data.instantaneousDemand !== undefined) {
                            payload[utils.postfixWithEndpointName("instantaneousDemand", msg, model, meta)] = utils.precisionRound(
                                msg.data.instantaneousDemand * factor,
                                Math.trunc(Math.log10(divisor ?? 1000)),
                            );
                        }
                        if (msg.data.currentDayConsumpDelivered !== undefined) {
                            payload[utils.postfixWithEndpointName("currentDayConsumpDelivered", msg, model, meta)] = utils.precisionRound(
                                msg.data.currentDayConsumpDelivered * factor,
                                Math.trunc(Math.log10(divisor ?? 1000)),
                            );
                        }
                        if (msg.data.previousDayConsumpDelivered !== undefined) {
                            const property = utils.postfixWithEndpointName("previousDayConsumpDelivered", msg, model, meta);
                            payload[property] = utils.precisionRound(msg.data.previousDayConsumpDelivered * factor, Math.trunc(Math.log10(divisor ?? 1000)));
                        }
                    }
                    return payload;
                },
            },
        ];

        const toZigbee = [
            {
                key: [
                    "divisor",
                    "multiplier",
                    "unitOfMeasure",
                    "water_consumed",
                    "currentSummDelivered",
                    "currentSummDelivered_raw",
                    "volumePerReport",
                    "measurement_poll_interval",
                    "intervalReadReportingPeriod",
                    "siteId",
                ],
                /**
                 * @param {Zh.Endpoint} entity
                 * @param {string} key
                 * @param {any} value
                 * @param {TzMeta} meta
                 * @param {KeyValue} options
                 */
                convertSet: async (entity, key, value, meta, options) => {
                    //        const payload = {};
                    //        const ep = me.determineEndpoint(entity, meta, 'seMetering');
                    logger.warning(`tzMeter[${entity.ID}] write: ${key} ${value}`, meta.device.ieeeAddr);                    
                    switch (key) {
                        case "unitOfMeasure": {
                            utils.assertString(value, key);
                            await entity.write("seMetering", {768: {value: unitsZiPulses.indexOf(value), type: enum_dt.DataType.ENUM8}});
                            break;
                        }
                        case "multiplier":
                            utils.assertNumber(value, key);
                            await entity.write("seMetering", {769: {value: value, type: enum_dt.DataType.UINT24}});
                            await entity.read("seMetering", [key]);
                            break;
                        case "divisor":
                            utils.assertNumber(value, key);
                            await entity.write("seMetering", {770: {value: value, type: enum_dt.DataType.UINT24}});
                            await entity.read("seMetering", [key]);
                            break;
                        case "currentSummDelivered_raw": {
                            const multiplier = entity.getClusterAttributeValue("seMetering", "multiplier");
                            const divisor = entity.getClusterAttributeValue("seMetering", "divisor");
                            const factor = multiplier && divisor ? divisor / multiplier : null;
                            utils.assertNumber(value, key);
                            await entity.write("seMetering", {0: {value: utils.precisionRound(value * (factor ?? 1), 0), type: enum_dt.DataType.UINT48}});
                            await entity.read("seMetering", ["currentSummDelivered"]);
                            break;
                        }
                        case "measurement_poll_interval": {
                            utils.assertNumber(value, key);
                            let val = value;
                            if (value < 60) val = 60;
                            if (value > 255) val = 255;
                            await entity.write("seMetering", {defaultUpdatePeriod: val});
                            //entity.read('seMetering', ['defaultUpdatePeriod']);
                            break;
                        }
                        case "intervalReadReportingPeriod":
                            utils.assertString(value, key);
                            await entity.write("seMetering", {
                                16: {value: ReportingPeriodValues[ReportingPeriodStrings.indexOf(value)], type: enum_dt.DataType.UINT16},
                            });
                            await entity.read("seMetering", [16]);
                            break;
                        case "siteId": {
                            utils.assertString(value, key);

                            let val = value.toString();
                            if (val.length >= meteringSiteIdLen) val = val.substring(0, meteringSiteIdLen);
                            else val = val.padEnd(meteringSiteIdLen, " ");
                            //value = val;
                            await entity.write("seMetering", {775: {value: val, type: enum_dt.DataType.OCTET_STR}});
                            await entity.read("seMetering", ["siteId"]);
                            break;
                        }
                        default:
                            await entity.write("seMetering", {[key]: value});
                    }
                    return {state: {[key]: value}};
                },
                /**
                 * @param {Zh.Endpoint} entity
                 * @param {string} key
                 * @param {TzMeta} meta
                 */
                convertGet: async (entity, key, meta) => {
                    //const ep = me.determineEndpoint(entity, meta, 'seMetering');
                    logger.warning(`tzMeter[${entity.ID}] Get: ${key}`, meta.device.ieeeAddr);
                    if (key === "measurement_poll_interval") {
                        //await ep.read('seMetering', ['defaultUpdatePeriod']);
                        //const ep8 = meta.device.getEndpoint(8);
                        const ep9 = meta.device.getEndpoint(9);
                        await entity.read("seMetering", ["defaultUpdatePeriod", "instantaneousDemand"]);
                        await ep9.read("seMetering", ["instantaneousDemand"]);
                    }
                    else if (key === "meteringDeviceType")
                        await entity.read("seMetering", ["meteringDeviceType", "siteId", "status"]);
                    else if (key === "currentSummDelivered" || key === "water_consumed")
                        await entity.read("seMetering", [
                            "currentSummDelivered",
                            "currentDayConsumpDelivered",
                            "previousDayConsumpDelivered",
                            "multiplier",
                            "divisor",
                        ]);
                    else if (key === "intervalReadReportingPeriod") await entity.read("seMetering", [16]);
                    else await entity.read("seMetering", [key]);
                },
            },
        ];
        // Handle unit changes and trigger exposes refresh
        const onEvent = [
            /**
             * @param {OnEventEvent} event
             */
            (event) => {
                if (event.type === "stop") {
                    logger.warning(`Event: ${event.type}`);
                    return;
                }
                logger.warning(`Event: ${event.type}`, event.data.device.ieeeAddr);
                if (event.type === "deviceOptionsChanged") {
                        event.data.deviceExposesChanged();
                        return;
                }
                if (event.type === "start") {
                    event.data.deviceExposesChanged();
                    return;
                }
            },
        ];
        return {
            onEvent,
            fromZigbee,
            toZigbee,
            options: [
                e.enum('unitOfMeasure_IN1', ea.STATE_SET, unitsZiPulses)
                .withLabel('Unit of measure (Endpoint: IN1)')
                .withDescription('Manual unit selection'),
                e.enum('unitOfMeasure_IN2', ea.STATE_SET, unitsZiPulses)
                .withLabel('Unit of measure (Endpoint: IN2)')
                .withDescription('Manual unit selection'),
            ], 
            isModernExtend: true,
        };
    },

    foxDiagnostic:() => {
        const fromZigbee = [
            {
                cluster: "haDiagnostic",
                type: ["attributeReport", "readResponse"],
                /**
                 * @param {any} model
                 * @param {FzMessage} msg
                 * @param {any} _publish
                 * @param {KeyValue} options
                 * @param {FzMeta} meta
                 */
                convert: (model, msg, _publish, options, meta) => {
                    const payload = {};
                    logger.warning(`fzDiag read: ${Object.keys(msg.data)} ${Object.values(msg.data)}`, definitions[1].zigbeeModel[0].trim());
                    if (msg.data.numberOfResets !== undefined) {
                        payload.diagNumOfResets = msg.data.numberOfResets;
                    }
                    if (msg.data.persistentMemoryWrites !== undefined) {
                        payload.diagPersistMemoryWrites = msg.data.persistentMemoryWrites;
                    }
                    if (msg.data["2"] !== undefined) {
                        payload.diagPersistMemoryWriteFails = msg.data["2"];
                    }
                    if (msg.data["3"] !== undefined) {
                        const val = msg.data["3"];
                        let mask = 1;
                        let stat = "";
                        for (const i of DiagPersistMemoryFailItemStrings) {
                            if (val & mask)
                                if (stat.length > 0) stat += ` | ${i}`;
                                else stat += i;
                            mask <<= 1;
                        }
                        if (stat === "") stat = "OK";
                        payload.diagPersistMemoryFailItems = stat;
                    }
                    if (msg.data["4"] !== undefined) {
                        payload.diagMemoryAllocatedBlocks = msg.data["4"];
                    }
                    if (msg.data["5"] !== undefined) {
                        payload.diagMemoryFreeBlocks = msg.data["5"];
                    }
                    if (msg.data["6"] !== undefined) {
                        payload.diagMemoryUsed = msg.data["6"];
                    }
                    if (msg.data["7"] !== undefined) {
                        payload.diagMemoryHighWater = msg.data["7"];
                    }
                    if (msg.data["8"] !== undefined) {
                        payload.diagCPUResetReason = DiagCPUResetReasonStrings[0x0003 & msg.data["8"]];
                        payload.diagCPUPowerMode = DiagCPUPowerModeStrings[0x0003 & (msg.data["8"] >> 2)];
                        payload.diagCPUCurrentSpeed = DiagCPUCurrentSpeed[0x0007 & (msg.data["8"] >> 4)];
                        payload.diagCPUCurrentSystemClock = DiagCPUCurrentSystemClockStrings[0x0001 & (msg.data["8"] >> 10)];
                        payload.diagCPUCurrentSystemClockSrc = DiagCPUCurrentSystemClockSrcStrings[0x0001 & (msg.data["8"] >> 11)];
                    }
                    if (msg.data["9"] !== undefined) {
                        payload.diagSystemUpTime = msg.data["9"];
                    }
                    if (msg.data["10"] !== undefined) {
                        payload.diagReport = DiagReportConfigStrings[DiagReportConfigValues.indexOf(msg.data["10"])];
                    }
                    if (msg.data["11"] !== undefined) {
                        payload.diagPersistMemoryErasedPages = msg.data["11"];
                    }
                    if (msg.data["12"] !== undefined) {
                        payload.diagDebounce = msg.data["12"];
                    }
                    if (msg.data["13"] !== undefined) {
                        payload.diagDebounce1 = msg.data["13"];
                    }
                    if (msg.data["14"] !== undefined) {
                        payload.diagDebounce2 = msg.data["14"];
                    }
                    return payload;
                },
            },
        ];

        const toZigbee = [
            {
                key: ["diagReport", "diagDebounce", "diagDebounce1", "diagDebounce2"],
                /**
                 * @param {Zh.Endpoint} entity
                 * @param {string} key
                 * @param {any} value
                 * @param {TzMeta} meta
                 */
                convertSet: async (entity, key, value, meta) => {
                    logger.warning(`tzDiag write: ${key} ${value}`, definitions[1].zigbeeModel[0].trim());
                    if (key === "diagReport") {
                        utils.assertString(value, "diagReport");
                        const val = DiagReportConfigValues[DiagReportConfigStrings.indexOf(value)];
                        const payload = {10: {value: val, type: enum_dt.DataType.UINT8}};
                        await entity.write("haDiagnostic", payload);
                        await entity.read("haDiagnostic", [10]);
                    }
                    if (key === "diagDebounce") {
                        utils.assertNumber(value, "diagDebounce");
                        const payload = {12: {value: value, type: enum_dt.DataType.UINT16}};
                        await entity.write("haDiagnostic", payload);
                        await entity.read("haDiagnostic", [12]);
                    }
                    return {state: {[key]: value}};
                },
                /**
                 * @param {Zh.Endpoint} entity
                 * @param {string} key
                 * @param {TzMeta} meta
                 */
                convertGet: async (entity, key, meta) => {
                    //const ep = determineEndpoint(entity, meta, 'genPowerCfg');
                    logger.warning(`tzDiag Get: ${key}`, definitions[1].zigbeeModel[0].trim());
                    if (key === "diagReport") {
                        await entity.read("haDiagnostic", [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]);
                        await entity.read("haDiagnostic", [12, 13, 14]);
                    }
                    if (key === "diagDebounce") await entity.read("haDiagnostic", [12, 13, 14]);
                    if (key === "diagDebounce1") {
                        const payload = {13: {value: 0, type: enum_dt.DataType.UINT16}};
                        await entity.write("haDiagnostic", payload);
                        await entity.read("haDiagnostic", [13]);
                    }
                    if (key === "diagDebounce2") {
                        const payload = {14: {value: 0, type: enum_dt.DataType.UINT16}};
                        await entity.write("haDiagnostic", payload);
                        await entity.read("haDiagnostic", [14]);
                    }
                },
            },
        ];
        return {
            fromZigbee,
            toZigbee,
            isModernExtend: true,
        };
    },
};

const definitions = [
    {
        zigbeeModel: ["FOX002"],
        model: "FOX002",
        vendor: "Fox.Instruments",
        description: "Cats feeder",
        fromZigbee: [fz.battery],
        toZigbee: [],
        exposes: [],
    },
    {
        zigbeeModel: ["FOX-Meter001"],
        model: "FOX-Meter001",
        vendor: "Fox.Instruments",
        description: "Water meter",
        extend: [
            me.deviceEndpoints({ endpoints: { IN1: 8, IN2: 9 } }),
            foxExtend.foxMetering(),
            foxExtend.foxDiagnostic(),
            foxExtend.foxBattery(),
        ],
        /**
         * @param {Zh.Device | import('zigbee-herdsman-converters/lib/types').DummyDevice} device
         * @param {KeyValue} options
         */
        exposes: (device, options) => {
            const meteringBaseExposes = (ep1, ep2, unitIn1, unitIn2) => {
                const exposeList = [];
                exposeList.push(exposes.options.measurement_poll_interval().withAccess(ea.STATE_SET | ea.STATE_GET)
                    .withUnit("seconds")
                    .withValueMin(120)
                    .withValueMax(255),
                // Dynamic unit exposes - units update when device meta changes
                e.numeric("instantaneousDemand", ea.STATE).withLabel("Instantaneous consumption").withEndpoint(ep1).withUnit(unitIn1),
                e.numeric("instantaneousDemand", ea.STATE).withLabel("Instantaneous consumption").withEndpoint(ep2).withUnit(unitIn2),

                e.enum("intervalReadReportingPeriod", ea.STATE_SET | ea.STATE_GET, ReportingPeriodStrings)
                    .withDescription("Press the Link button to upload value")
                    .withLabel("Reporting period")
                    .withCategory("config"),
                e.numeric("hoursInOperation", ea.STATE_GET).withUnit("hour").withLabel("Device in operation").withEndpoint(ep1));
                return exposeList;
            };

            const meteringExposes = (ep, unit) => {
                const exposeList = [];
                const description = "Press the Link button to upload value";

                exposeList.push(e.text("meteringDeviceType", ea.STATE_GET).withLabel("Device type").withEndpoint(ep),
                    e.text("siteId", ea.STATE_SET).withLabel("Device ID").withEndpoint(ep).withCategory("config"),
                    e.text("status", ea.STATE).withLabel("Device status").withEndpoint(ep),
                    e.numeric("water_consumed", ea.STATE | ea.STATE_GET)
                        .withLabel("Current consumption")
                        .withEndpoint(ep)
                        .withUnit(unit),
                    e.numeric("currentSummDelivered_raw", ea.STATE | ea.STATE_SET)
                        .withLabel("Current consumption set value")
                        .withValueMin(0).withValueMax(99999.99).withValueStep(0.01)
                        .withEndpoint(ep).withUnit(unit)
                        .withDescription(description),
                    e.numeric("currentDayConsumpDelivered", ea.STATE)
                        .withLabel("Current DAY consumption")
                        .withEndpoint(ep).withUnit(unit),
                    e.numeric("previousDayConsumpDelivered", ea.STATE)
                        .withLabel("Previous DAY consumption")
                        .withEndpoint(ep).withUnit(unit),
                    e.numeric("multiplier", ea.STATE_SET | ea.STATE)
                        .withLabel("Multiplier")
                        .withEndpoint(ep)
                        .withValueMin(1)
                        .withValueMax(1000)
                        .withValueStep(1)
                        .withDescription(description)
                        .withCategory("config"),
                    e.numeric("divisor", ea.STATE_SET | ea.STATE)
                        .withLabel("Divisor")
                        .withEndpoint(ep)
                        .withValueMin(1)
                        .withValueMax(1000)
                        .withValueStep(1)
                        .withDescription(description)
                        .withCategory("config"),
/*                     e.enum("unitOfMeasure", ea.STATE_SET | ea.STATE, unitsZiPulses)
                        .withLabel("Measure unit")
                        .withEndpoint(ep)
                        .withDescription(description)
                        .withCategory("config"),
 */
                    e.numeric("volumePerReport", ea.STATE_SET | ea.STATE_GET)
                        .withLabel("Volume per report (raw units)")
                        .withEndpoint(ep)
                        .withDescription(description)
                        .withCategory("config")
                );
                return exposeList;
            };

            const diagExposes = () => {
                // Diagnostic
                const exposeList = [];
                exposeList.push(e.enum("diagReport", ea.STATE_SET | ea.STATE_GET, DiagReportConfigStrings)
                        .withLabel("Diagnostic report configuration")
                        .withDescription("Press the Link button to upload value. Press the Update button to read all Diagnostic data")
                        .withCategory("config"),
                    e.numeric("diagDebounce", ea.STATE_SET | ea.STATE_GET)
                        .withLabel("Debounce configuration")
                        .withUnit("ms")
                        .withValueMin(100)
                        .withValueMax(2000)
                        .withValueStep(10)
                        .withDescription("Press the Link button to upload value. Press the Update button to read Debounce parameters")
                        .withCategory("config"),
                    e.numeric("diagDebounce1", ea.STATE_GET)
                        .withLabel("Debounce measured value [1]")
                        .withUnit("ms")
                        .withDescription("Press the Link button and the Update button to reset value")
                        .withCategory("diagnostic"),
                    e.numeric("diagDebounce2", ea.STATE_GET)
                        .withLabel("Debounce measured value [2]")
                        .withUnit("ms")
                        .withDescription("Press the link button and the Update button to reset value")
                        .withCategory("diagnostic"),
                    e.numeric("diagNumOfResets", ea.STATE).withLabel("Reset counter").withUnit("count").withCategory("diagnostic"),
                    e.numeric("diagPersistMemoryWrites", ea.STATE).withLabel("Flash memory write").withUnit("byte").withCategory("diagnostic"),
                    e.numeric("diagPersistMemoryErasedPages", ea.STATE)
                        .withLabel("Flash memory erased pages")
                        .withUnit("count")
                        .withCategory("diagnostic"),
                    e.numeric("diagPersistMemoryWriteFails", ea.STATE)
                        .withLabel("Flash memory write fail counter")
                        .withUnit("count")
                        .withCategory("diagnostic"),
                    e.text("diagPersistMemoryFailItems", ea.STATE).withLabel("Flash memory write fail parameters").withCategory("diagnostic"),
                    e.numeric("diagMemoryAllocatedBlocks", ea.STATE).withLabel("Memory allocated blocks").withUnit("block").withCategory("diagnostic"),
                    e.numeric("diagMemoryFreeBlocks", ea.STATE).withLabel("Memory free blocks").withUnit("block").withCategory("diagnostic"),
                    e.numeric("diagMemoryUsed", ea.STATE).withLabel("Memory used").withUnit("byte").withCategory("diagnostic"),
                    e.numeric("diagMemoryHighWater", ea.STATE).withLabel("Memory high water").withUnit("byte").withCategory("diagnostic"),
                    e.text("diagCPUResetReason", ea.STATE).withLabel("CPU Reset reason").withCategory("diagnostic"),
                    e.text("diagCPUPowerMode", ea.STATE).withLabel("CPU Power mode").withCategory("diagnostic"),
                    e.numeric("diagCPUCurrentSpeed", ea.STATE).withLabel("CPU Currnet speed").withUnit("MHz").withCategory("diagnostic"),
                    e.text("diagCPUCurrentSystemClock", ea.STATE).withLabel("CPU Current system clock").withCategory("diagnostic"),
                    e.text("diagCPUCurrentSystemClockSrc", ea.STATE).withLabel("CPU Current system clock source").withCategory("diagnostic"),
                    e.numeric("diagSystemUpTime", ea.STATE).withLabel("System up time").withUnit("seconds").withCategory("diagnostic")
                );
                return exposeList;
            };

            if (!utils.isDummyDevice(device)) {
                const unit1 = options?.unitOfMeasure_IN1 ? options.unitOfMeasure_IN1 : unitsZiPulses[1];
                const unit2 = options?.unitOfMeasure_IN2 ? options.unitOfMeasure_IN2 : unitsZiPulses[1];
                //const unitIn2 = unitsZiPulses[device.getEndpoint(9).getClusterAttributeValue("seMetering", "unitOfMeasure")];
                logger.warning(`Device Unit1: ${unit1} Unit2: ${unit2}`, device.ieeeAddr);
                return [...meteringBaseExposes("IN1", "IN2", unit1, unit2), ...meteringExposes("IN1", unit1),
                        ...meteringExposes("IN2", unit2), ...diagExposes()];
            }
            logger.warning(`Dummy Device: ${device.friendlyName}`, device.ieeeAddr);
            return [...diagExposes()];
        },
//        meta: {multiEndpoint: true},
        /**
         * @param {Zh.Device} device
         * @param {Zh.Endpoint} coordinatorEndpoint
         */
        configure: async (device, coordinatorEndpoint) => {
            const ep8 = device.getEndpoint(8);
            const ep9 = device.getEndpoint(9);
            await reporting.bind(ep8, coordinatorEndpoint, ["genPowerCfg", "seMetering", "haDiagnostic"]);
            await reporting.bind(ep9, coordinatorEndpoint, ["seMetering"]);
            //            await reporting.batteryVoltage(ep8, {min: 3600, max: 7200, change: 1});
            //            await reporting.batteryPercentageRemaining(ep8, {min: 3600, max: 7200, change: 1});
            await ep8.read("genPowerCfg", ["batteryVoltage", "batteryPercentageRemaining", "batteryAlarmState", "batteryRatedVoltage"]);
            await ep8.read("seMetering", ["divisor",
                                            "multiplier",
                                            "volumePerReport",
                                            "currentSummDelivered",
                                            "siteId",
                                            "meteringDeviceType",
                                            "status",
                                            "intervalReadReportingPeriod",
            ]);
            await ep9.read("seMetering", ["divisor",
                                            "multiplier",
                                            "volumePerReport",
                                            "currentSummDelivered",
                                            "siteId",
                                            "meteringDeviceType",
                                            "status",
                                            "intervalReadReportingPeriod",
            ]);
        },
    },
];

module.exports = definitions;
