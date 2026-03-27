from dataclasses import dataclass, field
from typing import Dict, List, Optional


@dataclass
class FieldSpec:
    c_type: str
    name: str
    array_len: Optional[str] = None


@dataclass
class ArgSpec:
    c_type: str
    name: str
    array_len: Optional[str] = None


@dataclass
class StructSpec:
    type: str
    name: Optional[str] = None
    ptr_name: Optional[str] = None
    fields: Optional[List[FieldSpec]] = None


@dataclass
class ClientSpec:
    name: str
    return_type: str
    args: List[ArgSpec] = field(default_factory=list)
    array_size: Optional[int] = None

    @property
    def base_name(self) -> str:
        return self.name

    @property
    def is_array(self) -> bool:
        return self.array_size is not None


@dataclass
class ServerSpec:
    name: str
    return_type: str
    args: List[ArgSpec] = field(default_factory=list)


@dataclass
class ClientBindingsSpec:
    name: str
    prototypes: List[ClientSpec] = field(default_factory=list)


@dataclass
class ComponentSpec:
    name: str
    public_headers: List[str]
    instance_type: Optional[str] = None
    init_fn: str = ''
    step_fn: str = ''
    inputs: Optional[StructSpec] = None
    outputs: Optional[StructSpec] = None
    config: Optional[StructSpec] = None
    client_bindings: Optional[ClientBindingsSpec] = None
    servers: List[ServerSpec] = field(default_factory=list)


@dataclass
class SwcImportSpec:
    name: str
    yaml: str


@dataclass
class InstanceSpec:
    name: str
    component: str
    default_config_ptr: Optional[str] = None
    user_static_bind: bool = False
    client_bindings: Dict[str, str] = field(default_factory=dict)


@dataclass
class ExpiryPointSpec:
    offset_ms: int
    run: List[str] = field(default_factory=list)


@dataclass
class ScheduleSpec:
    name: str
    period_ms: int
    switch_mode: str
    expiry_points: List[ExpiryPointSpec] = field(default_factory=list)


@dataclass
class SchedulesSpec:
    default: str
    items: List[ScheduleSpec] = field(default_factory=list)


@dataclass
class EngineSpec:
    mainfunction_period_ms: int
    integration_headers: List[str]
    enable_input_raw_api: bool
    enable_output_raw_api: bool
    enable_config_raw_api: bool
    enable_input_ovr_api: bool
    enable_output_ovr_api: bool
    enable_instance_info_api: bool
    enable_input_info_api: bool
    enable_output_info_api: bool
    enable_config_info_api: bool
    ovr_slot_count: int = 0
    ovr_buffer_len: int = 0
    generate_validator_header: bool = False


@dataclass
class EcuSpec:
    name: str
    swcs: list
    engine: EngineSpec
    instances: list = field(default_factory=list)
    schedules: Optional[SchedulesSpec] = None
