import argparse
import re
from pathlib import Path
import yaml
from jinja2 import Environment, FileSystemLoader, StrictUndefined
from model import *
from validate import validate_model


def parse_type_decl(t):
    m = re.fullmatch(r"\s*(?P<base>[A-Za-z_][A-Za-z0-9_]*)\s*(?:\[\s*(?P<len>[^\]]+)\s*\])?\s*", t)
    if m is None:
        raise ValueError(f"unsupported type declaration: {t}")
    return m.group("base"), m.group("len")


def load_fields(items):
    r = []
    for i in items:
        bt, arr = parse_type_decl(i["type"])
        r.append(FieldSpec(c_type=bt, name=i["name"], array_len=arr))
    return r


def load_args(items):
    r = []
    for i in items:
        bt, arr = parse_type_decl(i["type"])
        r.append(ArgSpec(c_type=bt, name=i["name"], array_len=arr))
    return r


def load_struct(block, *, allow_name=False, allow_ptr_name=False):
    if block is None:
        return None
    fields = None
    if "fields" in block:
        fields = load_fields(block.get("fields") or [])
    return StructSpec(
        type=block["type"],
        name=block.get("name") if allow_name else None,
        ptr_name=block.get("ptr_name") if allow_ptr_name else None,
        fields=fields,
    )


def load_component(path: Path):
    d = yaml.safe_load(path.read_text(encoding="utf-8"))
    inputs = load_struct(d.get("inputs"), allow_name=True)
    outputs = load_struct(d.get("outputs"), allow_name=True)
    config = load_struct(d.get("config"), allow_ptr_name=True)
    cb = None
    if d.get("client_bindings") is not None:
        cb = ClientBindingsSpec(
            name=d["client_bindings"]["name"],
            prototypes=[
                ClientSpec(name=i["name"], return_type=i["return_type"], args=load_args(i.get("args", [])))
                for i in d["client_bindings"].get("prototypes", [])
            ],
        )
    servers = [
        ServerSpec(name=i["name"], return_type=i["return_type"], args=load_args(i.get("args", [])))
        for i in d.get("servers", [])
    ]
    return ComponentSpec(
        name=d["component"],
        public_headers=d.get("public_headers", []),
        instance_type=d.get("instance_type", None),
        init_fn=d["init_fn"],
        step_fn=d["step_fn"],
        inputs=inputs,
        outputs=outputs,
        config=config,
        client_bindings=cb,
        servers=servers,
    )


def load_ecu(path: Path):
    d = yaml.safe_load(path.read_text(encoding="utf-8"))
    if "integration_headers" not in d["engine"]:
        raise ValueError("engine.integration_headers must be provided (possibly empty)")
    engine_dict = dict(d["engine"])
    engine_dict.setdefault("ovr_slot_count", 0)
    engine_dict.setdefault("ovr_buffer_len", 0)
    engine_dict.setdefault("generate_validator_header", False)
    eng = EngineSpec(**engine_dict)
    swcs = [SwcImportSpec(**i) for i in d.get("swcs", [])]
    instances = []
    for i in d.get("instances", []):
        x = dict(i)
        x["client_bindings"] = dict(x.get("client_bindings", {}))
        instances.append(InstanceSpec(**x))
    schedules = SchedulesSpec(
        default=d["schedules"]["default"],
        items=[
            ScheduleSpec(
                name=s["name"],
                period_ms=s["period_ms"],
                switch_mode=s["switch_mode"],
                expiry_points=[ExpiryPointSpec(**ep) for ep in s.get("expiry_points", [])],
            )
            for s in d["schedules"].get("items", [])
        ],
    )
    return EcuSpec(name=d["name"], swcs=swcs, engine=eng, instances=instances, schedules=schedules)


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
            bindings.append({
                "prototype": proto,
                "bind_expr": inst.client_bindings.get(proto.name),
            })
        instances.append({
            "name": inst.name,
            "component": comp,
            "default_config_ptr": inst.default_config_ptr,
            "user_static_bind": inst.user_static_bind,
            "client_bindings": bindings,
        })

    schedules = []
    for idx, s in enumerate(ecu.schedules.items):
        merged = merge_expiry_points(s.expiry_points)
        schedules.append({
            "name": s.name,
            "enum_name": "SWCIF_SCHEDULE_" + s.name,
            "idx": idx,
            "period_ms": s.period_ms,
            "period_tick": s.period_ms // ecu.engine.mainfunction_period_ms,
            "switch_mode": s.switch_mode,
            "expiry_points": [{
                "offset_ms": ep.offset_ms,
                "offset_tick": ep.offset_ms // ecu.engine.mainfunction_period_ms,
                "run": ep.run,
            } for ep in merged],
        })
    default_schedule = next(x for x in schedules if x["name"] == ecu.schedules.default)

    headers = []
    for c in comps:
        # validate public_headers if provided
        if getattr(c, 'public_headers', None) is not None:
            if not isinstance(c.public_headers, list) or not all(isinstance(h, str) for h in c.public_headers):
                raise ValueError(f'component {c.name}: public_headers must be list[str]')
            hdrs = c.public_headers if len(c.public_headers)>0 else [f'{c.name}.h']
        else:
            hdrs = [f'{c.name}.h']
        for h in hdrs:
            if h not in headers:
                headers.append(h)

    return {
        "ecu": ecu,
        "components": comps,
        "instances": instances,
        "headers": headers,
        "schedules": schedules,
        "default_schedule": default_schedule,
    }


def render_all(root: Path, ecu_yaml: Path, out_dir: Path, *, emit_swc_templates: bool=False, swc_templates_out: Path=None):
    ctx = build_context(root, ecu_yaml)
    env = Environment(
        loader=FileSystemLoader(str(root / "generator" / "templates")),
        trim_blocks=True,
        lstrip_blocks=True,
        keep_trailing_newline=True,
        undefined=StrictUndefined,
    )
    out_dir.mkdir(parents=True, exist_ok=True)
    # Optionally emit SWC skeleton stubs/templates
    if emit_swc_templates:
        sk_out = swc_templates_out or (out_dir / "swc_templates")
        sk_out.mkdir(parents=True, exist_ok=True)
        for comp in ctx["components"]:
            comp_dir = sk_out / comp.name
            comp_dir.mkdir(parents=True, exist_ok=True)
            (comp_dir / f"{comp.name}.h").write_text(env.get_template("SwcTemplate.h.j2").render(comp=comp), encoding="utf-8")
            (comp_dir / f"{comp.name}.c").write_text(env.get_template("SwcTemplate.c.j2").render(comp=comp), encoding="utf-8")

    names = [
        'SwcIf_IntCfg.h',
        "SwcIf_Cfg.h",
        "SwcIf_GenInc.h",
        "SwcIf_Gen.h",
        "SwcIf_GenSrv.h",
        "SwcIf_GenMeta.h",
        "SwcIf_GenMeta.c",
        "SwcIf_GenValidators.h",
        "SwcIf_Gen.c",
    ]
    for n in names:
        (out_dir / n).write_text(env.get_template(n + ".j2").render(**ctx).rstrip('\n') + '\n\n', encoding="utf-8")
    for inst in ctx["instances"]:
        (out_dir / f'SwcIf_Bind_{inst["name"]}.stub.c').write_text(
            env.get_template('SwcIf_Bind_Instance.stub.c.j2').render(instance=inst).rstrip('\n') + '\n\n',
            encoding="utf-8",
        )


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument('--root', required=True)
    p.add_argument('--ecu', required=True)
    p.add_argument('--out', required=True)
    p.add_argument('--emit-swc-templates', action='store_true', dest='emit_swc_templates')
    p.add_argument('--swc-templates-out', default=None)
    a = p.parse_args()
    render_all(Path(a.root).resolve(), Path(a.root).resolve() / a.ecu, Path(a.root).resolve() / a.out, emit_swc_templates=a.emit_swc_templates, swc_templates_out=(Path(a.swc_templates_out).resolve() if a.swc_templates_out else None))
