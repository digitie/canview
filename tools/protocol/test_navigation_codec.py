"""Golden vectors and rejection cases for proposed navigation payloads."""
import json
from pathlib import Path
import unittest
import navigation_codec as codec


def blank(name):
    values={n:bytes(16) if t=='bytes16' else 0 for n,t in codec.SCHEMA['messages'][name]['fields']}
    if name=='SENSOR_RESULT':values['snapshot_sha256_128']=codec.snapshot_digest(values)
    return values


class PayloadTests(unittest.TestCase):
    def decode(self,name,payload,version=None,caps=None):
        m=codec.SCHEMA['messages'][name]
        return codec.decode(name,payload,version=version or codec.SCHEMA[m['transport']+'_min_version'],capabilities=caps if caps is not None else {m['capability']})

    def test_fixed_golden(self):
        v={'esp_boot_id':0x0102030405060708,'query_id':0x11223344,'esp_t1_us':0x1020304050607080}
        golden=bytes.fromhex('0807060504030201443322118070605040302010')
        self.assertEqual(codec.encode('CLOCK_ANCHOR_QUERY',v),golden)
        self.assertEqual(self.decode('CLOCK_ANCHOR_QUERY',golden),v)

    def test_all_message_sizes_and_roundtrip(self):
        for name in codec.SCHEMA['messages']:
            v=blank(name)
            if name=='SENSOR_SUBSCRIBE':v.update(request_id=1,target_boot_id=1)
            if name=='SENSOR_HEALTH':v['sensor_kind']=1
            payload=codec.encode(name,v)
            self.assertLessEqual(len(payload),200)
            self.assertEqual(self.decode(name,payload),v)
            for bad in [payload[:-1],payload+b'\0']:
                with self.assertRaises(ValueError):self.decode(name,bad)

    def test_unknown_capability_and_old_version(self):
        body=codec.encode('NAV_STATE',blank('NAV_STATE'))
        for args in [{'version':[1,3]},{'version':[2,0]},{'caps':set()}]:
            with self.assertRaises(ValueError):self.decode('NAV_STATE',body,**args)

    def test_signed_coordinates_and_invalid_bits(self):
        v=blank('NAV_STATE');v.update(source_boot_id=1,validity=1,fix_type=3,latitude_e7=-900000000,longitude_e7=1800000000)
        self.assertEqual(self.decode('NAV_STATE',codec.encode('NAV_STATE',v)),v)
        for key,value in [('longitude_e7',1800000001),('latitude_e7',900000001),('validity',256),('reserved',1),('speed_mm_s',1)]:
            with self.assertRaises(ValueError):codec.encode('NAV_STATE',v|{key:value})

    def test_dr_expires_and_requires_uncertainty(self):
        v=blank('NAV_STATE')|{'validity':97,'fix_type':4,'horizontal_accuracy_mm':1000,'vertical_accuracy_mm':2000,'last_fix_age_ms':45000}
        codec.encode('NAV_STATE',v)
        for patch in [{'last_fix_age_ms':45001},{'horizontal_accuracy_mm':0},{'validity':65}]:
            with self.assertRaises(ValueError):codec.encode('NAV_STATE',v|patch)

    def test_baro_out_of_range(self):
        v=blank('BARO_STATE')|{'validity':1,'pressure_pa':101325}
        codec.encode('BARO_STATE',v)
        for pressure in [29999,125001]:
            with self.assertRaises(ValueError):codec.encode('BARO_STATE',v|{'pressure_pa':pressure})

    def test_subscription_operations(self):
        v=blank('SENSOR_SUBSCRIBE')|{'request_id':1,'target_boot_id':2,'operation':1,'sensor_kind':2,'rate_hz':20}
        codec.encode('SENSOR_SUBSCRIBE',v)
        for patch in [{'operation':2},{'operation':3},{'operation':0},{'rate_hz':17},{'sensor_kind':0},{'request_id':0}]:
            with self.assertRaises(ValueError):codec.encode('SENSOR_SUBSCRIBE',v|patch)

    def test_actual_overhead_and_budget(self):
        amount=codec.bandwidth({1:10,2:20,3:2,4:1},transport_overhead_bytes=36)
        self.assertLessEqual(amount,4096)
        for rates in [{1:25,2:100},{2:101}]:
            with self.assertRaises(ValueError):codec.bandwidth(rates,transport_overhead_bytes=36)
        with self.assertRaises(ValueError):codec.bandwidth({1:1},transport_overhead_bytes=0)

    def test_no_native_alignment_or_overflow(self):
        self.assertEqual(codec.layout('CLOCK_ANCHOR_QUERY').size,20)
        v={'esp_boot_id':-1,'query_id':0,'esp_t1_us':0}
        with self.assertRaises(ValueError):codec.encode('CLOCK_ANCHOR_QUERY',v)

    def test_subscription_golden(self):
        v=blank('SENSOR_SUBSCRIBE')|{'request_id':1,'target_boot_id':2,'expected_revision':3,'operation':1,'sensor_kind':2,'rate_hz':20,'count_limit':500}
        golden=bytes.fromhex('010000000000000002000000000000000300000001021400f4010000')
        self.assertEqual(codec.encode('SENSOR_SUBSCRIBE',v),golden)
        self.assertEqual(self.decode('SENSOR_SUBSCRIBE',golden),v)

    def test_utc_quality_requires_valid_leap_and_zero_invalid_fields(self):
        v=blank('UTC_ANCHOR')
        for patch in [{'uncertainty_us':1},{'utc_unix_ms':1},{'source_quality':2}]:
            with self.assertRaises(ValueError):codec.encode('UTC_ANCHOR',v|patch)
        valid=v|{'source_quality':2,'leap_status':1,'utc_unix_ms':1700000000000,'uncertainty_us':100}
        self.assertEqual(self.decode('UTC_ANCHOR',codec.encode('UTC_ANCHOR',valid)),valid)

    def test_result_digest_and_exact_snapshot(self):
        import hashlib
        v=blank('SENSOR_RESULT')|{'source_boot_id':2,'current_revision':3,'nav_rate_hz':10,'nav_count_limit':500}
        # Independent byte-level snapshot golden, no schema/codec packing.
        body=bytes.fromhex('0200000000000000030000000a00f401000000000000000000000000')
        v['snapshot_sha256_128']=hashlib.sha256(body).digest()[:16]
        self.assertEqual(self.decode('SENSOR_RESULT',codec.encode('SENSOR_RESULT',v)),v)
        for patch in [{'nav_rate_hz':20},{'imu_rate_hz':17},{'utc_count_limit':1},{'snapshot_sha256_128':bytes(16)}]:
            with self.assertRaises(ValueError):codec.encode('SENSOR_RESULT',v|patch)

    def assert_rejected_both_directions(self,name,values):
        with self.assertRaises(ValueError):codec.encode(name,values)
        # Bypass encoder validation to exercise hostile on-wire input independently.
        raw=codec.layout(name).pack(*(values[n] for n,_ in codec.SCHEMA['messages'][name]['fields']))
        with self.assertRaises(ValueError):self.decode(name,raw)

    def test_capability_masks_follow_rate_arrays(self):
        v=blank('SENSOR_CAPABILITIES')|{'hardware_profile':1,'feature_mask':15}
        for kind,stream in enumerate(['nav','imu','baro','utc'],start=1):
            width=len(codec.SCHEMA['rates_hz'][str(kind)])
            valid=v|{stream+'_rate_mask':(1<<width)-1}
            self.assertEqual(self.decode('SENSOR_CAPABILITIES',codec.encode('SENSOR_CAPABILITIES',valid)),valid)
            self.assert_rejected_both_directions('SENSOR_CAPABILITIES',v|{stream+'_rate_mask':1<<width})
            self.assert_rejected_both_directions('SENSOR_CAPABILITIES',v|{'feature_mask':0,stream+'_rate_mask':1})

    def test_dr_capability_cross_constraints(self):
        v=blank('SENSOR_CAPABILITIES')|{'hardware_profile':1,'feature_mask':17,'max_dr_age_ms':45000}
        self.assertEqual(self.decode('SENSOR_CAPABILITIES',codec.encode('SENSOR_CAPABILITIES',v)),v)
        for patch in [{'hardware_profile':2},{'hardware_profile':0},{'feature_mask':16},{'feature_mask':1},{'max_dr_age_ms':0}]:
            self.assert_rejected_both_directions('SENSOR_CAPABILITIES',v|patch)

    def test_unauthorized_result_has_no_snapshot(self):
        v=blank('SENSOR_RESULT')|{'request_id':1,'source_boot_id':1,'status':3,'snapshot_sha256_128':bytes(16)}
        self.assertEqual(self.decode('SENSOR_RESULT',codec.encode('SENSOR_RESULT',v)),v)
        for n,t in codec.SCHEMA['messages']['SENSOR_RESULT']['fields']:
            if n in {'request_id','source_boot_id','status'}:continue
            self.assert_rejected_both_directions('SENSOR_RESULT',v|{n:b'X'*16 if t=='bytes16' else 1})

    def test_malformed_version_shape(self):
        name='NAV_STATE';raw=codec.encode(name,blank(name))
        for version in [[1,4,999],[1,4.0],[True,4],[1],[],None,'1.4',[1,256],[-1,4]]:
            with self.assertRaises(ValueError):codec.decode(name,raw,version=version,capabilities={'sensor.nav.v1'})


if __name__=='__main__':unittest.main()
