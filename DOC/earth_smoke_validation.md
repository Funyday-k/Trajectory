# Earth Smoke Validation

## 1. Scope

This record closes subtasks 04 and 05 for the first Earth smoke path. The bundled Earth model is an engineering smoke model only.

## 2. Environment

- Date: 2026-05-31
- Executable: `build/DaMaSCUS-SUN-TrajectoryTXT`
- Earth config: `config/earth_smoke.cfg`
- Solar compatibility config: `config/smoke.cfg`
- MPI ranks: 1
- Random seed: `12345`, with per-rank rule `random_seed + mpi_rank`

## 3. Commands

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT DaMaSCUS-EarthModelCheck --config Release -j4
./build/DaMaSCUS-EarthModelCheck
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_smoke.cfg
find smoke_output -name 'trajectory_*.txt' -print | sort
head -5 smoke_output/earth/results_-0.301030_-39.000000/trajectory_1_task0.txt
```

## 4. Earth Model Check

```text
radius_km=6371
total_mass_kg=5.9722e+24
mass_center_kg=0
mass_surface_kg=5.9722e+24
surface_escape_speed_km_s=11.186
density_center_g_cm3=13.1
density_surface_g_cm3=2.7
electron_density_center=2.83998e-17
electron_density_outside=0
targets=8
```

Judgement: pass for smoke scope. Surface escape speed is close to `11.2 km/s`, enclosed surface mass equals the configured Earth mass, and outside electron density is zero.

## 5. Solar Compatibility Smoke

```text
Body: Sun
Body model: Standard Solar Model AGSS09
Body radius [km]: 695700
Body mass [kg]: 1.98848e+30
Trajectory directory: ./smoke_output/results_-0.301030_-39.000000
Trajectory files: 1
Captured: 0
Free: 1
Reflected: 0
Aborted: 0
Text rows written: 9
RK45 steps: 83
EARLY STOP: max_trajectories reached
```

Judgement: pass. The omitted-body legacy solar config keeps the old output directory rule.

## 6. Earth Smoke

```text
Body: Earth
Body model: Simplified Earth PREM smoke model
Body radius [km]: 6371
Body mass [kg]: 5.9722e+24
Body model file: vendor/damascus/data/earth_prem.dat
Body composition: layered
Trajectory directory: ./smoke_output/earth/results_-0.301030_-39.000000
Trajectory files: 1
Captured: 0
Free: 1
Reflected: 0
Aborted: 0
Text rows written: 7
RK45 steps: 63
EARLY STOP: max_trajectories reached
```

Judgement: pass. `body = "Earth"` runs through the main executable and writes to the Earth-prefixed output directory.

## 7. Output Files

```text
smoke_output/earth/results_-0.301030_-39.000000/trajectory_1_task0.txt
smoke_output/results_-0.301030_-39.000000/trajectory_1_task0.txt
smoke_output/sun/results_-0.301030_-39.000000/trajectory_1_task0.txt
```

Earth trajectory line count: 8.

First five Earth trajectory lines, shown with tab-separated columns preserved:

```text
# columns: time_s x_km y_km z_km vx_km_s vy_km_s vz_km_s E_eV
0.0000000000e+00	2.7548839393e+03	1.2202096513e+04	-2.4244626556e+03	-7.3497883262e+01	-2.7495467412e+02	5.8145265066e+01	2.3454752304e+02
3.1085729455e+01	4.6970066436e+02	3.6527114844e+03	-6.1655016838e+02	-7.3533415519e+01	-2.7514609170e+02	5.8180934027e+01	2.3454684617e+02
4.1265307724e+01	-2.7887389545e+02	8.5138572332e+02	-2.4223159059e+01	-7.3536392966e+01	-2.7522193874e+02	5.8191651764e+01	2.3454797284e+02
4.3538455975e+01	-4.4603040618e+02	2.2575980284e+02	1.0805501902e+02	-7.3533812883e+01	-2.7522578840e+02	5.8191355441e+01	2.3454750177e+02
```

## 8. Explicit Body And Error Paths

Explicit `body = "Sun"` was checked with a temporary config derived from `config/smoke.cfg`:

```text
Body: Sun
Trajectory directory: ./smoke_output/sun/results_-0.301030_-39.000000
Trajectory files: 1
Captured: 0
Free: 1
Reflected: 0
Aborted: 0
```

Unknown body handling was checked with temporary `body = "Moon"`:

```text
Unsupported body: Moon
bad body rejected with exit code 1
```

Judgement: pass. Explicit body configs use body-prefixed output directories, while unknown bodies fail before model construction.

## 9. Known Limits

- `earth_prem.dat` is coarse smoke data and should not be used for final physics analysis.
- The first Earth composition model is a three-layer constant-fraction model.
- This smoke run stops after one simulated trajectory because `max_trajectories = 1`; it validates runtime integration, not capture statistics.
