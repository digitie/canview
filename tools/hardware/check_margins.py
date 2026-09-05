"""First-order design bounds, not surge/SOA/loop or HIL qualification."""
from pathlib import Path
import json

ROOT=Path(__file__).resolve().parents[2]


def divider(reference, top, bottom, tolerance, leakage=0):
    low=reference[0]*(1+top*(1-tolerance)/(bottom*(1+tolerance)))-leakage*top*(1+tolerance)
    high=reference[1]*(1+top*(1+tolerance)/(bottom*(1-tolerance)))+leakage*top*(1+tolerance)
    return [low,high]


def calculate():
    # Resistor tolerance + 10ppm/K x 100K, deliberately independent extremes.
    output=divider((1.234,1.266),30700,10000,.001+.001,1e-6)
    fall=divider((1.15*.99,1.15*1.01),32000,10000,.0005+.001,100e-9)
    rise=[fall[0]*1.00325,fall[1]*1.00825]
    ov=divider((1.195,1.267),100000,4750,.001+.001,200e-9)
    watchdog=[(77.4*.120*.95+55)*.905,(77.4*(.120*1.05+.010)+55)*1.095]
    assert watchdog[1]<100
    assert output[0]>rise[1]
    assert output[1]<5.25 and fall[0]>4.75
    assert ov[1]<36
    result=dict(auto5v_dc_bound_v=output, auto5_good_falling_v=fall,auto5_good_rising_v=rise,
                minimum_startup_margin_v=output[0]-rise[1],maximum_positive_ripple_overshoot_budget_v=5.25-output[1],
                minimum_collapse_detection_voltage_margin_v=fall[0]-4.75,ov_cutoff_v=ov,watchdog_missing_pulse_ms=watchdog,
                assumptions=['MAX FB1uA applied as engineering allowance; datasheet test condition is FB=VCC, not guaranteed at1.25V',
                             '10ppm/K resistors, independent extremes over100K from25C',
                             'TPS3890 falling delay18us is TYPICAL; no guaranteed maximum, fast-collapse HIL mandatory',
                             'Watchdog120pF5% + <=10pF parasitic, IC timing±9.5%; heartbeat falling interval<=30ms',
                             'Bounds exclude ripple, overshoot, layout, transient SOA, capacitor DC-bias and thermal limits'])
    return result


if __name__=='__main__':
    result=calculate()
    (ROOT/'hardware/margin-check.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
