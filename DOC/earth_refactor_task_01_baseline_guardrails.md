# Subtask 01: Solar Baseline And Compatibility Guardrails

## Goal

Establish a repeatable solar smoke baseline without changing the existing solar physics behavior. This task also reserves the future `body` configuration entry point while keeping omitted `body` equivalent to the original solar runner.

## Scope

Touched or expected files:

- `src/main.cpp`
- `config/smoke.cfg`
- `README.md`
- `DOC/sun_smoke_baseline.md`

Out of scope for this historical subtask:

- Adding `Earth_Model`.
- Changing trajectory output columns.
- Changing output directory rules.
- Renaming the executable.

## Configuration Rules

The stage-01 contract was:

```text
body = "Sun";
random_seed = 12345;
```

- Omitted `body` defaults to `Sun`.
- `body = "Sun"` is accepted explicitly.
- Unknown bodies fail with a clear `Unsupported body: <value>` message.
- `random_seed` is optional; when present, each MPI rank uses `random_seed + mpi_rank`.
- `initial_radius_rsun` remains the solar boundary field for this stage.

## Acceptance Criteria

- Old `config/smoke.cfg` still runs without a `body` setting.
- Explicit `body = "Sun"` still runs.
- The trajectory TXT format is unchanged.
- A reproducible smoke record exists.
- The README documents the default `Sun` behavior.

## Suggested Validation

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
```

## Completed Work

Completion date: 2026-05-31

- Added `random_seed = 12345` to `config/smoke.cfg`.
- Added `body` parsing with default `Sun` behavior.
- Preserved the omitted-body solar output path.
- Added `DOC/sun_smoke_baseline.md` with the historical solar smoke baseline.

## Historical Stage Result

At the end of subtask 01, `body = "Earth"` intentionally failed because Earth runtime support had not been implemented yet. That result is kept as historical context only.

Final project state: `body = "Earth"` is supported after subtasks 04 and 05. See `DOC/earth_smoke_validation.md` for the final Earth smoke validation.
