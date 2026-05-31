# Subtask 04: Runtime Earth Integration

## Goal

Connect `Earth_Model` to the main executable and configuration system so users can run `body = "Earth"` while preserving `body = "Sun"` and old solar configs.

## Scope

Touched files:

- `src/main.cpp`
- `vendor/damascus/include/Simulation_Trajectory.hpp`
- `vendor/damascus/src/Simulation_Trajectory.cpp`
- `config/earth_smoke.cfg`
- `config/earth_example.cfg`
- `CMakeLists.txt`
- `README.md`

Out of scope:

- Improving the Earth composition model beyond smoke quality.
- Renaming the executable.
- Changing trajectory TXT columns.
- Removing `initial_radius_rsun` immediately.

## Configuration

Earth configuration fields:

```text
body = "Earth";
initial_radius_body_radius = 2.0;
asymptotic_distance_body_radius = 1000.0;
bincount_max_radius_body_radius = 2.0;
body_model_file = "vendor/damascus/data/earth_prem.dat";
body_composition = "layered";
random_seed = 12345;
```

Compatibility strategy:

- Omitted `body` defaults to `Sun` and keeps the legacy output directory rule.
- `body = "Sun"` accepts `initial_radius_rsun` first, with `initial_radius_body_radius` as the new preferred field.
- `body = "Earth"` uses `initial_radius_body_radius` and ignores `initial_radius_rsun`.
- Explicit `body` values use body-prefixed output directories.

## Implementation Summary

- Added a body factory in `main.cpp` that returns `Solar_Model` or `Earth_Model` through `std::unique_ptr<Celestial_Model>`.
- Routed scattering-rate interpolation, initial conditions, Kepler shifting, and TXT trajectory simulation through `Celestial_Model&`.
- Scaled the initial boundary, asymptotic sampling distance, and bincount maximum radius by the active body radius.
- Added startup logging for body name, model name, radius, mass, Earth model file, and composition.
- Added `config/earth_smoke.cfg` and `config/earth_example.cfg`.
- Preserved legacy omitted-body solar output paths.
- Restored the old no-argument `Trajectory_Result::Particle_Reflected()` as a solar-compatible wrapper to avoid downstream link failures.
- Updated CMake so Earth support is enabled only when `Earth_Model.hpp`, `Earth_Model.cpp`, and `earth_prem.dat` are present.
- Added a required CMake check for `Celestial_Model.hpp`, because the refactored trajectory code depends on it even when Earth support is disabled.

## Output Directory Rule

Legacy omitted-body solar config:

```text
<output_dir>/results_<log10_dm_mass>_<log10_cross_section>/trajectory_<id>_task<rank>.txt
```

Explicit body config:

```text
<output_dir>/<body_lowercase>/results_<log10_dm_mass>_<log10_cross_section>/trajectory_<id>_task<rank>.txt
```

Examples:

```text
smoke_output/results_-0.301030_-39.000000/trajectory_1_task0.txt
smoke_output/sun/results_-0.301030_-39.000000/trajectory_1_task0.txt
smoke_output/earth/results_-0.301030_-39.000000/trajectory_1_task0.txt
```

## Validation Result

Completed on 2026-05-31.

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT DaMaSCUS-EarthModelCheck --config Release -j4
./build/DaMaSCUS-EarthModelCheck
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_smoke.cfg
```

Observed results:

- Build passed.
- `DaMaSCUS-EarthModelCheck` reported `surface_escape_speed_km_s=11.186`.
- Legacy `config/smoke.cfg` wrote to `smoke_output/results_-0.301030_-39.000000`.
- `config/earth_smoke.cfg` wrote to `smoke_output/earth/results_-0.301030_-39.000000`.
- A temporary explicit `body = "Sun"` config wrote to `smoke_output/sun/results_-0.301030_-39.000000`.
- A temporary `body = "Moon"` config failed with `Unsupported body: Moon` and exit code 1.
