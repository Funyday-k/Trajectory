# Earth Refactor Subtask Overview

This document breaks the Earth simulation refactor into independently reviewable subtasks. The implementation order is: preserve the existing solar smoke path, introduce the generalized celestial-model interface, add the Earth model and data, connect the runtime `body = "Earth"` path, then close with validation and documentation.

## Review Updates

The subtask boundaries remain the same, but the final delivery must satisfy these constraints:

1. Solar smoke checks must remain reproducible, including the optional `random_seed` rule.
2. The trajectory layer must depend on `Celestial_Model`, not `Solar_Model`, and runtime distances must be body-scaled where appropriate.
3. The Earth data must be clearly marked as engineering smoke data, not production physics input.
4. Legacy solar output paths must remain stable when `body` is omitted.
5. Validation must be recorded with commands and observed values, not only a checklist.

## Overall Goal

The first refactor round turns the trajectory TXT runner from a solar-only path into a configurable body runner. The supported final state is:

- `body = "Sun"`: preserve the existing solar behavior.
- `body = "Earth"`: run the same trajectory, scattering, MPI, and TXT-output framework with Earth radius, mass, radial structure, composition, and escape speed.
- Omitted `body`: keep legacy solar behavior and legacy output directory layout.

## Subtasks

| Order | Subtask | Goal | Main outputs |
| --- | --- | --- | --- |
| 01 | `earth_refactor_task_01_baseline_guardrails.md` | Establish the solar baseline and compatibility rules | Solar smoke record, default `Sun` policy, seed rule |
| 02 | `earth_refactor_task_02_celestial_model_abstraction.md` | Decouple trajectory code from `Solar_Model` | `Celestial_Model.hpp`, generalized utility and trajectory signatures |
| 03 | `earth_refactor_task_03_earth_model_data.md` | Add Earth model data and interfaces | `Earth_Model.hpp/.cpp`, `earth_prem.dat`, model check target |
| 04 | `earth_refactor_task_04_runtime_integration.md` | Route runtime config to Sun or Earth | Body factory, Earth configs, body-prefixed output directories |
| 05 | `earth_refactor_task_05_validation_and_docs.md` | Close validation and user documentation | README updates, data-format docs, smoke validation record |

## Cross-Task Rules

- Keep C++11 compatibility.
- Keep the executable name `DaMaSCUS-SUN-TrajectoryTXT` for this refactor round.
- Keep `initial_radius_rsun` compatible for solar configs; prefer `initial_radius_body_radius` in new configs.
- Keep the trajectory TXT columns unchanged: `time_s x_km y_km z_km vx_km_s vy_km_s vz_km_s E_eV`.
- Keep the old solar output directory rule when `body` is omitted.
- Use body-prefixed output directories only when `body` is explicit.
- Mark the first Earth data and composition model as smoke-grade.
- Run at least one focused validation command after each implementation stage.

## Milestones

### M1: Solar Compatibility Baseline

- `config/smoke.cfg` runs without adding `body`.
- README states that the default body is `Sun`.
- A reproducible solar smoke baseline is recorded.

### M2: General Celestial Interface

- Core trajectory and utility code accepts `Celestial_Model&`.
- The solar smoke path still passes.
- Generic trajectory code no longer depends on `rSun`, `mSun`, `R_SUN_KM`, or a fixed solar bin maximum.

### M3: Earth Model Construction

- `Earth_Model` loads the radial Earth table.
- `Mass(R_earth)` and `Local_Escape_Speed(R_earth)` have the expected order of magnitude.
- The Earth data precision level is documented.

### M4: Earth Smoke Runtime

- `config/earth_smoke.cfg` completes a 1-rank run.
- Earth output is grouped under an `earth/` directory and does not mix with legacy solar output.

### M5: Documentation And Physical Checks

- README includes Sun and Earth run commands.
- `earth_model_data_format.md` describes the Earth data fields and units.
- `earth_smoke_validation.md` records the final Sun/Earth smoke checks.
