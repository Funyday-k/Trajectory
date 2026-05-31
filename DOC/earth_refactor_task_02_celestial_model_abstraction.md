# Subtask 02: Abstract The Celestial Model Interface

## Goal

Decouple trajectory propagation, initial-condition sampling, scattering-rate queries, and output helpers from the concrete `Solar_Model` type. After this task, the core simulation code should depend on a generic `Celestial_Model` interface.

## Scope

Expected files:

- `vendor/damascus/include/Celestial_Model.hpp`
- `vendor/damascus/include/Solar_Model.hpp`
- `vendor/damascus/src/Solar_Model.cpp`
- `vendor/damascus/include/Simulation_Utilities.hpp`
- `vendor/damascus/src/Simulation_Utilities.cpp`
- `vendor/damascus/include/Simulation_Trajectory.hpp`
- `vendor/damascus/src/Simulation_Trajectory.cpp`
- `src/main.cpp`

Out of scope for this historical subtask:

- Adding Earth data.
- Adding the runtime Earth factory.
- Renaming the executable.

## Interface Requirements

`Celestial_Model` provides the body-scale and medium queries required by the trajectory code:

```cpp
class Celestial_Model
{
  public:
    virtual ~Celestial_Model() {}

    virtual const std::string& Name() const = 0;
    virtual double Radius() const = 0;
    virtual double Total_Mass() const = 0;

    virtual double Mass(double r) = 0;
    virtual double Mass_Density(double r) = 0;
    virtual double Temperature(double r) = 0;
    virtual double Local_Escape_Speed(double r) = 0;
    virtual double Debye_Screening_Scale_Squared(double r) = 0;

    virtual unsigned int Target_Count() const = 0;
    virtual const obscura::Isotope& Target_Isotope(unsigned int index) const = 0;
    virtual double Number_Density_Nucleus(double r, unsigned int index) = 0;
    virtual double Number_Density_Electron(double r) = 0;

    virtual double DM_Scattering_Rate_Electron(obscura::DM_Particle& DM, double r, double DM_speed) = 0;
    virtual double DM_Scattering_Rate_Nucleus(obscura::DM_Particle& DM, double r, double DM_speed, unsigned int index) = 0;
    virtual double Total_DM_Scattering_Rate(obscura::DM_Particle& DM, double r, double DM_speed) = 0;
    virtual void Interpolate_Total_DM_Scattering_Rate(obscura::DM_Particle& DM, unsigned int N_radius, unsigned int N_speed) = 0;
};
```

## Required Generalization

- `Initial_Conditions` receives an explicit `asymptotic_distance` and uses `body.Radius()`.
- `Hyperbolic_Kepler_Shift` uses `body.Total_Mass()` and `body.Radius()`.
- `Trajectory_Simulator` stores a `Celestial_Model&`.
- `TrajectoryBincount` keeps `NUM_BINS`, but its bin width and maximum radius are runtime values.
- Generic trajectory code no longer uses solar-only constants directly.

## Suggested Validation

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
rg "Solar_Model&|rSun|mSun|R_SUN_KM|BIN_MAX_KM" src vendor/damascus/include vendor/damascus/src
```

Search results may remain in `Solar_Model.*`, solar construction code, and compatibility documentation, but not in generic trajectory algorithms.

## Completed Work

Completion date: 2026-05-31

- Added `Celestial_Model.hpp`.
- Updated `Solar_Model` to implement `Celestial_Model`.
- Generalized `Simulation_Utilities` and `Simulation_Trajectory` signatures to `Celestial_Model&`.
- Parameterized initial conditions, exterior Kepler propagation, and bincount radius handling.
- Preserved solar smoke behavior.

## Historical Stage Result

At the end of subtask 02, the main program still constructed only `Solar_Model`; the runtime Earth factory was intentionally left for subtask 04. That statement is historical only. The final project now supports `body = "Earth"`; see `DOC/earth_smoke_validation.md`.
