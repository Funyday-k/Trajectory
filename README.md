# DaMaSCUS-SUN Trajectory

Trajectory-only runner for DaMaSCUS-SUN. The executable writes each simulated trajectory to plain text files.

## Requirements

- CMake 3.21.2
- Boost 1.77.0
- Intel oneAPI 2022.2 with Intel C++ 2022.1.0
- Intel MPI 2021.6.0
- A C++11 build mode
- libconfig++

Boost is pinned to 1.77.0 because newer Boost.Math releases require C++14, while this DaMaSCUS/libphysica build uses C++11.

## Install Dependencies

### Linux

With environment modules:

```bash
module purge
module load cmake/3.21.2
module load intel-oneapi/2022.2
module load intel-mpi/2021.6.0
module load boost/1.77.0
module load libconfig
```

Without modules:

```bash
mkdir -p $HOME/opt
cd $HOME/opt
curl -L -o cmake-3.21.2-linux-x86_64.tar.gz https://github.com/Kitware/CMake/releases/download/v3.21.2/cmake-3.21.2-linux-x86_64.tar.gz
tar -xzf cmake-3.21.2-linux-x86_64.tar.gz
export PATH=$HOME/opt/cmake-3.21.2-linux-x86_64/bin:$PATH

curl -L -o boost_1_77_0.tar.gz https://archives.boost.io/release/1.77.0/source/boost_1_77_0.tar.gz
tar -xzf boost_1_77_0.tar.gz
export BOOST_ROOT=$HOME/opt/boost_1_77_0
```

Install libconfig++ with the system package manager:

```bash
# Ubuntu or Debian
sudo apt update
sudo apt install -y libconfig++-dev

# RHEL, Rocky Linux, or Fedora
sudo dnf install -y libconfig-devel
```

Initialize Intel oneAPI if it is installed under the default prefix:

```bash
source /opt/intel/oneapi/setvars.sh
```

### macOS

Install CMake, libconfig++, and MPI with Homebrew:

```bash
brew install cmake libconfig open-mpi
```

Install Boost 1.77.0 as a header prefix:

```bash
mkdir -p $HOME/opt
cd $HOME/opt
curl -L -o boost_1_77_0.tar.gz https://archives.boost.io/release/1.77.0/source/boost_1_77_0.tar.gz
tar -xzf boost_1_77_0.tar.gz
export BOOST_ROOT=$HOME/opt/boost_1_77_0
```

Check that the expected tools are active:

```bash
cmake --version
icpx --version
mpirun --version
```

On Linux with Intel MPI, also check:

```bash
mpiicpc -show
```

## Build

`DAMASCUS_ROOT`, when set, must point to this refactored DaMaSCUS source tree or another tree that contains the same generalized trajectory interfaces, including `include/Celestial_Model.hpp`, `include/Simulation_Trajectory.hpp`, `src/Simulation_Trajectory.cpp`, and `src/Simulation_Utilities.cpp`. Older unrefactored DaMaSCUS source trees are not compatible with this wrapper even when Earth support is disabled, because the trajectory code now depends on `Celestial_Model.hpp` unconditionally.

Earth support is enabled automatically when `Earth_Model.hpp`, `Earth_Model.cpp`, and `earth_prem.dat` are present under `DAMASCUS_ROOT`. If those Earth files are absent, the project can still build the solar path as long as `DAMASCUS_ROOT` is the refactored tree with `Celestial_Model.hpp`.

Linux with Intel MPI:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_COMPILER=mpiicc \
	-DCMAKE_CXX_COMPILER=mpiicpc \
	-DBoost_NO_BOOST_CMAKE=ON \
	-DBoost_NO_SYSTEM_PATHS=ON \
	-DBOOST_ROOT=$BOOST_ROOT \
	-DBoost_INCLUDE_DIR=$BOOST_ROOT
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
```

macOS with Homebrew MPI:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
	-DBoost_NO_BOOST_CMAKE=ON \
	-DBoost_NO_SYSTEM_PATHS=ON \
	-DBOOST_ROOT=$BOOST_ROOT \
	-DBoost_INCLUDE_DIR=$BOOST_ROOT
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
```

## Run

Supported bodies are `Sun` and `Earth`. Existing solar configs do not need a `body` setting; if omitted, the executable behaves as the original solar runner. `body = "Sun"` and `body = "Earth"` are both accepted explicitly. For reproducible smoke checks, set optional `random_seed`; when present, each MPI rank uses `random_seed + mpi_rank`.

Solar smoke test:

```bash
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
head -5 smoke_output/results_*/trajectory_1_task0.txt
```

Earth smoke test:

```bash
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_smoke.cfg
head -5 smoke_output/earth/results_*/trajectory_1_task0.txt
```

Normal solar run:

```bash
mpirun -np 4 ./build/DaMaSCUS-SUN-TrajectoryTXT config/example.cfg
```

Normal Earth run:

```bash
mpirun -np 4 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_example.cfg
```

Inside a SLURM job, for example with 32 MPI processes:

```bash
mpirun -np 32 ./build/DaMaSCUS-SUN-TrajectoryTXT config/example.cfg
```

### Body Configuration

Common fields:

```text
body = "Sun";                         // optional, defaults to Sun when omitted
initial_radius_body_radius = 2.0;      // outer simulation boundary in current body radii
asymptotic_distance_body_radius = 1000.0;
bincount_max_radius_body_radius = 2.0;
random_seed = 12345;                   // optional, seed is offset by MPI rank
```

Solar compatibility fields:

```text
initial_radius_rsun = 2.0;
```

`initial_radius_rsun` remains supported for solar configs and takes precedence over `initial_radius_body_radius` when `body` is `Sun`. New configs should prefer `initial_radius_body_radius`. If `asymptotic_distance_body_radius` is omitted for solar configs, the legacy `1000 AU` asymptotic distance is retained.

Earth-specific fields:

```text
body = "Earth";
body_model_file = "vendor/damascus/data/earth_prem.dat";
body_composition = "layered";
```

The bundled `earth_prem.dat` and `layered` composition are smoke-grade engineering data. They are suitable for testing the generalized trajectory path, not for final physics results. Earth support is enabled automatically when `Earth_Model.hpp`, `Earth_Model.cpp`, and `earth_prem.dat` are present under `DAMASCUS_ROOT`; otherwise the project builds the solar path and `body = "Earth"` reports that Earth support was not built. In all cases, `DAMASCUS_ROOT` must still contain `Celestial_Model.hpp` from this refactor.

### Output

The output directory is controlled by `output_dir` in the config file. When `body` is omitted, old solar configs keep the legacy path:

```text
smoke_output/results_<log10_dm_mass>_<log10_cross_section>/trajectory_1_task0.txt
```

When `body` is explicit, the parameter directory is grouped by body:

```text
smoke_output/sun/results_<log10_dm_mass>_<log10_cross_section>/trajectory_1_task0.txt
smoke_output/earth/results_<log10_dm_mass>_<log10_cross_section>/trajectory_1_task0.txt
```

Trajectory file names look like:

```text
trajectory_1_task0.txt
trajectory_2_task0.txt
trajectory_1_task1.txt
```

Each `.txt` file has these columns:

```text
time_s  x_km  y_km  z_km  vx_km_s  vy_km_s  vz_km_s  E_eV
```

