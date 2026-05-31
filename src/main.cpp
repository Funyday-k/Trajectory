#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <libconfig.h++>
#include <limits>
#include <memory>
#include <mpi.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "libphysica/Natural_Units.hpp"
#include "libphysica/Special_Functions.hpp"
#include "libphysica/Statistics.hpp"
#include "libphysica/Utilities.hpp"

#include "obscura/DM_Distribution.hpp"
#include "obscura/DM_Halo_Models.hpp"
#include "obscura/DM_Particle.hpp"
#include "obscura/DM_Particle_Standard.hpp"

#include "Dark_Photon.hpp"
#include "Celestial_Model.hpp"
#if DAMASCUS_HAS_EARTH_MODEL
#include "Earth_Model.hpp"
#endif
#include "Simulation_Trajectory.hpp"
#include "Simulation_Utilities.hpp"
#include "Solar_Model.hpp"
#include "version.hpp"

#ifndef DAMASCUS_HAS_EARTH_MODEL
#define DAMASCUS_HAS_EARTH_MODEL 0
#endif

using namespace DaMaSCUS_SUN;
using namespace libconfig;
using namespace libphysica::natural_units;

namespace
{
struct Run_Config
{
	std::string body = "Sun";
	bool body_configured = false;
	std::string body_model_file;
	std::string body_composition = "layered";
	std::string output_dir;
	unsigned int sample_size = 1000;
	unsigned int interpolation_points = 1000;
	unsigned long int max_trajectories = 0;
	unsigned long int maximum_number_of_scatterings = DEFAULT_MAXIMUM_SCATTERINGS;
	unsigned long int maximum_free_time_steps = DEFAULT_MAXIMUM_FREE_TIME_STEPS;
	double initial_radius_body_radius = 2.0;
	double initial_radius = 0.0;
	double asymptotic_distance = 1000.0 * AU;
	double asymptotic_distance_body_radius = 0.0;
	double bincount_max_radius_body_radius = 2.0;
	double bincount_max_radius = 0.0;
	double max_trajectory_wall_time_sec = 300.0;
	bool capture_mode = false;
	bool clear_existing_trajectories = true;
	unsigned int trajectory_write_stride = 1;
	int txt_precision = 10;
	bool random_seed_configured = false;
	int random_seed = 0;
};

struct Trajectory_Stats
{
	bool captured = false;
	bool aborted = false;
	bool free_particle = false;
	bool reflected_particle = false;
	unsigned long int scatterings = 0;
	unsigned long int rk45_steps = 0;
	unsigned long int rows_written = 0;
};

enum class Free_Propagation_Result
{
	Scatter,
	Escape,
	CaptureStop,
	StepLimit,
	Abort
};

std::string Join_Path(const std::string& directory, const std::string& name)
{
	if(directory.empty() || directory.back() == '/')
		return directory + name;
	return directory + "/" + name;
}

bool Has_Prefix(const std::string& text, const std::string& prefix)
{
	return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool Has_Suffix(const std::string& text, const std::string& suffix)
{
	return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void Ensure_Directory_Exists(const std::string& directory)
{
	if(directory.empty())
		return;

	std::string current;
	for(size_t index = 0; index < directory.size(); index++)
	{
		current.push_back(directory[index]);
		if(directory[index] != '/' || current.size() <= 1)
			continue;
		mkdir(current.c_str(), 0755);
	}
	mkdir(directory.c_str(), 0755);
}

void Clear_Trajectory_Text_Files(const std::string& directory)
{
	DIR* dir = opendir(directory.c_str());
	if(dir == NULL)
		return;

	struct dirent* entry = NULL;
	while((entry = readdir(dir)) != NULL)
	{
		const std::string name = entry->d_name;
		if(Has_Prefix(name, "trajectory_") && Has_Suffix(name, ".txt"))
			std::remove(Join_Path(directory, name).c_str());
	}
	closedir(dir);
}

template<typename T>
T Required_Value(const Config& config, const char* key)
{
	try
	{
		return config.lookup(key);
	}
	catch(const SettingNotFoundException&)
	{
		std::cerr << "Missing required setting: " << key << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

template<typename T>
T Optional_Value(const Config& config, const char* key, T default_value)
{
	try
	{
		return config.lookup(key);
	}
	catch(const SettingNotFoundException&)
	{
		return default_value;
	}
}

std::string Required_String(const Config& config, const char* key)
{
	try
	{
		return config.lookup(key).c_str();
	}
	catch(const SettingNotFoundException&)
	{
		std::cerr << "Missing required setting: " << key << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

std::string Optional_String(const Config& config, const char* key, const std::string& default_value)
{
	try
	{
		return config.lookup(key).c_str();
	}
	catch(const SettingNotFoundException&)
	{
		return default_value;
	}
}

bool Setting_Exists(const Config& config, const char* key)
{
	try
	{
		config.lookup(key);
		return true;
	}
	catch(const SettingNotFoundException&)
	{
		return false;
	}
}

unsigned long int Optional_Unsigned_Long(const Config& config, const char* key, unsigned long int default_value)
{
	try
	{
		config.lookup(key);
	}
	catch(const SettingNotFoundException&)
	{
		return default_value;
	}

	unsigned long long integer_value = 0;
	if(config.lookupValue(key, integer_value))
	{
		if(integer_value > std::numeric_limits<unsigned long int>::max())
		{
			std::cerr << "Setting is too large for unsigned long int: " << key << std::endl;
			std::exit(EXIT_FAILURE);
		}
		return static_cast<unsigned long int>(integer_value);
	}

	int int_value = 0;
	if(config.lookupValue(key, int_value))
	{
		if(int_value < 0)
		{
			std::cerr << "Setting must be non-negative: " << key << std::endl;
			std::exit(EXIT_FAILURE);
		}
		return static_cast<unsigned long int>(int_value);
	}

	double float_value = 0.0;
	if(config.lookupValue(key, float_value) && std::isfinite(float_value) && float_value >= 0.0 && std::floor(float_value) == float_value)
		return static_cast<unsigned long int>(float_value);

	std::cerr << "Setting must be a non-negative integer: " << key << std::endl;
	std::exit(EXIT_FAILURE);
}

void Require_Positive(double value, const char* key)
{
	if(!std::isfinite(value) || value <= 0.0)
	{
		std::cerr << key << " must be positive." << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

Run_Config Read_Run_Config(const Config& config)
{
	Run_Config run;
	run.body_configured = Setting_Exists(config, "body");
	run.body = Optional_String(config, "body", "Sun");
	if(run.body != "Sun" && run.body != "Earth")
	{
		std::cerr << "Unsupported body: " << run.body << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(run.body == "Earth")
	{
#if DAMASCUS_HAS_EARTH_MODEL
		run.body_model_file = Optional_String(config, "body_model_file", std::string(PROJECT_DIR) + "/data/earth_prem.dat");
		run.body_composition = Optional_String(config, "body_composition", "layered");
		if(run.body_composition != "layered")
		{
			std::cerr << "Unsupported body_composition for Earth: " << run.body_composition << std::endl;
			std::exit(EXIT_FAILURE);
		}
#else
		std::cerr << "Unsupported body: Earth (Earth model support was not built)." << std::endl;
		std::exit(EXIT_FAILURE);
#endif
	}
	run.output_dir = Optional_String(config, "output_dir", "./trajectory_output");
	int sample_size = Required_Value<int>(config, "sample_size");
	int interpolation_points = Optional_Value<int>(config, "interpolation_points", 1000);
	if(sample_size <= 0)
	{
		std::cerr << "sample_size must be positive." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if(interpolation_points < 0)
	{
		std::cerr << "interpolation_points must be non-negative." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	run.sample_size = static_cast<unsigned int>(sample_size);
	run.interpolation_points = static_cast<unsigned int>(interpolation_points);
	run.max_trajectories = Optional_Unsigned_Long(config, "max_trajectories", run.sample_size * 1000UL);
	run.maximum_number_of_scatterings = Optional_Unsigned_Long(config, "maximum_number_of_scatterings", DEFAULT_MAXIMUM_SCATTERINGS);
	run.maximum_free_time_steps = Optional_Unsigned_Long(config, "maximum_free_time_steps", DEFAULT_MAXIMUM_FREE_TIME_STEPS);
	if(run.body == "Sun" && Setting_Exists(config, "initial_radius_rsun"))
		run.initial_radius_body_radius = Optional_Value<double>(config, "initial_radius_rsun", 2.0);
	else
		run.initial_radius_body_radius = Optional_Value<double>(config, "initial_radius_body_radius", 2.0);
	Require_Positive(run.initial_radius_body_radius, (run.body == "Sun" && Setting_Exists(config, "initial_radius_rsun")) ? "initial_radius_rsun" : "initial_radius_body_radius");
	if(Setting_Exists(config, "asymptotic_distance_body_radius"))
	{
		run.asymptotic_distance_body_radius = Optional_Value<double>(config, "asymptotic_distance_body_radius", 0.0);
		Require_Positive(run.asymptotic_distance_body_radius, "asymptotic_distance_body_radius");
	}
	else if(run.body == "Earth")
		run.asymptotic_distance_body_radius = 1000.0;
	run.bincount_max_radius_body_radius = Optional_Value<double>(config, "bincount_max_radius_body_radius", run.initial_radius_body_radius);
	Require_Positive(run.bincount_max_radius_body_radius, "bincount_max_radius_body_radius");
	run.max_trajectory_wall_time_sec = Optional_Value<double>(config, "max_trajectory_wall_time_sec", 300.0);
	run.capture_mode = Optional_Value<bool>(config, "capture_mode", false);
	run.clear_existing_trajectories = Optional_Value<bool>(config, "clear_existing_trajectories", true);
	int write_stride = Optional_Value<int>(config, "trajectory_write_stride", 1);
	run.trajectory_write_stride = (write_stride > 0) ? static_cast<unsigned int>(write_stride) : 1;
	run.txt_precision = Optional_Value<int>(config, "txt_precision", 10);
	if(Setting_Exists(config, "random_seed"))
	{
		if(!config.lookupValue("random_seed", run.random_seed) || run.random_seed < 0)
		{
			std::cerr << "random_seed must be a non-negative integer." << std::endl;
			std::exit(EXIT_FAILURE);
		}
		run.random_seed_configured = true;
	}

	if(run.txt_precision < 6)
		run.txt_precision = 6;

	return run;
}

std::unique_ptr<obscura::DM_Particle> Build_DM_Particle(const Config& config)
{
	double DM_mass = Required_Value<double>(config, "DM_mass") * GeV;
	double DM_spin = Required_Value<double>(config, "DM_spin");
	double DM_fraction = Required_Value<double>(config, "DM_fraction");
	bool DM_light = Required_Value<bool>(config, "DM_light");
	std::string DM_interaction = Required_String(config, "DM_interaction");

	std::unique_ptr<obscura::DM_Particle> DM;
	if(DM_interaction == "SI")
	{
		DM.reset(new obscura::DM_Particle_SI());
		std::string form_factor = Required_String(config, "DM_form_factor");
		double mediator_mass = -1.0;
		if(form_factor == "General")
			mediator_mass = Required_Value<double>(config, "DM_mediator_mass") * MeV;
		dynamic_cast<obscura::DM_Particle_SI*>(DM.get())->Set_FormFactor_DM(form_factor, mediator_mass);
	}
	else if(DM_interaction == "SD")
	{
		DM.reset(new obscura::DM_Particle_SD());
	}
	else if(DM_interaction == "DP" || DM_interaction == "Dark photon")
	{
		DM.reset(new DM_Particle_Dark_Photon());
		std::string form_factor = Required_String(config, "DM_form_factor");
		double mediator_mass = -1.0;
		if(form_factor == "General")
			mediator_mass = Required_Value<double>(config, "DM_mediator_mass") * MeV;
		dynamic_cast<DM_Particle_Dark_Photon*>(DM.get())->Set_FormFactor_DM(form_factor, mediator_mass);
	}
	else
	{
		std::cerr << "Unsupported DM_interaction: " << DM_interaction << std::endl;
		std::exit(EXIT_FAILURE);
	}

	DM->Set_Mass(DM_mass);
	DM->Set_Spin(DM_spin);
	DM->Set_Fractional_Density(DM_fraction);
	DM->Set_Low_Mass_Mode(DM_light);

	if(DM_interaction == "SI" || DM_interaction == "SD")
	{
		bool isospin_conserved = Required_Value<bool>(config, "DM_isospin_conserved");
		double fp_rel = 1.0;
		double fn_rel = 1.0;
		if(!isospin_conserved)
		{
			fp_rel = config.lookup("DM_relative_couplings")[0];
			fn_rel = config.lookup("DM_relative_couplings")[1];
		}
		dynamic_cast<obscura::DM_Particle_Standard*>(DM.get())->Fix_Coupling_Ratio(fp_rel, fn_rel);
		double sigma_nucleon = Required_Value<double>(config, "DM_cross_section_nucleon") * cm * cm;
		double sigma_electron = Required_Value<double>(config, "DM_cross_section_electron") * cm * cm;
		DM->Set_Interaction_Parameter(sigma_nucleon, "Nuclei");
		DM->Set_Sigma_Electron(sigma_electron);
	}
	else
	{
		double sigma_electron = Required_Value<double>(config, "DM_cross_section_electron") * cm * cm;
		DM->Set_Interaction_Parameter(sigma_electron, "Electrons");
	}

	return DM;
}

std::unique_ptr<obscura::DM_Distribution> Build_DM_Distribution(const Config& config)
{
	std::string label = Required_String(config, "DM_distribution");
	double local_density = Required_Value<double>(config, "DM_local_density") * GeV / cm / cm / cm;

	if(label == "SHM" || label == "SHM++")
	{
		double v0 = Required_Value<double>(config, "SHM_v0") * km / sec;
		double v_escape = Required_Value<double>(config, "SHM_vEscape") * km / sec;
		libphysica::Vector observer(3, 0.0);
		for(int index = 0; index < 3; index++)
			observer[index] = config.lookup("SHM_vObserver")[index];
		observer = observer * km / sec;

		if(label == "SHM")
			return std::unique_ptr<obscura::DM_Distribution>(new obscura::Standard_Halo_Model(local_density, v0, observer, v_escape));

		double eta = Required_Value<double>(config, "SHMpp_eta");
		double beta = Required_Value<double>(config, "SHMpp_beta");
		return std::unique_ptr<obscura::DM_Distribution>(new obscura::SHM_Plus_Plus(local_density, v0, observer, v_escape, eta, beta));
	}

	if(label == "File")
	{
		std::string path = Required_String(config, "file_path");
		return std::unique_ptr<obscura::DM_Distribution>(new obscura::Imported_DM_Distribution(local_density, path));
	}

	std::cerr << "Unsupported DM_distribution: " << label << std::endl;
	std::exit(EXIT_FAILURE);
}

double Event_Energy_Ev(const Event& event, Celestial_Model& body_model, obscura::DM_Particle& DM)
{
	double radius = event.Radius();
	double speed = event.Speed();
	double v_escape = body_model.Local_Escape_Speed(radius);
	double energy = 0.5 * DM.mass * (speed * speed - v_escape * v_escape);
	return In_Units(energy, eV);
}

void Write_Trajectory_Row(std::ofstream& file, const Event& event, Celestial_Model& body_model, obscura::DM_Particle& DM, int precision, Trajectory_Stats& stats)
{
	file << std::scientific << std::setprecision(precision)
	     << In_Units(event.time, sec) << "\t"
	     << In_Units(event.position[0], km) << "\t"
	     << In_Units(event.position[1], km) << "\t"
	     << In_Units(event.position[2], km) << "\t"
	     << In_Units(event.velocity[0], km / sec) << "\t"
	     << In_Units(event.velocity[1], km / sec) << "\t"
	     << In_Units(event.velocity[2], km / sec) << "\t"
	     << Event_Energy_Ev(event, body_model, DM) << "\n";
	stats.rows_written++;
}

double RK45_Absolute_Max_Time_Step()
{
	return 1.0e6 * sec;
}

double RK45_Sanitized_Time_Step(double step)
{
	if(!std::isfinite(step) || step <= 0.0)
		return 0.1 * sec;
	return std::min(step, RK45_Absolute_Max_Time_Step());
}

double Free_Propagation_Time_Step_Cap(double radius, double speed, double maximum_distance, double central_mass)
{
	double cap = RK45_Absolute_Max_Time_Step();
	const double safe_speed = std::max(std::fabs(speed), 1.0e-12 * km / sec);
	const double crossing_scale = std::max(0.25 * maximum_distance, 10.0 * km);
	cap = std::min(cap, crossing_scale / safe_speed);
	const double safe_radius = std::max(radius, 1.0 * km);
	const double dynamical_time = sqrt(safe_radius * safe_radius * safe_radius / (G_Newton * central_mass));
	if(std::isfinite(dynamical_time) && dynamical_time > 0.0)
		cap = std::min(cap, 0.1 * dynamical_time);
	return std::max(cap, 1.0e-8 * sec);
}

std::unique_ptr<Celestial_Model> Build_Body_Model(const Run_Config& run_config)
{
	if(run_config.body == "Sun")
		return std::unique_ptr<Celestial_Model>(new Solar_Model());

#if DAMASCUS_HAS_EARTH_MODEL
	if(run_config.body == "Earth")
		return std::unique_ptr<Celestial_Model>(new Earth_Model(run_config.body_model_file));
#endif

	std::cerr << "Unsupported body: " << run_config.body << std::endl;
	std::exit(EXIT_FAILURE);
}

std::string Body_Output_Directory_Name(const std::string& body)
{
	if(body == "Earth")
		return "earth";
	return "sun";
}

void Print_Body_Summary(const Run_Config& run_config, Celestial_Model& body_model)
{
	std::cout << "Body: " << run_config.body << std::endl
	          << "Body model: " << body_model.Name() << std::endl
	          << "Body radius [km]: " << In_Units(body_model.Radius(), km) << std::endl
	          << "Body mass [kg]: " << In_Units(body_model.Total_Mass(), kg) << std::endl;
#if DAMASCUS_HAS_EARTH_MODEL
	if(run_config.body == "Earth")
	{
		Earth_Model* earth_model = dynamic_cast<Earth_Model*>(&body_model);
		if(earth_model != NULL)
		{
			std::cout << "Body model file: " << earth_model->Model_File() << std::endl;
			std::cout << "Body composition: " << run_config.body_composition << std::endl;
		}
	}
#endif
}

bool Outward_Escaping_At_Boundary(const Event& event, Celestial_Model& body_model, double boundary_radius)
{
	const double radius = event.Radius();
	if(radius < boundary_radius)
		return false;
	const double radial_velocity = (radius > 0.0) ? event.position.Dot(event.velocity) / radius : 0.0;
	return radial_velocity > 0.0 && event.Speed() > body_model.Local_Escape_Speed(radius);
}

Free_Propagation_Result Propagate_Freely_To_Txt(Event& current_event, obscura::DM_Particle& DM, Celestial_Model& body_model, Trajectory_Simulator& simulator, const Run_Config& run_config, std::ofstream& trajectory_file, std::chrono::steady_clock::time_point trajectory_wall_start, Trajectory_Stats& stats)
{
	if(Outward_Escaping_At_Boundary(current_event, body_model, run_config.initial_radius))
		return Free_Propagation_Result::Escape;

	Free_Particle_Propagator particle_propagator(current_event);
	double minus_log_xi = -log(libphysica::Sample_Uniform(simulator.PRNG));

	for(unsigned long int step = 0; step < run_config.maximum_free_time_steps; step++)
	{
		double r_before = particle_propagator.Current_Radius();
		double v_before = particle_propagator.Current_Speed();
		if(r_before >= body_model.Radius())
		{
			const double step_cap = Free_Propagation_Time_Step_Cap(r_before, v_before, run_config.initial_radius, body_model.Total_Mass());
			particle_propagator.time_step = std::min(RK45_Sanitized_Time_Step(particle_propagator.time_step), step_cap);
		}
		else
			particle_propagator.time_step = RK45_Sanitized_Time_Step(particle_propagator.time_step);

		double t_before = particle_propagator.Current_Time();
		particle_propagator.Runge_Kutta_45_Step(body_model);
		double actual_dt = particle_propagator.Current_Time() - t_before;
		double r_after = particle_propagator.Current_Radius();
		double v_after = particle_propagator.Current_Speed();
		stats.rk45_steps++;

		if(!std::isfinite(r_after) || !std::isfinite(v_after) || !std::isfinite(actual_dt))
		{
			current_event = particle_propagator.Event_In_3D();
			stats.aborted = true;
			return Free_Propagation_Result::Abort;
		}

		if(v_after > 0.75)
		{
			current_event = particle_propagator.Event_In_3D();
			stats.aborted = true;
			return Free_Propagation_Result::Abort;
		}

		if(run_config.max_trajectory_wall_time_sec > 0.0 && (stats.rk45_steps & 0xFFu) == 0u)
		{
			double wall_seconds = 1.0e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - trajectory_wall_start).count();
			if(wall_seconds > run_config.max_trajectory_wall_time_sec)
			{
				current_event = particle_propagator.Event_In_3D();
				stats.aborted = true;
				return Free_Propagation_Result::Abort;
			}
		}

		current_event = particle_propagator.Event_In_3D();
		if(stats.rk45_steps % run_config.trajectory_write_stride == 0)
			Write_Trajectory_Row(trajectory_file, current_event, body_model, DM, run_config.txt_precision, stats);

		if(Event_Energy_Ev(current_event, body_model, DM) < 0.0)
		{
			stats.captured = true;
			if(run_config.capture_mode)
				return Free_Propagation_Result::CaptureStop;
		}

		bool scattering = false;
		bool reflection = false;
		if(r_after < body_model.Radius())
		{
			if(v_after < 0.0)
				return Free_Propagation_Result::Abort;
			double total_rate = body_model.Total_DM_Scattering_Rate(DM, r_after, v_after);
			double time_step_max = (total_rate > 0.0) ? (0.1 / total_rate) : (1.0e30);
			if(particle_propagator.time_step > time_step_max)
				particle_propagator.time_step = time_step_max;
			minus_log_xi -= actual_dt * total_rate;
			if(minus_log_xi < 0.0)
				scattering = true;
		}
		else if(r_after >= run_config.initial_radius && r_after >= r_before && v_after > body_model.Local_Escape_Speed(r_after))
			reflection = true;

		if(scattering)
			return Free_Propagation_Result::Scatter;
		if(reflection)
			return Free_Propagation_Result::Escape;
	}

	current_event = particle_propagator.Event_In_3D();
	return Free_Propagation_Result::StepLimit;
}

Trajectory_Stats Simulate_Trajectory_To_Txt(Event initial_condition, obscura::DM_Particle& DM, Celestial_Model& body_model, Trajectory_Simulator& simulator, const Run_Config& run_config, std::ofstream& trajectory_file)
{
	Trajectory_Stats stats;
	Event current_event = initial_condition;
	auto trajectory_wall_start = std::chrono::steady_clock::now();

	trajectory_file << "# columns: time_s x_km y_km z_km vx_km_s vy_km_s vz_km_s E_eV\n";
	Write_Trajectory_Row(trajectory_file, current_event, body_model, DM, run_config.txt_precision, stats);
	if(Event_Energy_Ev(current_event, body_model, DM) < 0.0)
		stats.captured = true;

	while(stats.scatterings < run_config.maximum_number_of_scatterings)
	{
		Free_Propagation_Result result = Propagate_Freely_To_Txt(current_event, DM, body_model, simulator, run_config, trajectory_file, trajectory_wall_start, stats);
		if(result == Free_Propagation_Result::Scatter)
		{
			simulator.Scatter(current_event, DM);
			stats.scatterings++;
			Write_Trajectory_Row(trajectory_file, current_event, body_model, DM, run_config.txt_precision, stats);
			if(Event_Energy_Ev(current_event, body_model, DM) < 0.0)
			{
				stats.captured = true;
				if(run_config.capture_mode)
					break;
			}
			continue;
		}

		if(result == Free_Propagation_Result::Escape)
		{
			if(stats.scatterings == 0)
				stats.free_particle = true;
			else
				stats.reflected_particle = true;
		}
		break;
	}

	return stats;
}

std::string Parameter_Output_Directory(const Run_Config& run_config, obscura::DM_Particle& DM)
{
	double mass_log10 = log10(In_Units(DM.mass, GeV));
	double sigma_log10 = log10(In_Units(DM.Sigma_Proton(), cm * cm));
	std::string parameter_dir = "results_" + std::to_string(mass_log10) + "_" + std::to_string(sigma_log10);
	if(run_config.body_configured)
		return Join_Path(Join_Path(run_config.output_dir, Body_Output_Directory_Name(run_config.body)), parameter_dir);
	return Join_Path(run_config.output_dir, parameter_dir);
}

std::string Trajectory_File_Path(const std::string& output_dir, unsigned long int local_id, int mpi_rank)
{
	return Join_Path(output_dir, "trajectory_" + std::to_string(local_id) + "_task" + std::to_string(mpi_rank) + ".txt");
}
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
	int mpi_rank = 0;
	int mpi_processes = 1;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_processes);

	if(argc < 2)
	{
		if(mpi_rank == 0)
			std::cerr << "Usage: " << argv[0] << " config.cfg" << std::endl;
		MPI_Finalize();
		return EXIT_FAILURE;
	}

	Config config;
	try
	{
		config.readFile(argv[1]);
	}
	catch(const FileIOException&)
	{
		if(mpi_rank == 0)
			std::cerr << "Could not read config file: " << argv[1] << std::endl;
		MPI_Finalize();
		return EXIT_FAILURE;
	}
	catch(const ParseException& error)
	{
		if(mpi_rank == 0)
			std::cerr << "Config parse error at " << error.getFile() << ":" << error.getLine() << " - " << error.getError() << std::endl;
		MPI_Finalize();
		return EXIT_FAILURE;
	}

	Run_Config run_config = Read_Run_Config(config);
	std::unique_ptr<obscura::DM_Particle> DM = Build_DM_Particle(config);
	std::unique_ptr<obscura::DM_Distribution> DM_distribution = Build_DM_Distribution(config);

	auto time_start = std::chrono::system_clock::now();
	std::unique_ptr<Celestial_Model> body_model = Build_Body_Model(run_config);
	run_config.initial_radius = run_config.initial_radius_body_radius * body_model->Radius();
	run_config.bincount_max_radius = run_config.bincount_max_radius_body_radius * body_model->Radius();
	if(run_config.asymptotic_distance_body_radius > 0.0)
		run_config.asymptotic_distance = run_config.asymptotic_distance_body_radius * body_model->Radius();
	if(mpi_rank == 0)
	{
		std::cout << PROJECT_NAME << " " << PROJECT_VERSION << std::endl
		          << "Trajectory TXT runner" << std::endl
		          << "MPI processes: " << mpi_processes << std::endl
		          << "Output mode: trajectory txt files only" << std::endl;
		Print_Body_Summary(run_config, *body_model);
		if(run_config.random_seed_configured)
			std::cout << "PRNG seed: random_seed + mpi_rank, base random_seed = " << run_config.random_seed << std::endl;
		else
			std::cout << "PRNG seed: random_device" << std::endl;
	}
	body_model->Interpolate_Total_DM_Scattering_Rate(*DM, run_config.interpolation_points, run_config.interpolation_points);

	std::string output_dir = Parameter_Output_Directory(run_config, *DM);
	if(mpi_rank == 0)
	{
		Ensure_Directory_Exists(run_config.output_dir);
		Ensure_Directory_Exists(output_dir);
		if(run_config.clear_existing_trajectories)
			Clear_Trajectory_Text_Files(output_dir);
		std::cout << "Trajectory directory: " << output_dir << std::endl;
	}
	MPI_Barrier(MPI_COMM_WORLD);
	Ensure_Directory_Exists(output_dir);

	unsigned long int target_captured_per_rank = (run_config.sample_size + mpi_processes - 1) / mpi_processes;
	unsigned long int max_trajectories_per_rank = std::numeric_limits<unsigned long int>::max();
	if(run_config.max_trajectories != 0)
		max_trajectories_per_rank = (run_config.max_trajectories + mpi_processes - 1) / mpi_processes;

	Trajectory_Simulator simulator(*body_model, run_config.maximum_free_time_steps, run_config.maximum_number_of_scatterings, run_config.bincount_max_radius);
	simulator.max_trajectory_wall_time_sec = run_config.max_trajectory_wall_time_sec;
	if(run_config.random_seed_configured)
	{
		if(run_config.random_seed > std::numeric_limits<int>::max() - mpi_rank)
		{
			if(mpi_rank == 0)
				std::cerr << "random_seed + mpi_rank exceeds the supported integer range." << std::endl;
			MPI_Finalize();
			return EXIT_FAILURE;
		}
		simulator.Fix_PRNG_Seed(run_config.random_seed + mpi_rank);
	}

	unsigned long int local_total = 0;
	unsigned long int local_captured = 0;
	unsigned long int local_free = 0;
	unsigned long int local_reflected = 0;
	unsigned long int local_aborted = 0;
	unsigned long int local_rows = 0;
	unsigned long int local_rk45_steps = 0;
	bool early_stopped = false;
	int last_milestone = -1;

	while(local_captured < target_captured_per_rank && local_total < max_trajectories_per_rank)
	{
		unsigned long int local_id = local_total + 1;
		std::string trajectory_path = Trajectory_File_Path(output_dir, local_id, mpi_rank);
		std::ofstream trajectory_file(trajectory_path.c_str(), std::ios::out | std::ios::trunc);
		if(!trajectory_file)
		{
			std::cerr << "Rank " << mpi_rank << " could not open " << trajectory_path << std::endl;
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}

		Event initial_condition = Initial_Conditions(*DM_distribution, *body_model, simulator.PRNG, run_config.asymptotic_distance);
		Hyperbolic_Kepler_Shift(initial_condition, *body_model, run_config.initial_radius);
		Trajectory_Stats stats = Simulate_Trajectory_To_Txt(initial_condition, *DM, *body_model, simulator, run_config, trajectory_file);
		trajectory_file.close();

		local_total++;
		local_rows += stats.rows_written;
		local_rk45_steps += stats.rk45_steps;
		if(stats.captured)
			local_captured++;
		if(stats.free_particle)
			local_free++;
		if(stats.reflected_particle)
			local_reflected++;
		if(stats.aborted)
			local_aborted++;

		if(mpi_rank == 0)
		{
			double progress = std::min(1.0, static_cast<double>(local_captured) / static_cast<double>(target_captured_per_rank));
			int milestone = static_cast<int>(progress * 5.0);
			if(milestone > last_milestone)
			{
				last_milestone = milestone;
				double elapsed = 1.0e-6 * std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start).count();
				libphysica::Print_Progress_Bar(progress, 0, 44, elapsed);
			}
		}
	}

	if(local_total >= max_trajectories_per_rank && local_captured < target_captured_per_rank)
		early_stopped = true;

	unsigned long int global_total = local_total;
	unsigned long int global_captured = local_captured;
	unsigned long int global_free = local_free;
	unsigned long int global_reflected = local_reflected;
	unsigned long int global_aborted = local_aborted;
	unsigned long int global_rows = local_rows;
	unsigned long int global_rk45_steps = local_rk45_steps;
	int local_early_stopped = early_stopped ? 1 : 0;
	int global_early_stopped = 0;
	MPI_Allreduce(MPI_IN_PLACE, &global_total, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(MPI_IN_PLACE, &global_captured, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(MPI_IN_PLACE, &global_free, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(MPI_IN_PLACE, &global_reflected, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(MPI_IN_PLACE, &global_aborted, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(MPI_IN_PLACE, &global_rows, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(MPI_IN_PLACE, &global_rk45_steps, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(&local_early_stopped, &global_early_stopped, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

	auto time_end = std::chrono::system_clock::now();
	double duration = 1.0e-6 * std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
	if(mpi_rank == 0)
	{
		libphysica::Print_Progress_Bar(1.0, 0, 44, duration);
		std::cout << std::endl
		          << SEPARATOR
		          << "Trajectory TXT summary" << std::endl
		          << "Trajectory files: " << global_total << std::endl
		          << "Captured: " << global_captured << std::endl
		          << "Free: " << global_free << std::endl
		          << "Reflected: " << global_reflected << std::endl
		          << "Aborted: " << global_aborted << std::endl
		          << "Text rows written: " << global_rows << std::endl
		          << "RK45 steps: " << global_rk45_steps << std::endl;
		if(global_early_stopped)
			std::cout << "EARLY STOP: max_trajectories reached" << std::endl;
		std::cout << "Runtime: " << libphysica::Time_Display(duration) << std::endl
		          << SEPARATOR << std::endl;
	}

	MPI_Finalize();
	return EXIT_SUCCESS;
}