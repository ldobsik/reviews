import sys
from pathlib import Path as _Path
sys.path.insert(0, str(_Path(__file__).resolve().parent))

import argparse
import re
from pathlib import Path

import yaml
from jinja2 import Environment, FileSystemLoader, StrictUndefined

from model import *
from validate import validate_model

ARRAY_NAME_RE = re.compile(r'^(?P<base>[A-Za-z_][A-Za-z0-9_]*)\[(?P<idx>[0-9]+)\]$')

def parse_type_decl(t):
    m = re.fullmatch(r"\s*(?P<base>[A-Za-z_][A-Za-z0-9_]*)\s*(?:\[\s*(?P<len>[^\]]+)\s*\])?\s*", t)
    if m is None:
        raise ValueError(f'unsupported type declaration: {t}')
    return m.group('base'), m.group('len')

def parse_array_name(name):
    m = ARRAY_NAME_RE.fullmatch(name)
    return (m.group('base'), int(m.group('idx'))) if m else (name, None)

def load_fields(items):
    out = []
    for i in items:
        bt, arr = parse_type_decl(i['type'])
        out.append(FieldSpec(c_type=bt, name=i['name'], array_len=arr))
    return out

def load_args(items):
    out = []
    for i in items:
        bt, arr = parse_type_decl(i['type'])
        out.append(ArgSpec(c_type=bt, name=i['name'], array_len=arr))
    return out

def load_struct(block, *, allow_name=False, allow_ptr_name=False):
    if block is None:
        return None
    fields = load_fields(block.get('fields') or []) if 'fields' in block else None
    return StructSpec(type=block['type'], name=block.get('name') if allow_name else None, ptr_name=block.get('ptr_name') if allow_ptr_name else None, fields=fields)

def load_component(path: Path):
    d = yaml.safe_load(path.read_text(encoding='utf-8'))
    inputs = load_struct(d.get('inputs'), allow_name=True)
    outputs = load_struct(d.get('outputs'), allow_name=True)
    config = load_struct(d.get('config'), allow_ptr_name=True)
    cb = None
    if d.get('client_bindings') is not None:
        protos = []
        for i in d['client_bindings'].get('prototypes', []):
            base, size = parse_array_name(i['name'])
            protos.append(ClientSpec(name=base, array_size=size, return_type=i['return_type'], args=load_args(i.get('args', []))))
        cb = ClientBindingsSpec(name=d['client_bindings']['name'], prototypes=protos)
    servers = [ServerSpec(name=i['name'], return_type=i['return_type'], args=load_args(i.get('args', []))) for i in d.get('servers', [])]
    return ComponentSpec(name=d['component'], public_headers=d.get('public_headers', []), instance_type=d.get('instance_type', None), init_fn=d['init_fn'], step_fn=d['step_fn'], inputs=inputs, outputs=outputs, config=config, client_bindings=cb, servers=servers)

def load_ecu(path: Path):
    d = yaml.safe_load(path.read_text(encoding='utf-8'))
    engine_dict = dict(d['engine'])
    engine_dict.setdefault('ovr_slot_count', 0)
    engine_dict.setdefault('ovr_buffer_len', 0)
    engine_dict.setdefault('generate_validator_header', False)
    eng = EngineSpec(**engine_dict)
    swcs = [SwcImportSpec(**i) for i in d.get('swcs', [])]
    instances = []
    for i in d.get('instances', []):
        x = dict(i)
        x['client_bindings'] = dict(x.get('client_bindings', {}))
        instances.append(InstanceSpec(**x))
    schedules = SchedulesSpec(default=d['schedules']['default'], items=[ScheduleSpec(name=s['name'], period_ms=s['period_ms'], switch_mode=s['switch_mode'], expiry_points=[ExpiryPointSpec(**ep) for ep in s.get('expiry_points', [])]) for s in d['schedules'].get('items', [])])
    return EcuSpec(name=d['name'], swcs=swcs, engine=eng, instances=instances, schedules=schedules)

def merge_expiry_points(eps):
    out = []
    for ep in eps:
        if out and out[-1].offset_ms == ep.offset_ms:
            out[-1].run.extend(ep.run)
        else:
            out.append(ExpiryPointSpec(offset_ms=ep.offset_ms, run=list(ep.run)))
    return out

def build_context(root: Path, ecu_yaml: Path):
    ecu = load_ecu(ecu_yaml)
    comps = []
    comp_map = {}
    for swc in ecu.swcs:
        c = load_component((root / swc.yaml).resolve())
        comps.append(c)
        comp_map[c.name] = c
    validate_model(comps, ecu)

    instances = []
    for inst in ecu.instances:
        comp = comp_map[inst.component]
        bindings = []
        for proto in (comp.client_bindings.prototypes if comp.client_bindings else []):
            if proto.array_size is None:
                bindings.append({'prototype': proto, 'bind_expr': inst.client_bindings.get(proto.name)})
            else:
                bindings.append({'prototype': proto, 'items': [{'index': idx, 'bind_expr': inst.client_bindings.get(f'{proto.name}[{idx}]')} for idx in range(proto.array_size)]})
        instances.append({'name': inst.name, 'component': comp, 'default_config_ptr': inst.default_config_ptr, 'user_static_bind': inst.user_static_bind, 'client_bindings': bindings, 'mask_idx_in': len(instances), 'mask_idx_out': len(instances) + len(ecu.instances)})

    schedules = []
    for idx, s in enumerate(ecu.schedules.items):
        merged = merge_expiry_points(s.expiry_points)
        schedules.append({'name': s.name, 'enum_name': 'SWCIF_SCHEDULE_' + s.name, 'idx': idx, 'period_ms': s.period_ms, 'period_tick': s.period_ms // ecu.engine.mainfunction_period_ms, 'switch_mode': s.switch_mode, 'expiry_points': [{'offset_ms': ep.offset_ms, 'offset_tick': ep.offset_ms // ecu.engine.mainfunction_period_ms, 'run': ep.run} for ep in merged]})
    default_schedule = next(x for x in schedules if x['name'] == ecu.schedules.default)

    headers = []
    for c in comps:
        for h in (c.public_headers if c.public_headers else [f'{c.name}.h']):
            if h not in headers:
                headers.append(h)

    return {'ecu': ecu, 'components': comps, 'instances': instances, 'headers': headers, 'schedules': schedules, 'default_schedule': default_schedule, 'total_masks': len(ecu.instances) * 2}

def render_all(root: Path, ecu_yaml: Path, out_dir: Path):
    ctx = build_context(root, ecu_yaml)
    env = Environment(loader=FileSystemLoader(str(root / 'generator' / 'templates')), trim_blocks=True, lstrip_blocks=True, keep_trailing_newline=True, undefined=StrictUndefined)
    out_dir.mkdir(parents=True, exist_ok=True)
    templates_dir = out_dir / 'templates'
    templates_dir.mkdir(parents=True, exist_ok=True)
    names = ['SwcIf_IntCfg.h', 'SwcIf_GenCfg.h', 'SwcIf_Cfg.h', 'SwcIf_GenInc.h', 'SwcIf_Gen.h', 'SwcIf_GenSrv.h', 'SwcIf_GenMeta.h', 'SwcIf_GenMeta.c', 'SwcIf_GenValidators.h', 'SwcIf_Gen.c']
    for n in names:
        rendered = env.get_template(n + '.j2').render(**ctx).rstrip('\n') + '\n\n'
        if n == 'SwcIf_Cfg.h':
            (templates_dir / 'SwcIf_Cfg.h.template').write_text(rendered, encoding='utf-8')
        else:
            (out_dir / n).write_text(rendered, encoding='utf-8')
    for inst in ctx['instances']:
        rendered = env.get_template('SwcIf_Bind_Instance.stub.h.j2').render(instance=inst).rstrip('\n') + '\n\n'
        (templates_dir / f"SwcIf_Bind_{inst['name']}.h.template").write_text(rendered, encoding='utf-8')

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    ap.add_argument('--ecu', required=True)
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    render_all(Path(a.root).resolve(), Path(a.root).resolve() / a.ecu, Path(a.root).resolve() / a.out)
