"""Host reference payload codec, NOT deployed transport/authentication firmware."""
from pathlib import Path
import json
import struct
import hashlib

ROOT=Path(__file__).resolve().parents[2]
SCHEMA=json.loads((ROOT/'protocol/schema/navigation-v1.json').read_text(encoding='utf-8'))
FORMATS={'u8':'B','u16':'H','u32':'I','u64':'Q','i16':'h','i32':'i','i64':'q','bytes16':'16s'}


def layout(name):
    return struct.Struct('<'+''.join(FORMATS[t] for _,t in SCHEMA['messages'][name]['fields']))


def snapshot_digest(values):
    slots=[]
    for stream in ['nav','imu','baro','utc']:
        slots.extend([values[stream+'_rate_hz'],values[stream+'_count_limit']])
    try:body=struct.pack('<QI8H',values['source_boot_id'],values['current_revision'],*slots)
    except struct.error as exc:raise ValueError('snapshot integer overflow') from exc
    return hashlib.sha256(body).digest()[:16]


def validate(name,values):
    message=SCHEMA['messages'][name]
    if set(values)!={n for n,_ in message['fields']}:raise ValueError('field set mismatch')
    for n,t in message['fields']:
        v=values[n]
        if t=='bytes16':
            if not isinstance(v,bytes) or len(v)!=16:raise ValueError('digest size')
        elif type(v) is not int:raise ValueError('integer required')
        if n=='reserved' and v!=0:raise ValueError('unknown reserved bits')
    for n,(low,high) in message.get('limits',{}).items():
        if not low<=values[n]<=high:raise ValueError('range: '+n)
    for bit,fields in message.get('validity_groups',{}).items():
        if not values['validity']&int(bit) and any(values[n]!=0 for n in fields):raise ValueError('nonzero invalid field')
    if name=='NAV_STATE':
        if values['fix_type']==0 and values['validity']&127:raise ValueError('invalid fix carries navigation values')
        if values['fix_type']!=0 and not values['validity']&1:raise ValueError('fix without position')
        if values['fix_type']==4:
            if values['validity']&97!=97 or values['last_fix_age_ms']>45000:raise ValueError('invalid or expired DR')
            if not values['horizontal_accuracy_mm'] or not values['vertical_accuracy_mm']:raise ValueError('DR uncertainty required')
    if name=='BARO_STATE':
        if values['validity']&1 and values['pressure_pa']<30000:raise ValueError('pressure below sensor range')
        if values['validity']&4 and not 80000<=values['reference_pressure_pa']<=110000:raise ValueError('invalid reference pressure')
    if name=='UTC_ANCHOR':
        if values['source_quality']==0 and (values['utc_unix_ms'] or values['uncertainty_us']):raise ValueError('invalid UTC nonzero')
        if values['source_quality'] and values['leap_status']==0:raise ValueError('UTC without leap validity')
    if name=='CLOCK_ANCHOR_REPLY' and values['stm_t3_us']<values['stm_t2_us']:raise ValueError('reverse monotonic time')
    if name=='SENSOR_SUBSCRIBE':
        op,kind,rate,count=(values[n] for n in ['operation','sensor_kind','rate_hz','count_limit'])
        if not values['request_id'] or not values['target_boot_id']:raise ValueError('zero correlation')
        if op in [0,3] and (kind or rate or count):raise ValueError('GET/CLEAR must address own complete snapshot')
        if op==1 and (not kind or rate not in SCHEMA['rates_hz'][str(kind)]):raise ValueError('unsupported rate')
        if op==2 and (not kind or rate or count):raise ValueError('DELETE requires kind and zero rate/count')
    if name=='SENSOR_RESULT':
        for kind,stream in enumerate(['nav','imu','baro','utc'],start=1):
            rate,count=values[stream+'_rate_hz'],values[stream+'_count_limit']
            if rate and rate not in SCHEMA['rates_hz'][str(kind)]:raise ValueError('unsupported snapshot rate')
            if not rate and count:raise ValueError('disabled slot has count')
        if values['status'] in [0,1] and snapshot_digest(values)!=values['snapshot_sha256_128']:raise ValueError('snapshot digest mismatch')
        if values['status']!=0 and values['applied_time_us']:raise ValueError('non-applied result has apply time')


def encode(name,values):
    validate(name,values)
    try:return layout(name).pack(*(values[n] for n,_ in SCHEMA['messages'][name]['fields']))
    except (struct.error,OverflowError) as exc:raise ValueError('wire integer overflow') from exc


def decode(name,payload,*,version,capabilities):
    m=SCHEMA['messages'][name]
    required=SCHEMA[m['transport']+'_min_version']
    if tuple(version)<tuple(required) or version[0]!=required[0] or m['capability'] not in capabilities:
        raise ValueError('version/capability not negotiated')
    if not isinstance(payload,bytes) or len(payload)!=layout(name).size:raise ValueError('exact length required')
    values=dict(zip((n for n,_ in m['fields']),layout(name).unpack(payload)))
    validate(name,values)
    return values


def bandwidth(subscriptions,*,transport_overhead_bytes):
    """Use actual encoded header+tag size; no implicit payload-only budgeting."""
    if type(transport_overhead_bytes) is not int or not 1<=transport_overhead_bytes<=128:raise ValueError('unknown overhead')
    names={1:'NAV_STATE',2:'IMU_STATE',3:'BARO_STATE',4:'UTC_ANCHOR'}
    if len(subscriptions)>4:raise ValueError('too many streams')
    total=0
    for kind,rate in subscriptions.items():
        if kind not in names or rate not in SCHEMA['rates_hz'][str(kind)]:raise ValueError('unsupported stream/rate')
        total+=rate*(layout(names[kind]).size+transport_overhead_bytes)
    if total>SCHEMA['sensor_budget_bytes_per_second']['per_peer']:raise ValueError('BUDGET_EXCEEDED')
    return total


if __name__=='__main__':
    for n in SCHEMA['messages']:print(f'{n}: {layout(n).size} bytes')
