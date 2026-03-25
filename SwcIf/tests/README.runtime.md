# Runtime Smoke Harness

This harness regenerates each example ECU, compiles it with the **system `gcc`**, links a tiny per-ECU test program, and executes that program.

## Assumptions

- The repository already contains the required **manual files** and example support dependencies under `manual/<ECU>/`.
- If a manual per-instance binder is present, the harness uses it.
- If a manual binder is absent, the harness falls back to the generated stub binder.

## What it checks

- **HeartbeatECU**
  - `SwcIf_Init()` succeeds
  - `SwcIf_MainFunction()` runs once
  - `SWCIF_SERVER(HEARTBEAT_0, HEARTBEAT_GetTicks)()` increments from `0` to `1`

- **RearLampECU**
  - `SwcIf_Init()` succeeds
  - output pointer helper for `LAMPDRV_0` is non-null
  - `DRLPOS_GetStatus()` is callable before and after two scheduler steps
  - `DRLPOS_ForceOff()` server closure is callable

- **SingletonDemoECU**
  - `SwcIf_Init()` succeeds
  - singleton config pointer is wired (`FILTER_cfg != NULL`)
  - singleton output pointer helpers return the singleton globals (`&tx`, `&IO`)
  - the bound singleton reset callback clears `tx.y` back to `0.0f`

This is still a **smoke test**, not a full behavioral validation suite.

## Usage

From the repository root:

```bash
python3 tests/runtime_smoke.py
```

or:

```bash
sh tests/runtime_smoke.sh
```

To run just one ECU:

```bash
python3 tests/runtime_smoke.py --ecu SingletonDemoECU
```
