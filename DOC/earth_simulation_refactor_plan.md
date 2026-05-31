# Earth Simulation Refactor Plan

## 1. Background And Goal

The project is a trajectory TXT runner derived from DaMaSCUS-SUN. The main executable samples dark-matter initial conditions, propagates particles to an outer boundary, applies medium scattering, classifies capture/free/reflection outcomes, and writes one TXT file per simulated trajectory.

The refactor goal is to extend the runner from a solar-only simulation to a configurable body simulation. The first supported bodies are `Sun` and `Earth`.

Short-term goal:

- Add `Earth_Model` and run Earth smoke trajectories with minimal changes to the core algorithms.

Long-term direction:

- Keep `Solar_Model` and `Earth_Model` as implementations of a common `Celestial_Model` interface so future bodies or table-driven models can be added without copying the trajectory engine.

## 2. Starting Point

The original runtime path was:

1. Read a libconfig configuration file.
2. Construct a dark-matter particle.
3. Construct a dark-matter velocity distribution.
4. Construct `Solar_Model`.
5. Interpolate total scattering rates.
6. Simulate trajectories with MPI ranks.
7. Write `trajectory_<id>_task<rank>.txt` files.

The trajectory TXT columns remain:

```text
time_s  x_km  y_km  z_km  vx_km_s  vy_km_s  vz_km_s  E_eV
```

## 3. Solar Coupling Points

The original solar-only assumptions included:

- Concrete `Solar_Model` type in trajectory and utility signatures.
- Fixed solar radius and mass constants in generic propagation code.
- Solar data file `model_agss09.dat`.
- Config field `initial_radius_rsun`.
- Solar-scale radial bins.
- Exterior Kepler propagation using solar mass.
- Output and README wording centered on DaMaSCUS-SUN.

The refactor removes those assumptions from generic trajectory code while preserving the solar model implementation itself.

## 4. Earth Model Inputs

The first Earth model requires:

- Earth radius, using `6371 km` for the smoke model.
- Earth mass, using `5.9722e24 kg`.
- A radial table with radius, enclosed mass, density, and temperature.
- A composition model for target nuclei and electron density.
- Escape speed inside and outside Earth.
- Scattering-rate interpolation over `0 <= r <= R_earth`.

The bundled data file is:

```text
vendor/damascus/data/earth_prem.dat
```

The file is smoke-grade and should not be used for final physics analysis.

## 5. Architecture

### 5.1 Common Interface

`Celestial_Model` provides body properties and medium queries:

- Name, radius, and total mass.
- Enclosed mass, density, temperature, escape speed, and Debye scale.
- Target isotope information and number densities.
- Electron and nuclear scattering rates.
- Total scattering-rate interpolation.

### 5.2 Model Implementations

- `Solar_Model` implements `Celestial_Model` using the existing AGSS09 solar data.
- `Earth_Model` implements `Celestial_Model` using the smoke PREM-like table and a layered composition model.

### 5.3 Runtime Factory

`src/main.cpp` chooses the model from config:

```text
body = "Sun";
body = "Earth";
```

Omitted `body` defaults to `Sun`.

## 6. Runtime Configuration

General fields:

```text
body = "Earth";
initial_radius_body_radius = 2.0;
asymptotic_distance_body_radius = 1000.0;
bincount_max_radius_body_radius = 2.0;
random_seed = 12345;
```

Earth-specific fields:

```text
body_model_file = "vendor/damascus/data/earth_prem.dat";
body_composition = "layered";
```

Solar compatibility field:

```text
initial_radius_rsun = 2.0;
```

`initial_radius_rsun` remains supported for solar configs. New configs should prefer `initial_radius_body_radius`.

## 7. Output Layout

When `body` is omitted, legacy solar configs keep the old path:

```text
<output_dir>/results_<log10_dm_mass>_<log10_cross_section>/trajectory_<id>_task<rank>.txt
```

When `body` is explicit, output is body-prefixed:

```text
<output_dir>/<body_lowercase>/results_<log10_dm_mass>_<log10_cross_section>/trajectory_<id>_task<rank>.txt
```

Examples:

```text
smoke_output/results_-0.301030_-39.000000/trajectory_1_task0.txt
smoke_output/sun/results_-0.301030_-39.000000/trajectory_1_task0.txt
smoke_output/earth/results_-0.301030_-39.000000/trajectory_1_task0.txt
```

## 8. CMake And External Roots

The refactored trajectory code depends on `Celestial_Model.hpp` unconditionally. Therefore `DAMASCUS_ROOT`, when set, must point to this refactored DaMaSCUS source tree or another compatible tree that contains `include/Celestial_Model.hpp`.

Older unrefactored external DaMaSCUS source trees are not compatible, even when Earth support is disabled.

Earth support itself is optional and enabled only when these files exist under `DAMASCUS_ROOT`:

```text
include/Earth_Model.hpp
src/Earth_Model.cpp
data/earth_prem.dat
```

## 9. Subtask Plan

The work is split into five task books:

1. `DOC/earth_refactor_subtasks_overview.md`: overview, boundaries, and milestones.
2. `DOC/earth_refactor_task_01_baseline_guardrails.md`: solar baseline and compatibility guardrails.
3. `DOC/earth_refactor_task_02_celestial_model_abstraction.md`: generalized `Celestial_Model` interface.
4. `DOC/earth_refactor_task_03_earth_model_data.md`: `Earth_Model` and Earth data.
5. `DOC/earth_refactor_task_04_runtime_integration.md`: runtime body selection and Earth configs.
6. `DOC/earth_refactor_task_05_validation_and_docs.md`: final validation and documentation.

The final validation record is `DOC/earth_smoke_validation.md`.

## 10. Final Acceptance State

The final state for this refactor round is:

- `body = "Sun"` runs.
- Omitted `body` keeps the old solar behavior and old output path.
- `body = "Earth"` runs through the main executable.
- Earth smoke writes under `smoke_output/earth/...`.
- The Earth model check reports surface escape speed near `11.2 km/s`.
- README documents Sun and Earth commands.
- Project-owned Markdown documentation under `DOC/` is in English.
