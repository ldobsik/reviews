def _ensure_fields_shape(block_name, struct_spec):
    if struct_spec is not None:
        if struct_spec.fields is not None and len(struct_spec.fields) == 0:
            raise ValueError(f'{block_name}.fields must be omitted for opaque types or contain at least one field')


def validate_model(components, ecu):
    component_map = {component.name: component for component in components}
    instance_map = {}

    if ecu.engine.mainfunction_period_ms <= 0:
        raise ValueError('mainfunction_period_ms must be > 0')

    for instance in ecu.instances:
        if instance.name in instance_map:
            raise ValueError(f'duplicate instance name: {instance.name}')
        if instance.component not in component_map:
            raise ValueError(f'unknown component for instance {instance.name}: {instance.component}')
        instance_map[instance.name] = instance

    for component in components:
        if component.inputs is not None and not component.inputs.name:
            raise ValueError(f'inputs.name is required for component {component.name}')
        if component.outputs is not None and not component.outputs.name:
            raise ValueError(f'outputs.name is required for component {component.name}')
        if component.config is not None and not component.config.ptr_name:
            raise ValueError(f'config.ptr_name is required for component {component.name}')

        _ensure_fields_shape('inputs', component.inputs)
        _ensure_fields_shape('outputs', component.outputs)
        _ensure_fields_shape('config', component.config)

        if component.client_bindings is not None:
            if not component.client_bindings.name:
                raise ValueError(f'client_bindings.name is required for component {component.name}')
            names = [proto.name for proto in component.client_bindings.prototypes]
            if len(names) != len(set(names)):
                raise ValueError(f'duplicate client prototype name in component {component.name}')

    if ecu.schedules is None or len(ecu.schedules.items) == 0:
        raise ValueError('at least one schedule must be defined')
    if ecu.schedules.default not in {schedule.name for schedule in ecu.schedules.items}:
        raise ValueError('default schedule not found')

    for schedule in ecu.schedules.items:
        if schedule.period_ms <= 0:
            raise ValueError(f'period_ms must be > 0 for schedule {schedule.name}')
        if (schedule.period_ms % ecu.engine.mainfunction_period_ms) != 0:
            raise ValueError(f'period_ms not aligned for schedule {schedule.name}')
        if len(schedule.expiry_points) == 0:
            raise ValueError(f'schedule {schedule.name} must have at least one expiry point')
        last_offset = -1
        for expiry_point in schedule.expiry_points:
            if expiry_point.offset_ms < last_offset:
                raise ValueError(f'offset_ms not non-decreasing in schedule {schedule.name}')
            if expiry_point.offset_ms < 0 or expiry_point.offset_ms >= schedule.period_ms:
                raise ValueError(f'offset_ms out of range in schedule {schedule.name}')
            if (expiry_point.offset_ms % ecu.engine.mainfunction_period_ms) != 0:
                raise ValueError(f'offset_ms not aligned in schedule {schedule.name}')
            for instance_name in expiry_point.run:
                if instance_name not in instance_map:
                    raise ValueError(f'unknown instance {instance_name} in schedule {schedule.name}')
            last_offset = expiry_point.offset_ms

    for instance in ecu.instances:
        comp = component_map[instance.component]
        if instance.default_config_ptr is not None and comp.config is None:
            raise ValueError(f'instance {instance.name} provides default_config_ptr but component {comp.name} has no config block')
        if comp.client_bindings is None:
            if len(instance.client_bindings) != 0:
                raise ValueError(f'instance {instance.name} provides client_bindings but component {comp.name} has no client_bindings block')
        else:
            valid_names = {proto.name for proto in comp.client_bindings.prototypes}
            for binding_name in instance.client_bindings.keys():
                if binding_name not in valid_names:
                    raise ValueError(f'instance {instance.name} binds unknown client {binding_name} for component {comp.name}')

# --- Extended validation for singletons and shared I/O ---
try:
    _orig_validate_model
except NameError:
    _orig_validate_model = validate_model if 'validate_model' in globals() else None

def _is_static_component_(comp):
    t = getattr(comp, 'instance_type', None)
    return (t is None) or (str(t).strip() == '')

def validate_model(components, ecu):
    if _orig_validate_model:
        _orig_validate_model(components, ecu)
    comp_map = {c.name: c for c in components}
    comp_to_instances = {c.name: [] for c in components}
    for inst in ecu.instances:
        comp_to_instances[inst.component].append(inst)
    for comp in components:
        insts = comp_to_instances.get(comp.name, [])
        if _is_static_component_(comp):
            if len(insts) > 1:
                raise ValueError(f"Static component '{comp.name}' may have at most one instance named '{comp.name}'")
            if len(insts) == 1 and insts[0].name != comp.name:
                raise ValueError(f"Static component '{comp.name}' must use instance name '{comp.name}', got '{insts[0].name}'")
            if comp.config is not None and not getattr(comp.config, 'ptr_name', None):
                raise ValueError(f"Static component '{comp.name}' has config but missing ptr_name")
        if comp.inputs and comp.outputs and comp.inputs.name and comp.outputs.name and comp.inputs.name == comp.outputs.name:
            if comp.inputs.type != comp.outputs.type:
                raise ValueError(f"Component '{comp.name}': inputs/outputs share name '{comp.inputs.name}' but types differ")
            in_fields = comp.inputs.fields or []
            out_fields = comp.outputs.fields or []
            if len(in_fields) != len(out_fields):
                raise ValueError(f"Component '{comp.name}': inputs/outputs share name but field count differs")
            for i,(fi,fo) in enumerate(zip(in_fields, out_fields)):
                if (fi.c_type != fo.c_type) or (fi.name != fo.name) or (fi.array_len != fo.array_len):
                    raise ValueError(f"Component '{comp.name}': inputs/outputs share name but field[{i}] differs")
    for inst in ecu.instances:
        comp = comp_map[inst.component]
        if _is_static_component_(comp) and getattr(inst,'default_config_ptr',None) is not None:
            if comp.config is None or not getattr(comp.config,'ptr_name',None):
                raise ValueError(f"Instance '{inst.name}' provides default_config_ptr but component '{comp.name}' has no config.ptr_name (static)")
