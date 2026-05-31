# Subtask 03: Earth Model And Earth Data

## Goal

Add an `Earth_Model` implementation of `Celestial_Model`, including radius, total mass, enclosed mass, density, temperature, composition, number densities, escape speed, scattering-rate queries, and interpolation support.

## Scope

Touched files:

- `vendor/damascus/include/Earth_Model.hpp`
- `vendor/damascus/src/Earth_Model.cpp`
- `vendor/damascus/data/earth_prem.dat`
- `src/earth_model_check.cpp`
- `CMakeLists.txt`
- `DOC/earth_model_data_format.md`

Out of scope for this historical subtask:

- Adding the runtime `body = "Earth"` path.
- Adding `config/earth_smoke.cfg`.
- Promoting the first Earth table to production physics data.

## Data Contract

The Earth radial data file is:

```text
vendor/damascus/data/earth_prem.dat
```

Minimum columns:

```text
r_km  enclosed_mass_kg  density_g_cm3  temperature_K
```

The first data version is a coarse PREM-like engineering smoke table. It is meant to validate generalized runtime plumbing and basic physical scales, not final physics analysis.

## Composition Contract

The first composition model is a normalized, three-layer constant-fraction model:

- Core: Fe/Ni/S representative mixture.
- Mantle: O/Mg/Si-dominated silicate mixture.
- Crust: O/Si/Al-rich representative mixture.

Representative isotopes are O-16, Mg-24, Si-28, Fe-56, Ni-58, S-32, Ca-40, and Al-27.

## Acceptance Criteria

- `Earth_Model::Radius()` is about `6371 km`.
- `Earth_Model::Total_Mass()` is about `5.9722e24 kg`.
- `Earth_Model::Mass(0)` is close to zero.
- `Earth_Model::Mass(R_earth)` is close to `M_earth`.
- `Earth_Model::Local_Escape_Speed(R_earth)` is about `11.2 km/s`.
- Number densities and scattering rates outside Earth are zero.
- The model builds and can be constructed by a narrow check target.

## Suggested Validation

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT DaMaSCUS-EarthModelCheck --config Release -j4
./build/DaMaSCUS-EarthModelCheck
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
```

## Completed Work

Completion date: 2026-05-31

- Added `Earth_Model.hpp` and `Earth_Model.cpp`.
- Added `earth_prem.dat` as a smoke-grade radial table.
- Implemented Earth radius, mass, density, temperature, escape speed, target composition, number densities, and scattering rates.
- Added `DaMaSCUS-EarthModelCheck`.
- Added `earth_model_data_format.md`.

## Validation Result

Observed check output included:

```text
radius_km=6371
total_mass_kg=5.9722e+24
mass_center_kg=0
mass_surface_kg=5.9722e+24
surface_escape_speed_km_s=11.186
electron_density_outside=0
targets=8
```

## Final Status Note

At the end of subtask 03, `Earth_Model` was constructible but not yet routed through the main executable. Final runtime support was completed in subtask 04 and validated in `DOC/earth_smoke_validation.md`.
