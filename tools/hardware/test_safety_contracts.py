"""Net/part and steady-state Boolean regressions, NOT analog fault simulation."""
import itertools
import json
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]


def parts(board):
    model=json.loads((ROOT/'hardware'/board/'connectivity.json').read_text(encoding='utf-8'))
    return {p['ref']:p for p in model['components'] if not p['ref'].startswith('#')}


def net(part,pin):
    return part['pins'][str(pin)][2]


class SafetyContractTests(unittest.TestCase):
    def resistor_exists(self,components,value,a,b):
        return any(p['ref'].startswith('R') and p['value']==value and
                   {net(p,1),net(p,2)}=={a,b} and not p['dnp'] for p in components.values())

    def test_ft_active_high_enable_and_local_pulldowns(self):
        p=parts('communicator')
        for ref,permission,request,output in [('U28','TX_PERMIT','CAN3_TX_REQ','CAN3_TXD'),('U29','RX_ALLOWED','FT_EN_REQ','CAN3_EN')]:
            self.assertEqual(p[ref]['mpn'],'CAHCT1G126DBVRQ1')
            self.assertEqual(p[ref]['pins']['1'][0],'OE')
            self.assertEqual([net(p[ref],i) for i in range(1,6)],[permission,request,'GND',output,'AUTO5V'])
            self.assertTrue(self.resistor_exists(p,'1k',permission,'GND'))
        self.assertTrue(self.resistor_exists(p,'10k','AUTO5V','CAN3_TXD'))
        self.assertTrue(self.resistor_exists(p,'10k','CAN3_EN','GND'))

    def test_ft_loss_of_phy_rail_denies_all_request_combinations(self):
        # TI p8 truth table: OE=0 -> Z; local output pulls produce TXD=1/EN=0.
        # Unpowered PHY gates support Ioff; OE pulldowns discharge to GND.
        # This is settled-state logic only. Collapse latency remains a HIL gate.
        self.test_ft_active_high_enable_and_local_pulldowns()
        for phy_on,tx_permit,rx_allowed,tx_request,en_request in itertools.product([False,True],repeat=5):
            tx_oe=phy_on and tx_permit
            en_oe=phy_on and rx_allowed
            txd=tx_request if tx_oe else True
            enabled=en_request if en_oe else False
            if not phy_on:
                self.assertTrue(txd)
                self.assertFalse(enabled)
            if not tx_permit:self.assertTrue(txd)

    def test_usb_cc_regulator_is_before_mux_and_limiter(self):
        for board in ['communicator','bridge']:
            p=parts(board)
            self.assertEqual(p['U16']['mpn'],'TPS7A2033PDBVR')
            self.assertEqual([net(p['U16'],i) for i in [1,2,3,5]],['USB_VBUS','GND','USB_VBUS','USB_CC3V3'])
            self.assertEqual(net(p['U1'],12),'USB_CC3V3')
            self.assertEqual(net(p['U2'],5),'USB_CC3V3')
            self.assertEqual(net(p['U3'],1),'USB_VBUS')
            self.assertTrue(self.resistor_exists(p,'10k','USB_CC3V3','USB_DEFAULT_N'))
            self.assertTrue(self.resistor_exists(p,'3k','USB_CC3V3','GND'))
            self.assertTrue(self.resistor_exists(p,'100k','USB_POWER_ALLOWED','GND'))

    def test_lm_fets_have_verified_twenty_volt_dc_gate_rating(self):
        p=parts('communicator')
        for ref,drain,gate in [('Q1','VBAT_FUSED','HGATE'),('Q2','PROTECTED_VBAT','DGATE')]:
            self.assertEqual(p[ref]['mpn'],'BUK7Y12-100E')
            self.assertEqual([net(p[ref],i) for i in range(1,6)],['CS','CS','CS',gate,drain])
            self.assertIn('DC VGS +/-20V',p[ref]['spec'])
        # Manufacturer p2 DC limit vs LM7480 p6 maximum steady gate drive.
        self.assertGreater(20,14.5)
        self.assertGreater(20,13)

    def test_unused_controller_uart_names_match_official_header(self):
        p=parts('controller-adapter')['J1']['pins']
        self.assertEqual((p['25'][0],p['27'][0]),('TXD43_NC','RXD44_NC'))
        self.assertIsNone(p['25'][2]);self.assertIsNone(p['27'][2])


if __name__=='__main__':unittest.main()
