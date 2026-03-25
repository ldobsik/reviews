# SwcIf Full Package (Updated schema examples)

This package contains the static SwcIf core, a YAML/Jinja generator, example SWCs, example ECUs,
and pre-generated outputs demonstrating:

- fully reflectable inputs / outputs / config (`DRLPOS`)
- opaque output domain with no input/config domain (`LAMPDRV`)
- a SWC with no input/output/config domains at all (`HEARTBEAT`)

## Generate RearLampECU
```bash
python3 generator/generate.py --root . --ecu ecus/RearLampECU/RearLampECU.yaml --out generated/RearLampECU
```

## Generate HeartbeatECU
```bash
python3 generator/generate.py --root . --ecu ecus/HeartbeatECU/HeartbeatECU.yaml --out generated/HeartbeatECU
```
