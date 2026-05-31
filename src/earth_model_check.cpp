#include <iostream>

#include "Earth_Model.hpp"
#include "libphysica/Natural_Units.hpp"

using namespace DaMaSCUS_SUN;
using namespace libphysica::natural_units;

int main()
{
	Earth_Model earth;
	double radius = earth.Radius();
	std::cout << "Earth model: " << earth.Name() << std::endl;
	std::cout << "model_file=" << earth.Model_File() << std::endl;
	std::cout << "radius_km=" << In_Units(radius, km) << std::endl;
	std::cout << "total_mass_kg=" << In_Units(earth.Total_Mass(), kg) << std::endl;
	std::cout << "mass_center_kg=" << In_Units(earth.Mass(0.0), kg) << std::endl;
	std::cout << "mass_surface_kg=" << In_Units(earth.Mass(radius), kg) << std::endl;
	std::cout << "surface_escape_speed_km_s=" << In_Units(earth.Local_Escape_Speed(radius), km / sec) << std::endl;
	std::cout << "density_center_g_cm3=" << In_Units(earth.Mass_Density(0.0), gram / cm / cm / cm) << std::endl;
	std::cout << "density_surface_g_cm3=" << In_Units(earth.Mass_Density(radius), gram / cm / cm / cm) << std::endl;
	std::cout << "electron_density_center=" << earth.Number_Density_Electron(0.0) << std::endl;
	std::cout << "electron_density_outside=" << earth.Number_Density_Electron(2.0 * radius) << std::endl;
	std::cout << "targets=" << earth.Target_Count() << std::endl;
	return 0;
}