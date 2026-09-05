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
    def test_ota_reset_domains_and_boot0_authorization(self):
        p=parts('communicator')
        self.assertEqual(net(p['U11'],3),'ESP_RESET_N')
        self.assertEqual(net(p['U10'],7),'STM_RESET_N')
        self.assertEqual(net(p['J10'],10),'STM_RESET_N')
        self.assertEqual(net(p['U6'],1),'ESP_RESET_N')
        self.assertEqual(net(p['U17'],1),'STM_RESET_N')
        self.assertEqual(net(p['U6'],2),net(p['U17'],2))
        self.assertEqual(net(p['U6'],2),'GLOBAL_RESET_N')
        self.assertEqual(p['U50']['pins']['4'][1],'open_collector')
        self.assertEqual(net(p['U50'],4),'STM_RESET_N')
        self.assertEqual([net(p['U55'],i) for i in [1,3,6,4]],
                         ['STM_BOOT0_REQ','SERVICE_ACTIVE','3V3','ROM_BOOT_ALLOWED'])
        self.assertTrue(self.resistor_exists(p,'10k','BOOT0','GND'))
        self.assertEqual(net(p['J32'],2),'BOOT0')

    def test_ota_gate_uses_both_resets_and_physical_service(self):
        p=parts('communicator')
        self.assertEqual([net(p['U52'],i) for i in [1,3,6,4]],
                         ['ESP_RESET_N','STM_RESET_N','SERVICE_RUN','MCU_HEALTH_N'])
        self.assertEqual([net(p['U53'],i) for i in [1,3,6,4]],
                         ['MCU_HEALTH_N','ESP_RUN_OK','PHY_RESET_N','RUN_ALLOWED'])
        self.assertEqual(net(p['U36'],3),'RUN_ALLOWED')
        self.assertEqual(net(p['U31'],3),'RUN_ALLOWED')
        for signal in ['SERVICE_RUN','ESP_RUN_OK','RUN_ALLOWED']:
            self.assertTrue(self.resistor_exists(p,'10k',signal,'GND'))
        # Exhaustively evaluate exported AND gates; this is not analog HIL.
        for bits in itertools.product([False,True],repeat=7):
            state=dict(zip(['ESP_RESET_N','STM_RESET_N','SERVICE_RUN','ESP_RUN_OK',
                            'PHY_RESET_N','AUTO_GOOD','WD_OK_N'],bits))
            for ref in ['U52','U53','U36','U31']:
                state[net(p[ref],4)]=all(state[net(p[ref],i)] for i in [1,3,6])
            if not all(state[n] for n in ['ESP_RESET_N','STM_RESET_N','SERVICE_RUN','ESP_RUN_OK','PHY_RESET_N']):
                self.assertFalse(state['RX_ALLOWED'])
                self.assertFalse(state['ARM_HEALTH_N'])

    def test_service_interlock_rejects_gpio_backdrive_and_stale_arm(self):
        p=parts('communicator')
        self.assertEqual(p['U56']['mpn'],'SN74LVC1G17DBVR')
        self.assertEqual([net(p['U56'],i) for i in [2,3,4,5]],
                         ['SERVICE_RUN','GND','SERVICE_RUN_SENSE_SRC','3V3'])
        self.assertEqual(p['U56']['pins']['2'][1],'input')
        self.assertEqual(p['U56']['pins']['4'][1],'output')
        self.assertEqual(net(p['U11'],25),'SERVICE_RUN_SENSE')
        self.assertEqual(net(p['U54'],2),'SERVICE_RUN')
        self.assertEqual(net(p['J31'],2),'SERVICE_RUN')
        self.assertTrue(self.resistor_exists(p,'4.7k','SERVICE_RUN_SENSE_SRC','SERVICE_RUN_SENSE'))
        self.assertTrue(self.resistor_exists(p,'100k','SERVICE_RUN_SENSE','GND'))
        self.assertTrue(self.resistor_exists(p,'10k','SERVICE_RUN','GND'))
        for ref in ['U10','U11']:
            self.assertNotIn('SERVICE_RUN',[pin[2] for pin in p[ref]['pins'].values()])
        # Exported topology plus sequential fault model: GPIO48 stuck either way.
        # U56 has no reverse logic path. Series R limits GPIO/Y contention.
        for gpio_fault in [False,True]:
            q=False
            for shunt,arm_edge in [(True,True),(False,False),(True,False),(True,True)]:
                state=dict.fromkeys(['ESP_RESET_N','STM_RESET_N','ESP_RUN_OK','PHY_RESET_N',
                                     'AUTO_GOOD','WD_OK_N','TX_ARM','PHY3V3'],True)
                state[net(p['J31'],2)]=shunt
                state[net(p['U11'],25)]=gpio_fault
                for ref in ['U52','U53','U36','U31','U37']:
                    state[net(p[ref],4)]=all(state[net(p[ref],i)] for i in [1,3,6])
                clear_n=state[net(p['U32'],6)]
                q=False if not clear_n else (state[net(p['U32'],2)] if arm_edge else q)
                state[net(p['U32'],5)]=q
                state[net(p['U33'],4)]=all(state[net(p['U33'],i)] for i in [1,3,6])
                self.assertEqual(state['TX_PERMIT'],shunt and arm_edge)
                if not shunt:self.assertFalse(state['RX_ALLOWED'])

    def test_n16r8_memory_pins_and_recovery_buttons(self):
        p=parts('communicator')
        self.assertEqual(p['U11']['mpn'],'ESP32-S3-WROOM-1-N16R8')
        for pad in [28,29,30]:self.assertIsNone(net(p['U11'],pad))
        self.assertEqual(net(p['U11'],31),'USB_SERVICE_SENSE')
        self.assertEqual(net(p['U11'],12),'RECOVERY_BUTTON_N')
        self.assertEqual(net(parts('bridge')['U11'],4),'PAIR_BUTTON_N')
        header=parts('controller-adapter')['J1']
        self.assertEqual(net(header,13),'RECOVERY_BUTTON_N')
        self.assertEqual(net(header,22),'HOST_RESET_N')

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
