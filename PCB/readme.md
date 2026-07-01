# WaterMeter PCB

Designed in Altium designer.<br>
[PDF file](PCB_Water_meter_device.pdf) - Top and bottom layers for the production of printed circuit boards using laser-ironing technology.

PCB is modified to deliver the convenience of DIY manufacturing. VIAs size are increased. Sights are added to perform centering top and bottom layers. All pads with holes have centering dots to positioning the drill.

# Power source
User can use LiFePO4 or Li-Ion accumalator.

If LiFePO4 than **JP1** and **JP2** should be .. to exclude LDO 3.6V and supply device directly from accumulator

If Li-Ion than **JP1** and **JP2** should be .. to use LDO 3.6V

# Connectivity

**J1** - Inpit 1<br>
**J1** - Inpit 2<br>
**J1** - UART<br>
**J2** - DEBUG<br>

# LEDs and Keys

**LED1** - Blue, Input 1<br>
**LED2** - Red, Input 2<br>
**LED3** - Green, Device working mode<br>
**LED4** - Spare
**Key1** - Select device working mode<br>
**Key2** - Reboot device

LED3 state    | Key1 state  | Device mode
--------------|-------------|--
Light  | Key1 pressed and released | Device active for 2 minutes
Blink 500ms | Key1 pressed more than 5sec | Comissioning mode 
Blink 200ms | Key1 pressed more than 10sec | Device factory reset
