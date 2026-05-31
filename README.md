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

The current default body is `Sun`. Existing solar configs do not need a `body` setting; if omitted, the executable behaves as the solar runner. `body = "Sun"` is accepted explicitly, and other body names are reserved for later refactor tasks. For reproducible smoke checks, set optional `random_seed`; when present, each MPI rank uses `random_seed + mpi_rank`.

Smoke test:

```bash
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
head -5 smoke_output/results_*/trajectory_1_task0.txt
```

Normal run:

```bash
mpirun -np 4 ./build/DaMaSCUS-SUN-TrajectoryTXT config/example.cfg
```

Inside a SLURM job, for example with 32 MPI processes:

```bash
mpirun -np 32 ./build/DaMaSCUS-SUN-TrajectoryTXT config/example.cfg
```

The output directory is controlled by `output_dir` in the config file. Trajectory file names look like:

```text
trajectory_1_task0.txt
trajectory_2_task0.txt
trajectory_1_task1.txt
```

Each `.txt` file has these columns:

```text
time_s  x_km  y_km  z_km  vx_km_s  vy_km_s  vz_km_s  E_eV
```

