"""GNSS/INS, altitude and differential remote-I2S microphone circuits."""
from core_circuits import SC6, SOT5, VSS8


def ldo(c,s,ref,inp,out,voltage):
    c.ic(s,ref,f'TPS7A20{voltage}PDBVR',SOT5,['IN','GND','EN','NC','OUT'],
         {'1':inp,'2':'GND','3':inp,'5':out},types={'1':'power_in','2':'power_in','3':'input','5':'power_out'},source='tps7a20: p4 DBV pins, input/output capacitor requirements',spec=f'300mA LDO; fixed {voltage[0]}.{voltage[1]}V; 1uF effective output minimum')
    c.c(s,'1u',inp);c.c(s,'2.2u',out)


def navigation(c):
    s=c.sheet('15_gnss_ins','MTI-7-5A-T v5 module: host SPI mode3 <=2MHz. DNC pads remain NC. GPS/PPS required for qualified GNSS/INS.')
    names=['AUX_SCK','DNC','DNC','GND','VDDA','nRST','VDDIO','GND','SPI_nCS','SPI_MOSI','SPI_MISO','SPI_SCK','GND','PSEL0','PSEL1','SYNC_IN','RESERVED','AUX_RX','AUX_TX','SYNC_PPS','DE_RTS','DRDY_CTS','UART_RX','UART_TX','GND','AUX_nCS','AUX_MOSI','AUX_MISO']
    nets={str(n):v for n,v in {1:'BARO_SCK',4:'GND',5:'IMU_VDDA',6:'IMU_RESET_N',7:'3V3',8:'GND',9:'IMU_CS_N',10:'IMU_MOSI',11:'IMU_MISO',12:'IMU_SCLK',13:'GND',14:'GND',15:'3V3',18:'GPS_RX_TAP',19:'IMU_GPS_TX',20:'GPS_PPS',22:'IMU_DRDY',25:'GND',26:'BARO_CS_N',27:'BARO_MOSI',28:'BARO_MISO'}.items()}
    types={str(n):'power_in' for n in [4,5,7,8,13,25]}
    types.update({str(n):'input' for n in [6,9,10,12,14,15,16,18,20,23,28]})
    types.update({str(n):'output' for n in [1,11,19,21,22,24,26,27]})
    c.ic(s,'U40','MTI-7-5A-T','CANView:Xsens_MTi_1_series',names,nets,types=types,source='xsens-mti1-current: printed pp38-45,78-85; top-view pin1 at middle of north side',spec='v5.0 module; 2.8..3.6V; host MTSSP; GNSS/INS DR output expires after45s GNSS outage')
    c.two(s,'FB','600ohm@100MHz','3V3','IMU_VDDA',fp='Inductor_SMD:L_0402_1005Metric',mpn='74279279',spec='600ohm@100MHz;200mA;DCR<=1ohm;0402,1.0x0.5mm',source='we74279279:p1; xsens current manual single-supply filter')
    c.c(s,'470n','IMU_VDDA',size='0402');c.c(s,'100n','3V3',size='0402');c.c(s,'4.7u','3V3')
    c.r(s,'10k','3V3','IMU_CS_N')
    # Reset must be an open-drain GPIO. No strong external high driver.
    s=c.sheet('16_barometer','BMP384 barometric aiding belongs to MTi AUX SPI. Require MTi firmware >=1.18. ESP obtains pressure through MTi output.')
    names=['VDDIO','SCK','GND','SDI','SDO','CSB','INT','GND','GND','VDD']
    c.ic(s,'U41','BMP384','CANView:Bosch_BMP384_LGA10',names,{'1':'3V3','2':'BARO_SCK','3':'GND','4':'BARO_MOSI','5':'BARO_MISO','6':'BARO_CS_N','8':'GND','9':'GND','10':'3V3'},types={'1':'power_in','2':'input','3':'power_in','4':'input','5':'output','6':'input','7':'output','8':'power_in','9':'power_in','10':'power_in'},source='bmp384: pp45-48; xsens current manual GNSS/barometer interface',spec='300..1250hPa;2x2x0.95mm; port vent to cabin air; no conformal coat over vent')
    c.c(s,'100n','3V3',size='0402');c.c(s,'100n','3V3',size='0402');c.r(s,'10k','3V3','BARO_CS_N')
    s=c.sheet('17_gps_ports','Holybro F9P Helical SKU12018: main UART1 GH10 + UART2 PPS GH6. Receiver pin2=RX, pin3=TX. PPS is secondary pin5.')
    c.ic(s,'J40','GNSS UART1 / GH10','Connector_JST:JST_GH_SM10B-GHS-TB_1x10-1MP_P1.25mm_Horizontal',
         ['5V_OUT','GPS_RX','GPS_TX','SCL_NC','SDA_NC','SW_NC','LED_NC','3V3_NC','BUZZ_NC','GND'],
         {'1':'GPS5V','2':'GPS_RX_CONN','3':'GPS_TX_CONN','10':'GND'},mpn='SM10B-GHS-TB(LF)(SN)',spec='JST GH10,1.25mm,1A; host output supply; match Holybro main UART1 cable',source='holybro-f9p; jst-gh')
    c.ic(s,'J41','GNSS UART2 / PPS ONLY','Connector_JST:JST_GH_SM06B-GHS-TB_1x06-1MP_P1.25mm_Horizontal',
         ['5V_NC','RX2_NC','TX2_NC','NC','PPS_IN','GND'],{'5':'GPS_PPS_CONN','6':'GND'},mpn='SM06B-GHS-TB(LF)(SN)',spec='JST GH6; ONLY PPS5/GND6 connected; no UART2 data assumption',source='holybro-f9p; jst-gh')
    ldo(c,s,'U42','GPS5V','GPS3V3','33')
    c.ic(s,'U43','SN74LVC2G125DCUR',VSS8,['1OE_N','1A','2Y','GND','2A','1Y','2OE_N','VCC'],
         {'1':'GND','2':'GPS_TX_SELECTED','4':'GND','5':'GND','6':'GPS_RX_CONN','7':'GPS3V3','8':'GPS3V3'},
         types={'1':'input','2':'input','3':'tri_state','4':'power_in','5':'input','6':'tri_state','7':'input','8':'power_in'},source='sn74lvc2g125',spec='Powered by switched GNSS supply; Ioff blocks AUX_TX back-power when GPS is off')
    c.c(s,'100n','GPS3V3',size='0402')
    c.r(s,'33','IMU_GPS_TX','GPS_TX_SELECTED',note='FIT by default; mutually exclusive with ESP transmitter link')
    c.r(s,'33','GPS_TX_SELECT','GPS_TX_SELECTED',dnp=True,note='DNP; fit only after removing MTi link. GNSS/INS disabled in ESP-direct variant')
    s=c.sheet('18_gps_protection','Ioff buffers isolate external GPS TX/PPS during local power-off. GPS is disabled in USB service mode.')
    c.ic(s,'U44','SN74LVC2G17DCKR',SC6,['1A','GND','2A','2Y','VCC','1Y'],
         {'1':'GPS_TX_CONN','2':'GND','3':'GPS_PPS_CONN','4':'GPS_PPS','5':'3V3','6':'GPS_RX_TAP'},
         types={'1':'input','2':'power_in','3':'input','4':'output','5':'power_in','6':'output'},source='sn74lvc2g17',spec='5.5V tolerant Ioff; receiver UART must be3.3V TTL, never RS232')
    c.c(s,'100n','3V3',size='0402')
    for net in ['GPS_TX_CONN','GPS_PPS_CONN']:c.r(s,'100k',net,'GND')
    for i,(a,b) in enumerate([('GPS_TX_CONN','GPS_RX_CONN'),('GPS_PPS_CONN','GPS_PPS_CONN')],start=40):
        esd(c,s,f'D{i}',a,b)


def esd(c,s,ref,a,b):
    c.ic(s,ref,'TPD2E2U06DCKR','Package_TO_SOT_SMD:SOT-323_SC-70',['IO1','IO2','GND'],{'1':a,'2':b,'3':'GND'},source='tpd2e2u06:p3 DCK pins1/2=IO,pin3=GND',spec='5.5V standoff;1.5pF typical; ESD only, not DC overvoltage protection')


def lvds(c,s,ref,driver,signal,pair,power='3V3'):
    if driver:
        names=['VCC','GND','Z_N','Y_P','D']
        nets={'1':power,'2':'GND','3':pair+'_N','4':pair+'_P','5':signal}
        types={'1':'power_in','2':'power_in','3':'output','4':'output','5':'input'}
    else:
        names=['VCC','GND','A_P','B_N','R']
        nets={'1':power,'2':'GND','3':pair+'_P','4':pair+'_N','5':signal}
        types={'1':'power_in','2':'power_in','3':'input','4':'input','5':'output'}
        c.r(s,'100',pair+'_P',pair+'_N',note='100ohm receiver-end differential termination; do not add at transmitter')
    c.ic(s,ref,'SN65LVDS1DBVR' if driver else 'SN65LVDS2DBVR',SOT5,names,nets,types=types,source='sn65lvds1: pp3-4 DBV pin functions, timing tables',spec='3.3V LVDS endpoint; 100ohm twisted pair; shared ground required')
    c.c(s,'100n',power,size='0402')


def microphone_connector(c,s,ref):
    c.ic(s,ref,'REMOTE MIC / NOT ETHERNET','Connector_JST:JST_GH_SM08B-GHS-TB_1x08-1MP_P1.25mm_Horizontal',
         ['5V','GND','BCLK_P','BCLK_N','WS_P','WS_N','DATA_P','DATA_N'],
         {'1':'MIC5V','2':'GND','3':'CLK_P','4':'CLK_N','5':'WS_P','6':'WS_N','7':'DATA_P','8':'DATA_N'},mpn='SM08B-GHS-TB(LF)(SN)',source='jst-gh',spec='8-wire4twisted-pairs; keyed GH8 both ends;1:1 cable <=3m design target, SI qualification required')


def microphone_host(c):
    s=c.sheet('01_waveshare_header','Controller adapter ONLY: use onboard RTC0x51. Camera disabled/disconnected. GPIO38=BCLK,39=WS,40=I2S1 DIN.')
    names=['BAT_NC','5V_IN','GND','GND','GPIO21_NC','USB_DM_NC','GPIO38','USB_DP_NC','GPIO39','GPIO11_NC','GPIO40','GPIO10_NC','GPIO41_NC','GPIO9_NC','GPIO42_NC','GPIO17_NC','GPIO45_NC','GPIO18_NC','GPIO46_NC','BOOT_NC','GPIO47_NC','RESET_NC','GPIO48_NC','PWR_NC','TXD43_NC','SCL7_NC','RXD44_NC','SDA8_NC','GND','GND','3V3_IN','3V3_IN']
    c.ic(s,'J1','Waveshare 2x16 mating male','Connector_PinHeader_2.54mm:PinHeader_2x16_P2.54mm_Vertical',names,
         {'2':'HOST5V','3':'GND','4':'GND','7':'MIC_BCLK','9':'MIC_WS','11':'MIC_SD','29':'GND','30':'GND','31':'3V3','32':'3V3'},
         types={'2':'power_out','3':'power_out','31':'power_out'},mpn='GEN-2x16-MALE-2.54',source='waveshare-controller schematic and official STEP HEADER-SMD-F-2_54-2X16PIN',spec='Mates with board female socket;2.54mm pitch verified, insertion height/board-side orientation require mechanical check;5V only when host source provides it')
    c.c(s,'4.7u','HOST5V');c.c(s,'100n','3V3',size='0402')
    s=c.sheet('02_i2s_differential','32kHz / 64fs / 2.048MHz BCLK; two32-bit slots, 24-bit I2S words. Raw I2S is not routed down the long cable.')
    lvds(c,s,'U1',True,'MIC_BCLK','CLK');lvds(c,s,'U2',True,'MIC_WS','WS');lvds(c,s,'U3',False,'MIC_SD','DATA')
    s=c.sheet('03_mic_cable','Four twisted pairs; no RJ45 to avoid Ethernet/PoE misconnection. Power switch/current limit protects host header.')
    microphone_connector(c,s,'J2')
    # A resettable fuse limits wiring-fault energy; exact trip response is checked on bench.
    c.two(s,'F','0.10A PTC','HOST5V','MIC5V',fp='Fuse:Fuse_0805_2012Metric',mpn='MF-PSMF010X-2',spec='15V;Ihold0.1A@23C;Itrip0.3A;R1max7.5ohm;trip<=1.5s@0.5A; -40..85C; not a precision limiter',source='bourns-psmf:pp1-3')
    c.c(s,'1u','MIC5V')
    for i,pair in enumerate(['CLK','WS','DATA'],start=1):esd(c,s,f'D{i}',pair+'_P',pair+'_N')


def microphone_remote(c):
    s=c.sheet('01_receiver_power','Remote microphone board; supply cable5V. Two clock receivers, one data driver. T5848 is 1.8V ONLY.')
    microphone_connector(c,s,'J1')
    ldo(c,s,'U1','MIC5V','3V3','33');ldo(c,s,'U2','3V3','1V8','18')
    for i,pair in enumerate(['CLK','WS','DATA'],start=1):esd(c,s,f'D{i}',pair+'_P',pair+'_N')
    s=c.sheet('02_i2s_link','Receiver terminations on BCLK/WS. Use32kHz/2.048MHz. Cable+logic+SD output delay must meet the next rising-edge setup.')
    lvds(c,s,'U3',False,'BCLK_3V3','CLK');lvds(c,s,'U4',False,'WS_3V3','WS');lvds(c,s,'U5',True,'DATA_3V3','DATA')
    s=c.sheet('03_mems_microphone','T5848 bottom acoustic port. Pin7=VDD (datasheet p35 prose typo). Shield from direct airflow; no PCB copper/paste across hole.')
    c.ic(s,'MK1','MMICT5848-00-012','CANView:TDK_T5848_LGA8',
         ['WS','LR','GND','WAKE','THSEL','SCK','VDD','SD'],{'1':'WS_1V8','2':'GND','3':'GND','6':'BCLK_1V8','7':'1V8','8':'DATA_1V8'},
         types={'1':'input','2':'input','3':'power_in','4':'output','5':'input','6':'input','7':'power_in','8':'output'},source='t5848: pp7-10,30-31,35,37,39',spec='1.62..1.98V; HQ BCLK2.0..3.7MHz; SD valid<=75ns; 24bit I2S with20bit precision')
    c.c(s,'100n','1V8',size='0402');c.r(s,'100k','DATA_1V8','GND')
    for ref,a,b,direction in [('U6','BCLK_1V8','BCLK_3V3','GND'),('U7','WS_1V8','WS_3V3','GND'),('U8','DATA_1V8','DATA_3V3','1V8')]:
        c.ic(s,ref,'SN74AXC1T45DCKR',SC6,['VCCA','GND','A','B','DIR','VCCB'],
             {'1':'1V8','2':'GND','3':a,'4':b,'5':direction,'6':'3V3'},types={'1':'power_in','2':'power_in','3':'bidirectional','4':'bidirectional','5':'input','6':'power_in'},source='sn74axc1t45: p3, direction and VCC isolation',spec='A=1.8V/B=3.3V; fixed direction, no auto-direction level shifter')
        c.c(s,'100n','1V8',size='0402');c.c(s,'100n','3V3',size='0402')
