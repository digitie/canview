"""Vehicle/USB supplies, source qualification and reset-domain circuitry."""
from core_circuits import SOT5, SC6, gate3
from sensor_circuits import ldo, esd

SOT6='Package_TO_SOT_SMD:SOT-23-6'
SOT583='Package_TO_SOT_SMD:SOT-583-8'


def supervisor(c,s,ref,rail,out):
    c.ic(s,ref,'TLV803EA30DPWR','Package_SON:Texas_X2SON-5_0.8x0.8mm_P0.48mm',
         ['RESET_N','MR_N','PAD','GND','VDD'],{'1':out,'2':rail,'3':'GND','4':'GND','5':rail},
         types={'1':'open_collector','2':'input','3':'passive','4':'power_in','5':'power_in'},
         source='tlv803e:pp4-5,9; DPW pin3 PAD may be grounded',spec='3.08V threshold;130..270ms release delay; active-low open-drain')
    c.r(s,'10k',rail,out);c.c(s,'100n',rail,size='0402')


def limit_switch(c,s,ref,inp,out,en,rilim,prefix):
    c.ic(s,ref,'TPS2553QDBVRQ1',SOT6,['IN','GND','EN','FAULT_N','ILIM','OUT'],
         {'1':inp,'2':'GND','3':en,'4':prefix+'_FAULT_N','5':prefix+'_ILIM','6':out},
         types={'1':'power_in','2':'power_in','3':'input','4':'open_collector','5':'passive','6':'power_out'},
         source='tps2553:pp3,13',spec='2.5..6.5V; programmable current limit; thermal shutdown; finite trip response')
    c.r(s,rilim,prefix+'_ILIM','GND');c.r(s,'10k',inp,prefix+'_FAULT_N')
    c.c(s,'1u',inp);c.c(s,'1u',out)


def usb_input(c):
    s=c.sheet('01_usb_c','USB-C sink: requires CC advertisement >=1.5A. Default-current ports DO NOT power the board. No USB PD/high-voltage request.')
    names={n:n for n in ['A1','A4','A5','A6','A7','A8','A9','A12','B1','B4','B5','B6','B7','B8','B9','B12','SH']}
    nets={'A1':'GND','A4':'USB_VBUS','A5':'CC1','A6':'USB_DP_CONN','A7':'USB_DM_CONN','A9':'USB_VBUS','A12':'GND',
          'B1':'GND','B4':'USB_VBUS','B5':'CC2','B6':'USB_DP_CONN','B7':'USB_DM_CONN','B9':'USB_VBUS','B12':'GND','SH':'GND'}
    c.ic(s,'J1','USB4105-GF-A','Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal',names,nets,
         types={'A4':'power_out','A1':'power_out'},source='gct-usb4105: mechanical drawing, contact assignments',spec='16contactUSB2 receptacle; shield bonded at entry; 5V only; C-to-C >=1.5A source required')
    c.c(s,'1u','USB_VBUS',voltage='16V',note='Keep total pre-switch VBUS capacitance below10uF')
    esd(c,s,'D1','USB_DP_CONN','USB_DM_CONN');esd(c,s,'D2','CC1','CC2')
    c.ic(s,'U1','TUSB320LAIRWBR','Package_DFN_QFN:Texas_X2QFN-12_1.6x1.6mm_P0.4mm',
         ['CC1','CC2','PORT','VBUS_DET','ADDR','OUT3','OUT1','OUT2','ID','GND','EN_N','VDD'],
         {'1':'CC1','2':'CC2','3':'GND','4':'VBUS_DET','7':'USB_DEFAULT_N','10':'GND','11':'GND','12':'USB_CC3V3'},
         types={'1':'bidirectional','2':'bidirectional','3':'input','4':'input','5':'input','6':'open_collector','7':'open_collector','8':'open_collector','9':'open_collector','10':'power_in','11':'input','12':'power_in'},
         source='tusb320lai:pp3,12 Table3,23 application,36 land',spec='PORT=GND UFP; ADDR NC GPIO mode; internal Rd, NO external5.1k; OUT1 low only attached1.5/3A')
    c.r(s,'900k','USB_VBUS','VBUS_DET',tolerance='1%');c.r(s,'10k','USB_CC3V3','USB_DEFAULT_N');c.c(s,'100n','USB_CC3V3',size='0402')
    c.ic(s,'U2','SN74LVC1G04DBVR',SOT5,['NC','A','GND','Y','VCC'],{'2':'USB_DEFAULT_N','3':'GND','4':'USB_POWER_ALLOWED','5':'USB_CC3V3'},
         types={'2':'input','3':'power_in','4':'output','5':'power_in'},source='sn74lvc1g04',spec='USB-domain inverter, default/unattached => limiter disabled')
    c.c(s,'100n','USB_CC3V3',size='0402');c.r(s,'100k','USB_POWER_ALLOWED','GND')
    s=c.sheet('01a_usb_cc_supply','Pre-limiter USB-only3.3V powers CC detector and its inverter. TUSB320LAI VDD maximum5.0V: NEVER connect it directly to USB VBUS. No SYS rail dependency.')
    ldo(c,s,'U16','USB_VBUS','USB_CC3V3','33')
    c.r(s,'3k','USB_CC3V3','GND',note='>=1mA minimum load for LDO specified output accuracy; include in pre-attach/suspend current budget')
    s=c.sheet('01b_usb_limit','USB load current limit about1A, below the minimum1.5A CC advertisement. Input<=6.5V; no PD sink negotiation.')
    limit_switch(c,s,'U3','USB_VBUS','USB_LIMITED','USB_POWER_ALLOWED','26.7k','USB')


def system_supply(c,vehicle):
    s=c.sheet('02_system_power','TPS2116 isolates automotive5V and USB; CAN PHY/GPS never fed from mux output.3.3V EN follows SYS5V, not automotive PGOOD.' if vehicle else 'Bridge USB-only3.3V buck. No CAN transceiver or vehicle-control output on this board.')
    if vehicle:
        c.ic(s,'U4','TPS2116DRLR',SOT583,['GND','VOUT','VIN1','PR1','MODE','VIN2','VOUT','ST'],
             {'1':'GND','2':'SYS5V','3':'AUTO5V','4':'AUTO_PRIORITY','5':'AUTO5V','6':'USB_LIMITED','7':'SYS5V','8':'USB_SERVICE_SENSE'},
             types={'1':'power_in','2':'power_out','3':'power_in','4':'input','5':'input','6':'power_in','7':'passive','8':'open_collector'},
             source='tps2116:pp3,12-16',spec='1.6..5.5V;4A power mux; source status NOT PGOOD; finite reverse response')
        c.r(s,'100k','AUTO5V','AUTO_PRIORITY');c.r(s,'30.1k','AUTO_PRIORITY','GND')
        c.r(s,'10k','3V3','USB_SERVICE_SENSE')
        c.c(s,'1u','AUTO5V');c.c(s,'1u','USB_LIMITED')
    else:
        c.r(s,'0','USB_LIMITED','SYS5V',power='0.25W',size='0805',note='Current>=1A zero-ohm jumper')
        c.power_flag(s,'SYS5V','USB limiter through0ohm link')
    c.ic(s,'U5','TPS629210DRLR',SOT583,['FB_VSET','PG','VOS','SW','GND','VIN','EN','MODE'],
         {'1':'VSET_3V3','3':'3V3','4':'SYS_SW','5':'GND','6':'SYS5V','7':'SYS5V','8':'SYS_MODE'},
         types={'1':'passive','2':'open_collector','3':'input','4':'output','5':'power_in','6':'power_in','7':'input','8':'passive'},
         source='tps629210:pp3,11-13,20-22',spec='1A buck; VSET249k=3.3V; MODE27.4k=autoPFM/PWM and output discharge')
    c.r(s,'249k','VSET_3V3','GND');c.r(s,'27.4k','SYS_MODE','GND')
    c.two(s,'L','2.2uH','SYS_SW','3V3',fp='CANView:Murata_DFE252012PD',mpn='DFE252012PD-2R2M=P2',spec='2.2uH±20%;2.5x2.0x1.2mm;Isat2.8A/Itemp2.2A;DCR84mohm;20VDC;-40..125C',source='murata-dfe252012:p1; tps629210 p22')
    c.c(s,'4.7u','SYS5V',voltage='16V');c.c(s,'22u','3V3',voltage='10V',size='0805',note='Verify effective capacitance with ALL downstream bypass; no extra100uF bulk without stability analysis')
    c.power_flag(s,'3V3','TPS629210 SW through output inductor')
    s=c.sheet('03_reset','MCU supervisor3.08V with130..270ms release. Communicator also has an independent PHY-domain supervisor.')
    supervisor(c,s,'U6','3V3','SYS_RESET_N')
    c.two(s,'SW','RESET','SYS_RESET_N','GND',fp='Button_Switch_SMD:SW_SPST_TL3342',mpn='TL3342F160QG',spec='Momentary normally-open; shared MCU reset')


def vehicle_front(c):
    s=c.sheet('00_vehicle_input','12V fused branch only. Common-source LM74800, bidirectional input TVS. No claim of validated unsuppressed load-dump survival.')
    c.ic(s,'J2','VEHICLE12V','Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical',['VBAT','GND'],{'1':'VBAT_IN','2':'GND'},
         types={'1':'power_out'},mpn='B2B-XH-A(LF)(SN)',source='jst-xh',spec='3A rated connector; external1A harness fuse at tap; fused IGN/ACC preferred; NOT an OBD pin assignment')
    c.two(s,'F','1A125V','VBAT_IN','VBAT_FUSED',fp='Fuse:Fuse_Littelfuse-NANO2-451_453',mpn='0451001.MRL',spec='1A;125V;50A interrupt@125VAC/DC,300A@32VDC;6.1x2.69mm; replace after trip; coordinate upstream harness fuse',source='littelfuse-451:pp2,4')
    c.ic(s,'D100','SMBJ36CA','Diode_SMD:D_SMB',['IO1','IO2'],{'1':'VBAT_FUSED','2':'GND'},spec='36V bidirectional TVS;600W10/1000us rating NOT350ms load-dump energy guarantee',source='smbj')
    c.c(s,'100n','VBAT_FUSED',voltage='100V',size='0805')
    c.power_flag(s,'VBAT_FUSED','J2 vehicle source through fuse')
    s=c.sheet('00b_reverse_ov','LM74800 common-source: HGATE drives INPUT FET; DGATE drives OUTPUT FET; A/OUT both=CS. Exposed pad FLOATS.')
    for ref,drain,gate in [('Q1','VBAT_FUSED','HGATE'),('Q2','PROTECTED_VBAT','DGATE')]:
        c.ic(s,ref,'BUK7Y12-100E','Package_TO_SOT_SMD:LFPAK56',['S','S','S','G','D'],
             {'1':'CS','2':'CS','3':'CS','4':gate,'5':drain},types={'4':'input'},source='buk7y12-100e:pp1-2,4,10 pinout, DC VGS and SOA; lm7480 pp6,29-32',spec='100V N-MOS; DC VGS +/-20V; RDSon<=12mohm@10V/25C; LFPAK56; common-source pair; pulse SOA qualification required')
    c.ic(s,'U7','LM74800QDRRRQ1','Package_SON:WSON-12-1EP_3x3mm_P0.5mm_EP1.5x2.5mm',
         ['DGATE','A','VSNS','SW','OV','EN_UVLO','GND','HGATE','OUT','VS','CAP','C','RTN_FLOAT'],
         {'1':'DGATE','2':'CS','3':'LM_VS','4':'LM_VS','5':'LM_OV','6':'LM_VS','7':'GND','8':'HGATE','9':'CS','10':'LM_VS','11':'LM_CAP','12':'PROTECTED_VBAT'},
         types={'1':'output','2':'input','3':'input','4':'passive','5':'input','6':'input','7':'power_in','8':'output','9':'input','10':'power_in','11':'output','12':'input','13':'passive'},source='lm7480:pp3,29-32; LM EVM CS capacitor erratum via TI forum',spec='3..65V controller; no current limiting; RTN EP13 isolated copper only')
    c.r(s,'10k','VBAT_FUSED','LM_VS',power='0.25W',voltage='150V',size='0805')
    c.c(s,'100n','LM_VS',voltage='100V',size='0805');c.c(s,'100n','LM_CAP','LM_VS',voltage='25V',size='0402')
    c.c(s,'1u','CS',voltage='100V',size='1206',note='TI EVM15nF corrected to1uF due to DGATE oscillation')
    c.ic(s,'D4','BAS21H,115','Diode_SMD:D_SOD-123F',['K','A'],{'1':'LM_VS','2':'CS'},source='bas21h',spec='200V diode; anodeCS cathodeVS; supplies controller from CS after input drop')
    c.ic(s,'D5','BZT52H-C47,115','Diode_SMD:D_SOD-123F',['K','A'],{'1':'LM_VS','2':'GND'},source='bzt52h',spec='47V zener±5%;375mW; VS clamp below65V subject to pulse qualification')
    c.r(s,'100k','PROTECTED_VBAT','LM_OV',voltage='100V',size='0603',tolerance='0.1%',tempco='10')
    c.r(s,'4.75k','LM_OV','GND',size='0603',tolerance='0.1%',tempco='10')
    c.power_flag(s,'LM_VS','LM supply via10k/diode; operating supply not an independent source')
    c.power_flag(s,'PROTECTED_VBAT','Vehicle source through common-source protection FETs')


def auto_converter(c):
    s=c.sheet('00c_auto_5v','MAX20040B adjustable5.0875V at2.2MHz, inside PHY5V range. OUT_S Kelvin.4.5V startup; low-crank capability must be measured.')
    names=['PGND2','LX1','BST1','VCC','AGND1','COMP','SPS','PGOOD','FSW','FB','FSYNC','AGND2','IN','PGND1','OUT','OUT_S','EN','NC','LX2','BST2','EP']
    nets={str(i+1):n for i,n in enumerate(['GND','AUTO_LX1','AUTO_BST1','AUTO_VCC4','GND','AUTO_COMP','AUTO_VCC4','AUTO_PGOOD','AUTO_FSW','AUTO_FB','AUTO_VCC4','GND','PROTECTED_VBAT','GND','AUTO5V','AUTO5V','PROTECTED_VBAT',None,'AUTO_LX2','AUTO_BST2','GND'])}
    types={str(n):'power_in' for n in [1,5,12,13,14,21]};types.update({'2':'output','19':'output','4':'power_out','8':'open_collector','15':'power_out'})
    c.ic(s,'U8','MAX20040BATPA/VY+','Package_DFN_QFN:TQFN-20-1EP_4x4mm_P0.5mm_EP2.9x2.9mm',names,nets,types=types,source='max20040:pp9-10,14-17; max20040-evm; KiCad21-100172 footprint / shared land90-0409 per MAX20090p29',spec='36V input max;5.0875V/1.2A nominal;4.7uH;2.2MHz; compensation starting point per EVM',note='FOOTPRINT PROVISIONAL: manufacturer land90-0409 original not archived/overlaid; fabrication blocked until pad geometry sign-off')
    c.r(s,'30.7k','AUTO5V','AUTO_FB',tolerance='0.1%',tempco='10',size='0603')
    c.r(s,'10k','AUTO_FB','GND',tolerance='0.1%',tempco='10',size='0603')
    for _ in range(2):c.c(s,'4.7u','PROTECTED_VBAT',voltage='50V',size='1210',mpn='GRM32ER71H475KA88L')
    c.c(s,'2.2u','AUTO_VCC4',voltage='16V',dielectric='X5R',mpn='GRM188R61C225KE15D')
    for n in [1,2]:c.c(s,'100n',f'AUTO_BST{n}',f'AUTO_LX{n}',voltage='50V',size='0402',mpn='GRM155R71H104KE14D')
    for n in [1,2]:
        c.ic(s,f'D{5+n}','PMEG4010BEA,115','Diode_SMD:D_SOD-323', ['K','A'],
             {'1':f'AUTO_BST{n}','2':'AUTO_VCC4'},source='max20040-evm:p4 external bootstrap feed; pmeg4010bea:pp1-5',
             spec='40V1A Schottky; SOD323; replaces EVM30V dual diode for voltage margin; validate bootstrap ripple/leakage at hot')
    c.two(s,'L','4.7uH','AUTO_LX1','AUTO_LX2',fp='Inductor_SMD:L_Coilcraft_XAL5030-XXX',mpn='XAL5030-472MEC',spec='4.7uH±20%; shielded; peak current/temperature per vendor and regulator limit',source='coilcraft-xal5030; max20040-evm')
    c.r(s,'12k','AUTO_FSW','GND');c.r(s,'22k','AUTO_COMP','AUTO_COMP_C')
    c.c(s,'680p','AUTO_COMP_C',dielectric='C0G',tolerance='5%',size='0402');c.c(s,'22p','AUTO_COMP',dielectric='C0G',tolerance='5%',size='0402')
    for _ in range(2):c.c(s,'22u','AUTO5V',voltage='25V',size='1210',mpn='GRM32ER71E226KE15L')
    c.r(s,'10k','PHY3V3','AUTO_PGOOD');c.r(s,'2.2k','AUTO5V','GND',note='Defined discharge,2.3mA at5V; not a substitute for transient tests')


def rail_qualification(c):
    s=c.sheet('03b_phy_power','PHY3V3 is automotive-only. Its own supervisor initializes arm latch on USB-first hot-plug. FT inputs track AUTO5V, not held-up3.3V.')
    ldo(c,s,'U9','AUTO5V','PHY3V3','33');supervisor(c,s,'U12','PHY3V3','PHY_RESET_N')
    c.r(s,'4.7k','PHY3V3','GND')
    s=c.sheet('03c_auto_qualification','Independent battery threshold and real5V qualification. PGOOD alone is not sufficient for MAX3055. Threshold/latency worst cases remain test gates.')
    c.ic(s,'U13','TPS3700DDCR',SOT6,['OUTA','GND','INA_P','INB_N','VDD','OUTB'],{'1':'BATT_GOOD','2':'GND','3':'BATT_SENSE','4':'GND','5':'PHY3V3'},
         types={'1':'open_collector','2':'power_in','3':'input','4':'input','5':'power_in','6':'open_collector'},source='tps3700:pp4,6-7',spec='Battery rising8Vnom; protects MAX3055 BATT operating condition; OUTB unused opposite polarity')
    c.r(s,'190k','PROTECTED_VBAT','BATT_SENSE',tolerance='0.1%',size='0603');c.r(s,'10k','BATT_SENSE','GND',tolerance='0.1%')
    c.r(s,'10k','PHY3V3','BATT_GOOD');c.c(s,'100n','PHY3V3',size='0402')
    c.ic(s,'U14','TPS389001DSER','Package_SON:WSON-6_1.5x1.5mm_P0.5mm', ['SENSE','GND','MR_N','VDD','CT','RESET_N'],
         {'1':'AUTO5_SENSE','2':'GND','3':'PHY3V3','4':'PHY3V3','5':'AUTO5_DELAY','6':'AUTO5_GOOD'},
         types={'1':'input','2':'power_in','3':'input','4':'power_in','5':'passive','6':'open_collector'},source='tps3890:pp3,5; DSE package',spec='1.15V adjustable sense±1%; delayed release; monitor AUTO5V independently of converterPGOOD')
    c.r(s,'32k','AUTO5V','AUTO5_SENSE',tolerance='0.05%',tempco='10',size='0603');c.r(s,'10k','AUTO5_SENSE','GND',tolerance='0.05%',tempco='10',size='0603')
    c.c(s,'10n','AUTO5_DELAY',dielectric='C0G',tolerance='5%');c.c(s,'100n','PHY3V3',size='0402');c.r(s,'10k','PHY3V3','AUTO5_GOOD')
    gate3(c,s,'U15','BATT_GOOD','AUTO5_GOOD','AUTO_PGOOD','AUTO_GOOD')
    s=c.sheet('19_gps_power','GPS automotive-only switched supply. Fault-current limit independent of ESP. Never enable in USB-only service.')
    gate3(c,s,'U45','GPS_PWR_REQ','AUTO_GOOD','SYS_RESET_N','GPS_PWR_EN')
    c.r(s,'100k','GPS_PWR_REQ','GND')
    limit_switch(c,s,'U46','AUTO5V','GPS5V','GPS_PWR_EN','61.9k','GPS')
    c.r(s,'4.7k','GPS5V','GND',note='Discharge switched receiver supply after shutdown')
