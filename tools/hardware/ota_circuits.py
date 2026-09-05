"""OTA recovery wiring; application firmware keeps ESP_RUN_OK LOW in service."""
from core_circuits import gate3, inv, SOT5
from power_circuits import supervisor


def communicator(c):
    s=c.sheet('21_ota_reset','Separate reset outputs: U6 ESP, U17 STM. Common manual MR resets both; STM SWD/internal reset never resets ESP updater.')
    c.r(s,'10k','3V3','GLOBAL_RESET_N')
    supervisor(c,s,'U17','3V3','STM_RESET_N','GLOBAL_RESET_N')
    c.ic(s,'U50','SN74LVC1G07DBVR',SOT5,['NC','A','GND','Y','VCC'],
         {'2':'STM_RESET_CMD_N','3':'GND','4':'STM_RESET_N','5':'3V3'},
         types={'2':'input','3':'power_in','4':'open_collector','5':'power_in'},
         source='https://www.ti.com/lit/ds/symlink/sn74lvc1g07.pdf: RevAG pp3,6,9; DBV pin2 A,4 open-drain Y',
         spec='Non-inverting open-drain reset buffer; Ioff; GPIO1 LOW asserts STM reset, HIGH releases')
    c.c(s,'100n','3V3',size='0402')
    c.r(s,'10k','3V3','STM_RESET_CMD_N')
    c.r(s,'10k','3V3','STM_RECOVERY_N')
    s=c.sheet('22_ota_interlock','Remove SERVICE_RUN shunt for OTA. Open/missing shunt disables all PHY modes even if firmware requests RUN. Refitting never restores the cleared ARM latch.')
    c.ic(s,'J31','RUN link / REMOVE FOR OTA','Connector_PinHeader_1.27mm:PinHeader_1x02_P1.27mm_Vertical',
         ['3V3','SERVICE_RUN'],{'1':'3V3','2':'SERVICE_RUN'},
         spec='Accessible removable shunt; absent=service; fit only for normal operation; independent of TX_ARM J30')
    c.r(s,'10k','SERVICE_RUN','GND')
    c.r(s,'10k','ESP_RUN_OK','GND')
    gate3(c,s,'U52','ESP_RESET_N','STM_RESET_N','SERVICE_RUN','MCU_HEALTH_N')
    gate3(c,s,'U53','MCU_HEALTH_N','ESP_RUN_OK','PHY_RESET_N','RUN_ALLOWED')
    c.r(s,'10k','RUN_ALLOWED','GND')
    s=c.sheet('23_ota_recovery','RECOVERY GPIO8 held at reset selects signed ESP test app. PB9 LOW requests authenticated STM bootloader with BOOT0 LOW. J32 normally OPEN restricts ROM to physical service.')
    c.r(s,'10k','3V3','RECOVERY_BUTTON_N')
    c.two(s,'SW','RECOVERY','RECOVERY_BUTTON_N','GND',
          fp='Button_Switch_SMD:SW_SPST_TL3342',mpn='TL3342F160QG',spec='Normally open; hold >=5s across reset; not ESP GPIO0')
    c.r(s,'10k','STM_BOOT0_REQ','GND')
    inv(c,s,'U54','SERVICE_RUN','SERVICE_ACTIVE',rail='3V3')
    gate3(c,s,'U55','STM_BOOT0_REQ','SERVICE_ACTIVE','3V3','ROM_BOOT_ALLOWED',rail='3V3')
    c.r(s,'1k','ROM_BOOT_ALLOWED','ROM_BOOT_LINK')
    c.ic(s,'J32','ROM_BOOT authorization / OPEN','Connector_PinHeader_1.27mm:PinHeader_1x02_P1.27mm_Vertical',
         ['ROM_BOOT_LINK','BOOT0'],{'1':'ROM_BOOT_LINK','2':'BOOT0'},
         spec='Shunt normally absent; factory/physical recovery only; never populated for ordinary OTA; STM BOOT0 has10k pulldown')
    s=c.sheet('24_ota_test_points','Probe independent reset, recovery request, service interlock and BOOT0 during fault injection.')
    for net in ['GLOBAL_RESET_N','ESP_RESET_N','STM_RESET_N','STM_RESET_CMD_N','STM_RECOVERY_N',
                'BOOT0','SERVICE_RUN','ESP_RUN_OK','RUN_ALLOWED']:
        c.ic(s,c.automatic('TP'),net,'TestPoint:TestPoint_Pad_D1.0mm',['PROBE'],{'1':net},
             mpn='PCB-PAD-1MM',spec='Bare1mm probe pad; no purchased component',source='PCB fabrication feature')


def controller(c):
    s=c.sheet('04_ota_recovery','Camera must be disconnected. Header pin13 GPIO41 selects recovery; pin22 RESET resets host. GPIO38..40 microphone remains independent; onboard BOOT GPIO0 is ROM only.')
    c.r(s,'10k','3V3','RECOVERY_BUTTON_N')
    for name,net in [('RECOVERY','RECOVERY_BUTTON_N'),('HOST_RESET','HOST_RESET_N')]:
        c.two(s,'SW',name,net,'GND',fp='Button_Switch_SMD:SW_SPST_TL3342',
              mpn='TL3342F160QG',spec='Normally-open momentary; host reset is sink-only; hold recovery5s across reset')
