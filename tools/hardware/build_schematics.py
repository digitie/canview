"""Generate deterministic KiCad source files; use export-review.ps1 for exports."""
from kicad_model import Circuit, LIB, generate
import core_circuits
import custom_footprints
import sensor_circuits
import power_circuits
import ota_circuits


def main():
    custom_footprints.create()
    c=Circuit('communicator')
    power_circuits.vehicle_front(c)
    power_circuits.auto_converter(c)
    power_circuits.usb_input(c)
    power_circuits.system_supply(c,True)
    power_circuits.rail_qualification(c)
    core_circuits.stm32(c)
    core_circuits.esp32(c)
    core_circuits.can_phys(c)
    core_circuits.can_ft(c)
    core_circuits.tx_gate(c)
    core_circuits.can_ports(c)
    sensor_circuits.navigation(c)
    s=c.sheet('20_test_points','1mm probe pads: no fitted connector required. Scope grounds first; never connect earth-referenced probes to an unknown vehicle ground.')
    for net in ['GND','PROTECTED_VBAT','AUTO5V','SYS5V','3V3','PHY3V3','AUTO_GOOD','ESP_RESET_N','PHY_RESET_N','WD_PULSE','TX_PERMIT','CAN1_TXD','CAN2_TXD','CAN3_TXD','CAN3_EN','CAN1_STB','CAN2_STB','CAN1_RX','CAN2_RX','CAN3_RX','STM_TX_SRC','ESP_TX','STM_RTS_SRC','ESP_RTS']:
        c.ic(s,c.automatic('TP'),net,'TestPoint:TestPoint_Pad_D1.0mm', ['PROBE'], {'1':net},
             mpn='PCB-PAD-1MM',spec='Bare1mm probe pad; no purchased component; accessible with insulated probe',source='PCB fabrication feature')
    c.power_flag('04_stm32','VDDA','3V3 through0ohm analog supply link')
    c.power_flag('15_gnss_ins','IMU_VDDA','3V3 through ferrite74279279')
    ota_circuits.communicator(c)
    symbols=generate(c)
    print(f'{c.name}: {len(c.parts)} components, {len(c.sheets)} sheets')
    b=Circuit('bridge')
    power_circuits.usb_input(b)
    power_circuits.system_supply(b,False)
    core_circuits.esp32(b,True)
    symbols.extend(generate(b))
    print(f'{b.name}: {len(b.parts)} components, {len(b.sheets)} sheets')
    h=Circuit('controller-adapter')
    sensor_circuits.microphone_host(h)
    ota_circuits.controller(h)
    h.power_flag('03_mic_cable','MIC5V','Waveshare5V output through cable fuse')
    symbols.extend(generate(h))
    print(f'{h.name}: {len(h.parts)} components, {len(h.sheets)} sheets')
    m=Circuit('microphone')
    sensor_circuits.microphone_remote(m)
    m.power_flag('01_receiver_power','MIC5V','Cable5V from fused controller adapter')
    m.power_flag('01_receiver_power','GND','Cable ground conductor')
    symbols.extend(generate(m))
    print(f'{m.name}: {len(m.parts)} components, {len(m.sheets)} sheets')
    LIB.mkdir(parents=True,exist_ok=True)
    (LIB/'CANView.kicad_sym').write_text('(kicad_symbol_lib (version 20241209) (generator "kicad_symbol_editor")\n'+'\n'.join(symbols)+'\n)\n',encoding='utf-8')
    print('Generated four review projects; run native KiCad export/ERC next.')


if __name__=='__main__':main()
