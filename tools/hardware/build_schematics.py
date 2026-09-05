"""Generate KiCad source files. Run export_verify.py afterwards using KiCad 10."""
from kicad_model import Circuit, LIB, generate
import core_circuits
import custom_footprints


def main():
    custom_footprints.create()
    c=Circuit('communicator')
    core_circuits.stm32(c)
    core_circuits.esp32(c)
    core_circuits.can_phys(c)
    core_circuits.tx_gate(c)
    core_circuits.can_ports(c)
    symbols=generate(c)
    LIB.mkdir(parents=True,exist_ok=True)
    (LIB/'CANView.kicad_sym').write_text('(kicad_symbol_lib (version 20241209) (generator "kicad_symbol_editor")\n'+'\n'.join(symbols)+'\n)\n',encoding='utf-8')
    print(f'Generated {len(c.parts)} components in {len(c.sheets)} sheets')


if __name__=='__main__':main()
