"""Communicator MCU/CAN circuitry; pin numbers are physical package pads."""
from kicad_model import Circuit, LIB

SOT5 = 'Package_TO_SOT_SMD:SOT-23-5'
SC6 = 'Package_TO_SOT_SMD:SOT-363_SC-70-6'
VSS8 = 'Package_SO:VSSOP-8_2.3x2mm_P0.5mm'


def stm32(c):
    s = c.sheet('04_stm32', 'STM32G474CEU6 / UFQFPN48. Pin 7 is the PG10-NRST dual-function reset pad. HSE 16MHz. USART2 4Mbps RTS/CTS. SWD only; USB pins are CAN1.')
    nets = {str(n):v for n,v in {1:'3V3',5:'HSE_IN',6:'HSE_OUT',7:'SYS_RESET_N',8:'ESP_RTS',9:'STM_RTS_SRC',10:'STM_TX_SRC',11:'ESP_TX',12:'STB1_REQ',13:'STB2_REQ',14:'FT_EN_REQ',15:'STM_ARM_EDGE',17:'WD_PULSE',18:'GATE_SENSE',19:'AUTO_GOOD_MCU',20:'VDDA',21:'VDDA',23:'3V3',25:'CAN2_RX',26:'CAN2_TX_REQ',27:'FT_ERR_N',30:'CAN3_RX',33:'CAN1_RX',34:'CAN1_TX_REQ',35:'3V3',36:'SWDIO',37:'SWCLK',38:'CAN3_TX_REQ',46:'BOOT0',48:'3V3',49:'GND'}.items()}
    c.from_library(s,'U10','STM32G474CEU6','Package_DFN_QFN:QFN-48-1EP_7x7mm_P0.5mm_EP5.6x5.6mm','MCU_ST_STM32G4','STM32G474CEUx',nets,pin_name_overrides={'7':'PG10-NRST'},source='stm32g474: UFQFPN48 pinout, AF9/AF11/AF7; pin 7 PG10-NRST reset function; stm32g4-hardware')
    for _ in range(3): c.c(s,'100n','3V3',size='0402',note='One per VDD pin: 23,35,48; shortest return to EP49')
    c.c(s,'4.7u','3V3');c.c(s,'100n','3V3',size='0402',note='VBAT pin1')
    c.r(s,'0','3V3','VDDA',note='Do not substitute unmodelled high-Q ferrite')
    c.c(s,'1u','VDDA');c.c(s,'100n','VDDA',size='0402')
    c.r(s,'10k','BOOT0','GND');c.r(s,'10k','STM_ARM_EDGE','GND');c.r(s,'10k','WD_PULSE','GND')
    for n in ['CAN1_TX_REQ','CAN2_TX_REQ','CAN3_TX_REQ','STB1_REQ','STB2_REQ']:
        c.r(s,'10k','3V3',n)
    c.r(s,'10k','FT_EN_REQ','GND')
    s = c.sheet('05_debug_clock_uart', '16MHz crystal CL18pF: 27pF + estimated 4.5pF parasitic; tune after PCB. SWD VTref is OUTPUT ONLY.')
    c.ic(s,'Y1','ABM8-16.000MHZ-D2-T','Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm', ['XI','GND','XO','GND'],{'1':'HSE_IN','2':'GND','3':'HSE_DRV','4':'GND'},spec='16MHz; CL18pF; ±20ppm; ±50ppm temperature; -40..85C; ESR70ohm maximum; drive10..100uW',source='abracon-abm8: pp1-2, D temperature option')
    c.r(s,'0','HSE_OUT','HSE_DRV',note='HSE drive-limiting series R footprint; measure crystal drive and startup margin before release')
    for net in ['HSE_IN','HSE_DRV']: c.c(s,'27p',net,voltage='50V',dielectric='C0G',tolerance='5%',size='0402')
    c.ic(s,'J10','SWD 2x5 1.27mm','Connector_PinHeader_1.27mm:PinHeader_2x05_P1.27mm_Vertical',
         ['VTref_OUT','SWDIO','GND','SWCLK','GND','SWO_NC','KEY_NC','NC','GND','NRST'],{'1':'3V3','2':'SWDIO','3':'GND','4':'SWCLK','5':'GND','9':'GND','10':'SYS_RESET_N'},spec='Unshrouded header; matching keyed adapter or controlled pin1 orientation required; VTref sense only',note='Use pin1 marking; SWO not routed; do not power from debugger')
    for src,dst in [('STM_TX_SRC','STM_TX'),('STM_RTS_SRC','STM_RTS'),('ESP_TX_SRC','ESP_TX'),('ESP_RTS_SRC','ESP_RTS')]: c.r(s,'33',src,dst, note='Place next to the named transmitter')
    for n in ['STM_RTS','ESP_RTS']:c.r(s,'10k','3V3',n)
    for n in ['STM_TX','ESP_TX']:c.r(s,'47k','3V3',n)


def esp32(c, bridge=False):
    module='ESP32-S3-WROOM-1' if bridge else 'ESP32-S3-MINI-1'
    s = c.sheet('06_esp32' if not bridge else '02_esp32', module + '. Keep antenna clearance. USB native serial/JTAG. PSRAM pins reserved.')
    from kicad_model import library_pins
    pins=library_pins('RF_Module',module)
    nets={n:'GND' for n,(name,t) in pins.items() if name=='GND'}
    if not bridge:
        nets.update({'3':'3V3','4':'ESP_BOOT_N','45':'SYS_RESET_N','23':'USB_DM','24':'USB_DP'})
        nets.update({'19':'ESP_RTS_SRC','20':'STM_RTS','21':'ESP_TX_SRC','22':'STM_TX',
                     '8':'GPS_TX_SELECT','9':'GPS_RX_TAP','10':'GPS_PPS',
                     '14':'IMU_SCLK','15':'IMU_MOSI',
                     '16':'IMU_MISO','17':'IMU_CS_N','18':'IMU_DRDY','25':'IMU_RESET_N',
                     '27':'GPS_PWR_REQ','28':'USB_SERVICE_SENSE'})
    else:
        nets.update({'2':'3V3','3':'SYS_RESET_N','27':'ESP_BOOT_N','13':'USB_DM','14':'USB_DP',
                     '4':'PAIR_BUTTON_N','5':'STATUS_LED'})
    c.from_library(s,'U11',module+('-N8R2' if bridge else '-N4R2'),
                   'RF_Module:ESP32-S3-WROOM-1' if bridge else 'CANView:ESP32-S3-MINI-1',
                   'RF_Module',module,nets,source='esp32s3-wroom' if bridge else 'esp32s3-mini',
                   spec=('8MB' if bridge else '4MB')+' flash + 2MB PSRAM; 3.0..3.6V; -40..85C')
    for val in ['100n','10u','10u']:c.c(s,val,'3V3')
    c.r(s,'10k','3V3','ESP_BOOT_N')
    s=c.sheet('07_esp_service' if not bridge else '03_service', 'BOOT button for ROM download. Reset supervisor is shared on Communicator. Keep GPIO0 released for normal boot.')
    c.two(s,'SW','BOOT','ESP_BOOT_N','GND',fp='Button_Switch_SMD:SW_SPST_TL3342',mpn='TL3342F160QG',spec='Momentary normally open; logic signal only')
    c.r(s,'22','USB_DM_CONN','USB_DM');c.r(s,'22','USB_DP_CONN','USB_DP')
    if bridge:
        c.r(s,'10k','3V3','PAIR_BUTTON_N')
        c.two(s,'SW','PAIR','PAIR_BUTTON_N','GND',fp='Button_Switch_SMD:SW_SPST_TL3342',mpn='TL3342F160QG',spec='Momentary normally open; hold >=3s commissioning')
        c.r(s,'2.2k','STATUS_LED','LED_A')
        c.ic(s,'D3','BLUE','LED_SMD:LED_0603_1608Metric', ['K','A'],{'1':'GND','2':'LED_A'},mpn='GEN-LED-0603-BLUE',spec='Blue LED; pad1 cathode, pad2 anode; If<=1mA; Vf<=3.2V')


def can_phys(c):
    s=c.sheet('08_can_fd','TCAN1046AV: actual pin11=VIO, pin5=GND2, pin7=RXD2, pin8=STB2. CAN PHY power is vehicle-only.')
    names=['TXD1','GND1','VCC','RXD1','GND2','TXD2','RXD2','STB2','CANL2','CANH2','VIO','CANL1','CANH1','STB1']
    nets=['CAN1_TXD','GND','AUTO5V','CAN1_RX_PHY','GND','CAN2_TXD','CAN2_RX_PHY','CAN2_STB','CAN2_L','CAN2_H','PHY3V3','CAN1_L','CAN1_H','CAN1_STB']
    c.ic(s,'U20','TCAN1046AVDYYRQ1','CANView:TI_DYY0014A',names,{str(i+1):n for i,n in enumerate(nets)},types={'1':'input','2':'power_in','3':'power_in','4':'output','5':'power_in','6':'input','7':'output','8':'input','9':'bidirectional','10':'bidirectional','11':'power_in','12':'bidirectional','13':'bidirectional','14':'input'},source='tcan1046av: pp4-5, operating modes, DYY0014A land pattern',spec='Dual CAN FD; 4.5..5.5V VCC; 3.3V VIO; bus fault ±58V')
    for net in ['AUTO5V','PHY3V3']:c.c(s,'100n',net,size='0402')
    c.c(s,'4.7u','AUTO5V')
    for i in [1,2]:
        c.r(s,'10k','PHY3V3',f'CAN{i}_TXD');c.r(s,'10k','PHY3V3',f'CAN{i}_STB')
    c.ic(s,'U21','SN74LVC2G17DCKR',SC6,['1A','GND','2A','2Y','VCC','1Y'],{'1':'CAN1_RX_PHY','2':'GND','3':'CAN2_RX_PHY','4':'CAN2_RX','5':'3V3','6':'CAN1_RX'},types={'1':'input','2':'power_in','3':'input','4':'output','5':'power_in','6':'output'},source='sn74lvc2g17: DCK pins, Ioff, VIH/VIL',spec='5.5V-tolerant inputs; partial-power-down Ioff; powered from MCU rail')
    c.c(s,'100n','3V3',size='0402')


def can_ft(c):
    s=c.sheet('09_can_ft','MAX3055 only for verified ISO11898-3 125kbps bus. RTH/RTL 4.7k distributed termination; no CANH-CANL 120ohm.')
    names=['INH','TXD','RXD','ERR_N','STB','EN','WAKE','RTH','RTL','VCC','CANH','CANL','GND','BATT']
    nets={str(i+1):n for i,n in enumerate(['FT_INH_MON','CAN3_TXD','CAN3_RX_PHY','FT_ERR_PHY','AUTO5V','CAN3_EN','FT_WAKE','FT_RTH','FT_RTL','AUTO5V','CAN3_H','CAN3_L','GND','PROTECTED_VBAT'])}
    c.ic(s,'U22','MAX3055ASD+','Package_SO:SOIC-14_3.9x8.7mm_P1.27mm',names,nets,types={'1':'output','2':'input','3':'output','4':'output','5':'input','6':'input','7':'input','8':'passive','9':'passive','10':'power_in','11':'bidirectional','12':'bidirectional','13':'power_in','14':'power_in'},source='max3055: pp1,11,16-17',spec='125kbps FT CAN; VCC4.75..5.25V; BATT>=5V for valid operation; bus fault±80V')
    c.c(s,'100n','AUTO5V',size='0402');c.c(s,'100n','PROTECTED_VBAT',voltage='50V');c.c(s,'4.7u','AUTO5V')
    c.r(s,'10k','PROTECTED_VBAT','FT_WAKE',voltage='50V',size='0603')
    c.r(s,'220k','FT_INH_MON','GND',voltage='50V',size='0603',note='Defined load <0.18mA at32V; INH never controls regulator')
    c.r(s,'4.7k','FT_RTH','CAN3_H',power='0.25W',voltage='100V',size='0805')
    c.r(s,'4.7k','FT_RTL','CAN3_L',power='0.25W',voltage='100V',size='0805')
    c.r(s,'10k','AUTO5V','CAN3_TXD');c.r(s,'10k','CAN3_EN','GND')
    c.ic(s,'U23','SN74LVC2G17DCKR',SC6,['1A','GND','2A','2Y','VCC','1Y'],{'1':'CAN3_RX_PHY','2':'GND','3':'FT_ERR_PHY','4':'FT_ERR_N','5':'3V3','6':'CAN3_RX'},types={'1':'input','2':'power_in','3':'input','4':'output','5':'power_in','6':'output'},source='sn74lvc2g17',spec='5.5V-tolerant inputs, Ioff; do not replace with resistor divider')
    c.c(s,'100n','3V3',size='0402')


def can_ports(c):
    s=c.sheet('14_can_connectors','BOARD pinout only, NOT a Tucson harness assignment. CAN1/2 split termination DNP for vehicle stub. CAN3 uses RTH/RTL.')
    c.ic(s,'J20','CAN BUS 1/2/3','Connector_JST:JST_GH_SM08B-GHS-TB_1x08-1MP_P1.25mm_Horizontal',
         ['CAN1_H','CAN1_L','CAN2_H','CAN2_L','CAN3_H','CAN3_L','GND','GND'],
         {'1':'CAN1_H','2':'CAN1_L','3':'CAN2_H','4':'CAN2_L','5':'CAN3_H','6':'CAN3_L','7':'GND','8':'GND'},
         mpn='SM08B-GHS-TB(LF)(SN)',spec='JST GH8 keyed 1.25mm; twisted pair1-2/3-4/5-6; signal only; cable strain relief required',source='JST GH series drawing in installed KiCad footprint')
    for i in [1,2,3]:
        c.ic(s,f'D{20+i}','ESD2CAN24DBZRQ1','Package_TO_SOT_SMD:SOT-23',['IO1','IO2','GND'],{'1':f'CAN{i}_H','2':f'CAN{i}_L','3':'GND'},source='esd2can24: p3 pinout; application and layout',spec='24V stand-off bidirectional; ~3pF; protect at connector; NOT sustained battery-short limiter')
    for i in [1,2]:
        c.r(s,'60.4',f'CAN{i}_H',f'CAN{i}_TERM',size='0805',voltage='100V',power='0.25W',dnp=True)
        c.r(s,'60.4',f'CAN{i}_L',f'CAN{i}_TERM',size='0805',voltage='100V',power='0.25W',dnp=True)
        c.c(s,'4.7n',f'CAN{i}_TERM',voltage='100V',dielectric='C0G',tolerance='5%',size='0805',dnp=True)
def tx_gate(c):
    s=c.sheet('10_tx_gate','Independent TX gate: no permit => tri-state TXD + PHY rail pull-ups. Ioff input buffers prevent USB-only backpower.')
    names=['1OE_N','1A','2Y','GND','2A','1Y','2OE_N','VCC']
    types={'1':'input','2':'input','3':'tri_state','4':'power_in','5':'input','6':'tri_state','7':'input','8':'power_in'}
    c.ic(s,'U24','SN74LVC2G125DCUR',VSS8,names,{'1':'TX_OE_N','2':'CAN1_TX_REQ','3':'CAN2_TXD','4':'GND','5':'CAN2_TX_REQ','6':'CAN1_TXD','7':'TX_OE_N','8':'PHY3V3'},types=types,source='sn74lvc2g125: p4, Ioff',spec='Dual independent tristate buffers; 3.3V PHY-domain supply')
    # FT logic outputs track the SAME rail as MAX3055 VCC. A separate3.3V
    # reservoir must not hold its logic pins above VCC+0.3V on5V collapse.
    for ref,oe,inp,out in [('U28','TX_OE_N','CAN3_TX_REQ','CAN3_TXD'),('U29','FT_RX_OE_N','FT_EN_REQ','CAN3_EN')]:
        c.ic(s,ref,'SN74LV1T125DBVR',SOT5,['OE_N','A','GND','Y','VCC'],{'1':oe,'2':inp,'3':'GND','4':out,'5':'AUTO5V'},types={'1':'input','2':'input','3':'power_in','4':'tri_state','5':'power_in'},source='sn74lv1t125:pp3,5-6; input tolerance independent of supply, VIH<=2.11V at5.5V',spec='3.3V input translated to MAX3055 own5V rail; do not claim general output Ioff isolation')
        c.c(s,'100n','AUTO5V',size='0402')
    c.r(s,'10k','PHY3V3','TX_OE_N');c.r(s,'10k','PHY3V3','FT_RX_OE_N')
    c.c(s,'100n','PHY3V3',size='0402')
    # STB requests retain existing PA4/PA5 active-high-standby semantics.
    s=c.sheet('11_phy_modes','RX mode is independent of TX permit. On invalid automotive power all PHY enables return to hardware standby.')
    c.ic(s,'U26','SN74LVC2G125DCUR',VSS8,names,{'1':'FT_RX_OE_N','2':'STB1_REQ','3':'CAN2_STB','4':'GND','5':'STB2_REQ','6':'CAN1_STB','7':'FT_RX_OE_N','8':'PHY3V3'},types=types,source='sn74lvc2g125',spec='Ioff inputs; pulls on PHY side enforce standby')
    gate3(c,s,'U36','AUTO_GOOD','SYS_RESET_N','PHY_RESET_N','RX_ALLOWED')
    inv(c,s,'U27','RX_ALLOWED','FT_RX_OE_N')
    c.c(s,'100n','PHY3V3',size='0402')
    s=c.sheet('12_watchdog_latch','120pF gives64.288ms nominal watchdog; budget<72ms with5% C and10pF stray. WDO recovery CANNOT re-arm TX; new ARM edge required.')
    c.ic(s,'U30','TPS3431SDRBR','CANView:TI_DRB0008A',
         ['VDD','CWD','EN','GND','SET1','WDI','WDO_N','ENOUT','EP'],
         {'1':'PHY3V3','2':'WDT_C','3':'PHY3V3','4':'GND','5':'PHY3V3','6':'WD_PULSE','7':'WD_OK_N','9':'GND'},
         types={'1':'power_in','2':'passive','3':'input','4':'power_in','5':'input','6':'input','7':'open_collector','8':'open_collector','9':'power_in'},source='tps3431: pp3,6,9-11',spec='External missing-pulse watchdog; falling-edge WDI; firmware scheduler heartbeat, never autonomous timer')
    c.c(s,'120p','WDT_C',dielectric='C0G',tolerance='5%',size='0402');c.c(s,'100n','PHY3V3',size='0402');c.r(s,'10k','PHY3V3','WD_OK_N')
    gate3(c,s,'U31','WD_OK_N','SYS_RESET_N','AUTO_GOOD','ARM_HEALTH_N')
    s=c.sheet('12b_latch_clear','PHY-domain POR and physical disarm both CLEAR the latch, including USB-first automotive hot-plug. No stale Q reuse.')
    gate3(c,s,'U37','ARM_HEALTH_N','PHY_RESET_N','TX_ARM','ARM_CLEAR_N')
    c.ic(s,'U32','SN74LVC1G74DCUR',VSS8,['CLK','D','Q_N','GND','Q','CLR_N','PRE_N','VCC'],{'1':'STM_ARM_EDGE','2':'PHY3V3','5':'ARM_LATCH','4':'GND','6':'ARM_CLEAR_N','7':'PHY3V3','8':'PHY3V3'},types={'1':'input','2':'input','3':'output','4':'power_in','5':'output','6':'input','7':'input','8':'power_in'},source='sn74lvc1g74: p4 pin functions (Q=5, inverted Q=3) and asynchronous clear truth table',spec='Latched disarm after watchdog or power failure; no automatic re-arm')
    c.c(s,'100n','PHY3V3',size='0402')
    s=c.sheet('13_arm_switch','Physical arm link is OPEN by default. Software cannot bypass the link. Gate sense is buffered into MCU power domain.')
    c.ic(s,'J30','TX_ARM normally OPEN','Connector_PinHeader_1.27mm:PinHeader_1x02_P1.27mm_Vertical',['PHY3V3','TX_ARM'],{'1':'PHY3V3','2':'TX_ARM'},spec='Removable shunt; default absent. Installer-controlled TX arm, not user UI setting')
    c.r(s,'10k','TX_ARM','GND')
    c.r(s,'100k','ARM_LATCH','GND')
    gate3(c,s,'U33','ARM_LATCH','ARM_CLEAR_N','RX_ALLOWED','TX_PERMIT')
    inv(c,s,'U34','TX_PERMIT','TX_OE_N')
    c.ic(s,'U35','SN74LVC2G17DCKR',SC6,['1A','GND','2A','2Y','VCC','1Y'],{'1':'TX_PERMIT','2':'GND','3':'AUTO_GOOD','4':'AUTO_GOOD_MCU','5':'3V3','6':'GATE_SENSE'},types={'1':'input','2':'power_in','3':'input','4':'output','5':'power_in','6':'output'},source='sn74lvc2g17',spec='Ioff supply-domain sense isolation')
    c.c(s,'100n','3V3',size='0402')


def gate3(c,s,ref,a,b,d,out):
    c.ic(s,ref,'SN74LVC1G11DCKR',SC6,['A','GND','B','Y','VCC','C'],{'1':a,'2':'GND','3':b,'4':out,'5':'PHY3V3','6':d},types={'1':'input','2':'power_in','3':'input','4':'output','5':'power_in','6':'input'},source='sn74lvc1g11: DCK pin functions',spec='3-input AND; Ioff support')
    c.c(s,'100n','PHY3V3',size='0402')


def inv(c,s,ref,inp,out):
    c.ic(s,ref,'SN74LVC1G04DBVR',SOT5,['NC','A','GND','Y','VCC'],{'2':inp,'3':'GND','4':out,'5':'PHY3V3'},types={'2':'input','3':'power_in','4':'output','5':'power_in'},source='sn74lvc1g04: DBV pin functions',spec='Ioff support')
    c.c(s,'100n','PHY3V3',size='0402')
