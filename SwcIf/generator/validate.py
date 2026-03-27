import re

ARRAY_NAME_RE = re.compile(r'^(?P<base>[A-Za-z_][A-Za-z0-9_]*)\[(?P<idx>[0-9]+)\]$')

def _split_array_name(name):
    m = ARRAY_NAME_RE.fullmatch(name)
    return (m.group('base'), int(m.group('idx'))) if m else (name, None)

def validate_model(components, ecu):
    if ecu.engine.ovr_slot_count > 32:
        raise ValueError('ovr_slot_count > 32 is not supported')

    component_map = {c.name: c for c in components}
    instance_map = {}
    for inst in ecu.instances:
        if inst.name in instance_map:
            raise ValueError(f'duplicate instance name: {inst.name}')
        if inst.component not in component_map:
            raise ValueError(f'unknown component for instance {inst.name}: {inst.component}')
        instance_map[inst.name] = inst

    for comp in components:
        if comp.client_bindings is None:
            continue
        proto_map = {p.name: p for p in comp.client_bindings.prototypes}
        for inst in ecu.instances:
            if inst.component != comp.name:
                continue
            seen = {}
            for key in inst.client_bindings.keys():
                base, idx = _split_array_name(key)
                if base not in proto_map:
                    raise ValueError(f'instance {inst.name} binds unknown client {key}')
                proto = proto_map[base]
                if proto.array_size is None and idx is not None:
                    raise ValueError(f'instance {inst.name} uses indexed syntax for scalar client {base}')
                if proto.array_size is not None:
                    if idx is None or idx < 0 or idx >= proto.array_size:
                        raise ValueError(f'instance {inst.name} binds invalid index for {base}')
                    seen.setdefault(base, set()).add(idx)
            for p in comp.client_bindings.prototypes:
                if p.array_size is not None:
                    idxs = seen.get(p.name, set())
                    if len(idxs) not in (0, p.array_size):
                        raise ValueError(f'instance {inst.name} must bind all or none of {p.name}[0..{p.array_size - 1}]')
