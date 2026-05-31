# Earth Model Data Format

## 1. File Location

The first Earth model table is stored at:

```text
vendor/damascus/data/earth_prem.dat
```

## 2. Columns

The table is whitespace-separated and allows `#` comments:

```text
r_km  enclosed_mass_kg  density_g_cm3  temperature_K
```

- `r_km`: radial coordinate in kilometers, strictly increasing from `0` to `6371`.
- `enclosed_mass_kg`: mass enclosed inside `r_km`, in kilograms.
- `density_g_cm3`: local mass density in `g/cm^3`.
- `temperature_K`: local temperature in Kelvin. Values must be positive.

`Earth_Model` converts these columns to natural units at load time and builds interpolation tables for enclosed mass, mass density, temperature, and local escape speed.

## 3. Current Data Status

The current `earth_prem.dat` is a coarse PREM-like engineering smoke table with hand-rounded values. It is meant to validate the generalized trajectory and scattering framework, not to produce final physics results.

Before production use, replace it with a documented PREM-derived table and verify the enclosed mass integral, layer boundaries, and temperature model.

## 4. Composition Model

The first Earth composition model is a normalized, three-layer constant mass-fraction model:

| Layer | Radius Range | Notes |
| --- | --- | --- |
| Core | `0 <= r <= 3480 km` | Fe/Ni/S representative mixture |
| Mantle | `3480 < r <= 6346 km` | O/Mg/Si-dominated silicate mixture |
| Crust | `6346 < r <= 6371 km` | O/Si/Al-rich representative crust mixture |

The target list uses one representative isotope per element: O-16, Mg-24, Si-28, Fe-56, Ni-58, S-32, Ca-40, and Al-27. The layer fractions are normalized inside `Earth_Model` and should be treated as smoke-level approximations.

## 5. Sanity Checks

The lightweight check target prints basic values:

```bash
cmake --build build --target DaMaSCUS-EarthModelCheck --config Release -j4
./build/DaMaSCUS-EarthModelCheck
```

Expected order-of-magnitude checks:

- `radius_km` is about `6371`.
- `total_mass_kg` and `mass_surface_kg` are about `5.9722e24`.
- `surface_escape_speed_km_s` is about `11.2`.
- `electron_density_center` is positive.
- `electron_density_outside` is `0`.