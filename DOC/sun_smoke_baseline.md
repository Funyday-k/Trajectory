# Solar Smoke Baseline Record

## 1. Baseline Scope

This record belongs to subtask 01. It captures the solar smoke baseline after adding the optional `random_seed` field and the default `body = "Sun"` behavior.

The early-stage Earth failure noted below is historical context only. The final project supports `body = "Earth"`; see `DOC/earth_smoke_validation.md`.

## 2. Environment

- Date: 2026-05-31
- Historical branch: `earth-task-01-baseline-guardrails`
- Executable: `build/DaMaSCUS-SUN-TrajectoryTXT`
- Config: `config/smoke.cfg`
- MPI ranks: 1
- Body: omitted, interpreted as `Sun`
- Seed: `random_seed = 12345`, with per-rank rule `random_seed + mpi_rank`

## 3. Build And Run Commands

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBoost_NO_BOOST_CMAKE=ON \
  -DBoost_NO_SYSTEM_PATHS=ON \
  -DBOOST_ROOT=$HOME/opt/boost_1_77_0 \
  -DBoost_INCLUDE_DIR=$HOME/opt/boost_1_77_0
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
```

## 4. Key Log

```text
DaMaSCUS-SUN-TrajectoryTXT 0.1.0
Trajectory TXT runner
MPI processes: 1
Body: Sun
Output mode: trajectory txt files only
PRNG seed: random_seed + mpi_rank, base random_seed = 12345
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

## 5. Output Artifacts

- Trajectory directory: `smoke_output/results_-0.301030_-39.000000`
- Trajectory files: 1
- File: `trajectory_1_task0.txt`
- Total lines: 10
- Historical MD5: `7ce9aae522dca681d9d9c95611bd685f`

First five lines, shown with tab-separated columns preserved:

```text
# columns: time_s x_km y_km z_km vx_km_s vy_km_s vz_km_s E_eV
0.0000000000e+00	2.6599388430e+05	1.3416790529e+06	-2.5522251583e+05	-1.2551065882e+02	-4.9870526841e+02	1.0306315114e+02	2.3454752304e+02
1.1196703547e+03	1.1423674345e+05	7.2300493701e+05	-1.2858062735e+05	-1.4969716723e+02	-6.3455345792e+02	1.2806017596e+02	2.3454405109e+02
1.7850094203e+03	2.8751820813e+03	2.0478472304e+05	-2.7348813072e+04	-1.8959567718e+02	-1.0725678468e+03	1.9694005830e+02	2.3453989111e+02
1.9144005989e+03	-2.0761059706e+04	4.8126251051e+04	1.6821208406e+02	-1.6058310052e+02	-1.3493630947e+03	2.2378386707e+02	2.3454349529e+02
```

## 6. Historical Compatibility Checks

- `config/smoke.cfg` without `body`: passed, logged `Body: Sun`.
- Temporary explicit `body = "Sun"`: passed.
- Temporary `body = "Earth"`: failed at this historical stage with `Unsupported body: Earth`, as expected before the Earth runtime factory existed.
- Repeating the same `random_seed`: passed with the same trajectory MD5.

## 7. Refresh After Subtasks 04 And 05

Refresh date: 2026-05-31

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT DaMaSCUS-EarthModelCheck --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
```

Result:

- Old `config/smoke.cfg` still omits `body` and is interpreted as `Sun`.
- Output directory is still `smoke_output/results_-0.301030_-39.000000`.
- Trajectory files: 1.
- Total lines: 10.
- Summary: `Captured = 0`, `Free = 1`, `Reflected = 0`, `Aborted = 0`, `Text rows written = 9`, `RK45 steps = 83`.
- Startup log now also includes `Body model = Standard Solar Model AGSS09`, `Body radius = 695700 km`, and `Body mass = 1.98848e+30 kg`.
- Temporary explicit `body = "Sun"` passed and wrote to `smoke_output/sun/results_-0.301030_-39.000000`.

Final Earth status is recorded in `DOC/earth_smoke_validation.md`.
