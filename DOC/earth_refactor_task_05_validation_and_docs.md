# Subtask 05: Validation, Documentation, And Final Acceptance

## Goal

After Earth smoke runtime support is available, close the work with physical sanity checks, README updates, data-format documentation, and final acceptance records.

## Scope

Touched files:

- `README.md`
- `DOC/earth_model_data_format.md`
- `DOC/sun_smoke_baseline.md`
- `DOC/earth_smoke_validation.md`
- `config/earth_smoke.cfg`
- `config/earth_example.cfg`

Out of scope:

- Changing trajectory TXT columns.
- Introducing a production Earth geochemistry model.
- Renaming the executable.

## Required Earth Checks

- `Earth_Model::Mass(0)` is close to zero.
- `Earth_Model::Mass(R_earth)` is close to `M_earth`.
- `Earth_Model::Local_Escape_Speed(R_earth)` is close to `11.2 km/s`.
- Exterior escape speed follows the `sqrt(2GM/r)` scale.
- `Total_DM_Scattering_Rate` is zero for `r > R_earth`.
- `Number_Density_Electron` is zero for `r > R_earth`.
- Scattering-rate interpolation covers `0 <= r <= R_earth`.
- `Initial_Conditions` scales impact parameters with the active body radius.
- `TrajectoryBincount` uses the configured body-scaled maximum radius.
- `Hyperbolic_Kepler_Shift` uses the active body mass outside the body.

## Required Solar Compatibility Checks

- Old `config/smoke.cfg` still runs.
- Explicit `body = "Sun"` still runs.
- The trajectory TXT columns are unchanged.
- README still provides a valid solar command.
- Omitted `body` keeps the old output directory rule.

## Documentation Requirements

README must cover:

- Supported bodies.
- Solar smoke command.
- Earth smoke command.
- `body`, `initial_radius_body_radius`, `asymptotic_distance_body_radius`, `bincount_max_radius_body_radius`, and `body_model_file`.
- `initial_radius_rsun` compatibility and deprecation guidance.
- Body-prefixed output directories.
- `DAMASCUS_ROOT` requirements for the refactored `Celestial_Model` tree.

`earth_model_data_format.md` must cover:

- `earth_prem.dat` location.
- Columns, units, and validity assumptions.
- Composition layers and representative isotopes.
- Smoke-only limitations.

## Final Validation Commands

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT DaMaSCUS-EarthModelCheck --config Release -j4
./build/DaMaSCUS-EarthModelCheck
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_smoke.cfg
find smoke_output -name 'trajectory_*.txt' | sort
head -5 smoke_output/earth/results_*/trajectory_1_task0.txt
```

## Completed Work

Completion date: 2026-05-31

- README documents Sun and Earth runs, body configuration, output paths, and `DAMASCUS_ROOT` compatibility.
- `earth_model_data_format.md` documents the Earth data format and smoke limitations.
- `earth_smoke_validation.md` records build, Sun smoke, Earth smoke, output files, and boundary checks.
- `sun_smoke_baseline.md` preserves the historical solar baseline and points to the final Earth validation.
- All project-owned Markdown documentation under `DOC/` is now written in English.

## Validation Result

- Build passed.
- Solar and Earth smoke both completed with one MPI rank.
- Explicit `body = "Sun"` wrote to `smoke_output/sun/...`.
- Unknown `body = "Moon"` failed clearly with exit code 1.
- Earth model checks passed: `Mass(0) = 0`, `Mass(R_earth) = 5.9722e+24 kg`, `Local_Escape_Speed(R_earth) = 11.186 km/s`, and `Number_Density_Electron(2 R_earth) = 0`.
- Earth smoke wrote `smoke_output/earth/results_-0.301030_-39.000000/trajectory_1_task0.txt`.
