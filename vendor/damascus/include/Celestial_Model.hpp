#ifndef __Celestial_Model_hpp_
#define __Celestial_Model_hpp_

#include <string>

#include "obscura/DM_Particle.hpp"
#include "obscura/Target_Nucleus.hpp"

namespace DaMaSCUS_SUN
{

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

extern double Thermal_Averaged_Relative_Speed(double temperature, double mass_target, double v_DM);

} // namespace DaMaSCUS_SUN

#endif