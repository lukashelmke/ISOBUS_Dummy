#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/twai_plugin.hpp"

#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/isobus_task_controller_server.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"


#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>


extern "C" const uint8_t object_pool_start[] asm("_binary_StandardPool_iop_start");
extern "C" const uint8_t object_pool_end[]   asm("_binary_StandardPool_iop_end");


class SimpleLogger : public isobus::CANStackLogger
{
public:
	void sink_CAN_stack_log(LoggingLevel level, const std::string &text) override
	{
		printf("[AgIsoStack %d] %s\n", static_cast<int>(level), text.c_str());
	}
};

static SimpleLogger g_logger;

struct FunctionNameEntry
{
	std::uint8_t ig;
	std::uint8_t vs;
	std::uint8_t fc;
	const char *name;
};

static const FunctionNameEntry kFunctionNames[] = {
	// Common functions (apply to all IG/VS) from user-provided list
	{ 255, 255, 0, "Engine" },
	{ 255, 255, 1, "Auxiliary Power Unit (APU)" },
	{ 255, 255, 2, "Electric Propulsion Control" },
	{ 255, 255, 3, "Transmission" },
	{ 255, 255, 4, "Battery Pack Monitor" },
	{ 255, 255, 5, "Shift Control/Console" },
	{ 255, 255, 6, "Power TakeOff - (Main or Rear)" },
	{ 255, 255, 7, "Axle - Steering" },
	{ 255, 255, 8, "Axle - Drive" },
	{ 255, 255, 9, "Brakes - System Controller" },
	{ 255, 255, 10, "Brakes - Steer Axle" },
	{ 255, 255, 11, "Brakes - Drive Axle" },
	{ 255, 255, 12, "Retarder - Engine" },
	{ 255, 255, 13, "Retarder - Driveline" },
	{ 255, 255, 14, "Cruise Control" },
	{ 255, 255, 15, "Fuel System" },
	{ 255, 255, 16, "Steering Controller" },
	{ 255, 255, 17, "Suspension - Steer Axle" },
	{ 255, 255, 18, "Suspension - Drive Axle" },
	{ 255, 255, 19, "Instrument Cluster" },
	{ 255, 255, 20, "Trip Recorder" },
	{ 255, 255, 21, "Cab Climate Control" },
	{ 255, 255, 22, "Aerodynamic Control" },
	{ 255, 255, 23, "Vehicle Navigation" },
	{ 255, 255, 24, "Vehicle Security" },
	{ 255, 255, 25, "Network Interconnect ECU" },
	{ 255, 255, 26, "Body Controller" },
	{ 255, 255, 27, "Power TakeOff (Secondary or Front)" },
	{ 255, 255, 28, "Off Vehicle Gateway" },
	{ 255, 255, 29, "Virtual Terminal (in cab)" },
	{ 255, 255, 30, "Management Computer" },
	{ 255, 255, 31, "Propulsion Battery Charger" },
	{ 255, 255, 32, "Headway Controller" },
	{ 255, 255, 33, "System Monitor" },
	{ 255, 255, 34, "Hydraulic Pump Controller" },
	{ 255, 255, 35, "Suspension - System Controller" },
	{ 255, 255, 36, "Pneumatic - System Controller" },
	{ 255, 255, 37, "Cab Controller" },
	{ 255, 255, 38, "Tire Pressure Control" },
	{ 255, 255, 39, "Ignition Control Module" },
	{ 255, 255, 40, "Seat Control" },
	{ 255, 255, 41, "Lighting - Operator Controls" },
	{ 255, 255, 42, "Water Pump Control" },
	{ 255, 255, 43, "Transmission Display" },
	{ 255, 255, 44, "Exhaust Emission Control" },
	{ 255, 255, 45, "Vehicle Dynamic Stability Control" },
	{ 255, 255, 46, "Oil Sensor Unit" },
	{ 255, 255, 47, "Information System Controller" },
	{ 255, 255, 48, "Ramp Control" },
	{ 255, 255, 49, "Clutch/Converter Control" },
	{ 255, 255, 50, "Auxiliary Heater" },
	{ 255, 255, 51, "Forward-Looking Collision Warning System" },
	{ 255, 255, 52, "Chassis Controller" },
	{ 255, 255, 53, "Alternator/Charging System" },
	{ 255, 255, 54, "Communications Unit, Cellular" },
	{ 255, 255, 55, "Communications Unit, Satellite" },
	{ 255, 255, 56, "Communications Unit, Radio" },
	{ 255, 255, 57, "Steering Column Unit" },
	{ 255, 255, 58, "Fan Drive Control" },
	{ 255, 255, 59, "Starter" },
	{ 255, 255, 60, "Cab Display" },
	{ 255, 255, 61, "File Server / Printer" },
	{ 255, 255, 62, "On-Board Diagnostic Unit" },
	{ 255, 255, 63, "Engine Valve Controller" },
	{ 255, 255, 64, "Endurance Braking" },
	{ 255, 255, 65, "Gas Flow Measurement" },
	{ 255, 255, 66, "I/O Controller" },
	{ 255, 255, 67, "Electrical System Controller" },
	{ 255, 255, 68, "Aftertreatment system gas measurement" },
	{ 255, 255, 69, "Engine Emission Aftertreatment System" },
	{ 255, 255, 70, "Auxiliary Regeneration Device" },
	{ 255, 255, 71, "Transfer Case Control" },
	{ 255, 255, 72, "Coolant Valve Controller" },
	{ 255, 255, 73, "Rollover Detection Control" },
	{ 255, 255, 74, "Lubrication System" },
	{ 255, 255, 75, "Supplemental Fan" },
	{ 255, 255, 76, "Temperature Sensor" },
	{ 255, 255, 77, "Fuel Properties Sensor" },
	{ 255, 255, 78, "Fire Suppression System" },
	{ 255, 255, 79, "Power Systems Manager" },
	{ 255, 255, 80, "Electric Powertrain" },
	{ 255, 255, 81, "Hydraulic Powertrain" },
	{ 255, 255, 82, "File Server" },
	{ 255, 255, 83, "Printer" },
	{ 255, 255, 84, "Start Aid Device" },
	{ 255, 255, 85, "Engine Injection Control Module" },
	{ 255, 255, 86, "EV Communication Controller" },
	{ 255, 255, 87, "Driver Impairment Device" },
	{ 255, 255, 88, "Electric Power Converter" },
	{ 255, 255, 89, "Supply Equipment Communication Controller (SECC)" },
	{ 255, 255, 90, "Vehicle Adapter Communication Controller (VACC)" },
	{ 255, 255, 91, "Accessory Electric Motor Controller" },
	{ 255, 255, 92, "Current Sensor" },
	{ 255, 255, 93, "Fuel Cell System" },
	{ 255, 255, 94, "Auxiliary Display" },

	// IG 0
	{ 0, 0, 128, "Reserved" },
	{ 0, 0, 129, "Off-board diagnostic-service tool" },
	{ 0, 0, 130, "On-board data logger" },
	{ 0, 0, 131, "PC Keyboard" },
	{ 0, 0, 132, "Safety Restraint System" },
	{ 0, 0, 133, "Turbocharger" },
	{ 0, 0, 134, "Ground based speed sensor" },
	{ 0, 0, 135, "Keypad" },
	{ 0, 0, 136, "Humidity sensor" },
	{ 0, 0, 137, "Thermal Management System Controller" },
	{ 0, 0, 138, "Brake Stroke Alert" },
	{ 0, 0, 139, "On-board axle group scale" },
	{ 0, 0, 140, "On-board axle group display" },
	{ 0, 0, 141, "Battery Charger" },
	{ 0, 0, 142, "Turbocharger Compressor Bypass" },
	{ 0, 0, 143, "Turbocharger Wastegate" },
	{ 0, 0, 144, "Throttle" },
	{ 0, 0, 145, "Inertial Sensor" },
	{ 0, 0, 146, "Fuel Actuator" },
	{ 0, 0, 147, "Engine Exhaust Gas Recirculation" },
	{ 0, 0, 148, "Engine Exhaust Backpressure" },
	{ 0, 0, 149, "On-board bin weighing scale" },
	{ 0, 0, 150, "On-board bin weighing scale display" },
	{ 0, 0, 151, "Engine Cylinder Pressure Monitoring System" },
	{ 0, 0, 152, "Object Detection" },
	{ 0, 0, 153, "Object Detection Display" },
	{ 0, 0, 154, "Object Detection Sensor" },
	{ 0, 0, 155, "Personnel Detection Device" },
	{ 0, 0, 255, "Not Available" },
	{ 0, 127, 255, "Not Available" },

	// IG 1
	{ 1, 0, 128, "Tachograph" },
	{ 1, 0, 129, "Door Controller" },
	{ 1, 0, 130, "Articulation Turntable Control" },
	{ 1, 0, 131, "Body-to-Vehicle Interface Control" },
	{ 1, 0, 132, "Slope Sensor" },
	{ 1, 0, 134, "Retarder Display" },
	{ 1, 0, 135, "Differential Lock Controller" },
	{ 1, 0, 136, "Low-Voltage Disconnect" },
	{ 1, 0, 137, "Roadway Information" },
	{ 1, 0, 138, "Automated Driving" },
	{ 1, 0, 255, "Not Available" },
	{ 1, 1, 128, "Forward Road Image Processing" },
	{ 1, 1, 129, "Fifth Wheel Smart System" },
	{ 1, 1, 130, "Catalyst Fluid Sensor" },
	{ 1, 1, 131, "Adaptive Front Lighting System" },
	{ 1, 1, 132, "Idle Control System" },
	{ 1, 1, 133, "User Interface System" },
	{ 1, 1, 255, "Not Available" },
	{ 1, 2, 255, "Not Available" },
	{ 1, 127, 255, "Not Available" },

	// IG 2 (Agriculture/Forestry)
	{ 2, 0, 128, "Non Virtual Terminal Display" },
	{ 2, 0, 129, "Operator Controls - Machine Specific" },
	{ 2, 0, 130, "Task Controller (Mapping Computer)" },
	{ 2, 0, 131, "Position Control" },
	{ 2, 0, 132, "Machine Control" },
	{ 2, 0, 133, "Foreign Object Detection" },
	{ 2, 0, 134, "Tractor ECU" },
	{ 2, 0, 135, "Sequence Control Master" },
	{ 2, 0, 136, "Product Dosing" },
	{ 2, 0, 137, "Product Treatment" },
	{ 2, 0, 138, "Reserved" },
	{ 2, 0, 139, "Data Logger" },
	{ 2, 0, 140, "Decision Support" },
	{ 2, 0, 141, "Lighting Controller" },
	{ 2, 0, 142, "TIM Server" },
	{ 2, 0, 255, "Not Available" },
	{ 2, 1, 129, "Auxiliary Valve Control" },
	{ 2, 1, 130, "Rear Hitch Control" },
	{ 2, 1, 131, "Front Hitch Control" },
	{ 2, 1, 132, "Tractor Machine Control" },
	{ 2, 1, 134, "Center Hitch Control" },
	{ 2, 1, 255, "Not Available" },
	{ 2, 2, 132, "Tillage Machine Control" },
	{ 2, 2, 135, "Tillage Depth Control" },
	{ 2, 2, 136, "Frame Control" },
	{ 2, 2, 255, "Not Available" },
	{ 2, 3, 132, "Secondary Tillage Machine Control" },
	{ 2, 3, 135, "Secondary Tillage Depth Control" },
	{ 2, 3, 136, "Frame Control" },
	{ 2, 3, 255, "Not Available" },
	{ 2, 4, 128, "Seed Rate Control" },
	{ 2, 4, 129, "Section On/Off Control" },
	{ 2, 4, 131, "Position Control" },
	{ 2, 4, 132, "Planters/Seeders Machine Control" },
	{ 2, 4, 133, "Product Flow" },
	{ 2, 4, 134, "Product Level" },
	{ 2, 4, 135, "Depth Control" },
	{ 2, 4, 136, "Frame Control" },
	{ 2, 4, 137, "Down Pressure" },
	{ 2, 4, 255, "Not Available" },
	{ 2, 5, 128, "Fertilize Rate Control" },
	{ 2, 5, 129, "Section On/Off Control" },
	{ 2, 5, 130, "Product Pressure" },
	{ 2, 5, 131, "Position Control" },
	{ 2, 5, 132, "Fertilizers Machine Control" },
	{ 2, 5, 133, "Product Flow" },
	{ 2, 5, 134, "Product Level" },
	{ 2, 5, 135, "Height/Depth Control" },
	{ 2, 5, 136, "Frame Control" },
	{ 2, 5, 255, "Not Available" },
	{ 2, 6, 128, "Spray Rate Control" },
	{ 2, 6, 129, "Section On/Off Control" },
	{ 2, 6, 130, "Product Pressure" },
	{ 2, 6, 131, "Position Control" },
	{ 2, 6, 132, "Sprayers Machine Control" },
	{ 2, 6, 133, "Product Flow" },
	{ 2, 6, 134, "Product Level" },
	{ 2, 6, 135, "Boom Height Control" },
	{ 2, 6, 136, "Frame Control" },
	{ 2, 6, 255, "Not Available" },
	{ 2, 7, 128, "Tailing Monitor" },
	{ 2, 7, 129, "Header Control" },
	{ 2, 7, 130, "Product Loss Monitor" },
	{ 2, 7, 131, "Product Moisture" },
	{ 2, 7, 132, "Harvester Machine Control" },
	{ 2, 7, 133, "Product Flow" },
	{ 2, 7, 134, "Product Level" },
	{ 2, 7, 135, "Header Height Control" },
	{ 2, 7, 255, "Not Available" },
	{ 2, 8, 132, "Root Harvesters Machine Control" },
	{ 2, 8, 133, "Product Flow" },
	{ 2, 8, 134, "Product Level" },
	{ 2, 8, 135, "Depth Control" },
	{ 2, 8, 255, "Not Available" },
	{ 2, 9, 128, "Twine Wrapper Control" },
	{ 2, 9, 129, "Product Packaging Control" },
	{ 2, 9, 131, "Product Moisture" },
	{ 2, 9, 132, "Forage Machine Control" },
	{ 2, 9, 133, "Product Flow" },
	{ 2, 9, 135, "Working Height Control" },
	{ 2, 9, 255, "Not Available" },
	{ 2, 10, 255, "Not Available" },
	{ 2, 11, 132, "Transport Machine Control" },
	{ 2, 11, 136, "Unload Control" },
	{ 2, 11, 255, "Not Available" },
	{ 2, 12, 255, "Not Available" },
	{ 2, 13, 132, "Powered Devices Machine Control" },
	{ 2, 13, 255, "Not Available" },
	{ 2, 14, 132, "Special Crop Machine Control" },
	{ 2, 14, 255, "Not Available" },
	{ 2, 15, 128, "Material Rate Control" },
	{ 2, 15, 132, "Earthworks Machine Control" },
	{ 2, 15, 133, "Material Flow" },
	{ 2, 15, 134, "Material Level" },
	{ 2, 15, 135, "Depth Control" },
	{ 2, 15, 255, "Not Available" },
	{ 2, 16, 132, "Skidder Machine Control" },
	{ 2, 16, 255, "Not Available" },
	{ 2, 17, 128, "Guidance Feeler" },
	{ 2, 17, 129, "Camera System" },
	{ 2, 17, 130, "Crop Scouting" },
	{ 2, 17, 131, "Material Properties Sensing" },
	{ 2, 17, 132, "Inertial Measurement Unit (IMU)" },
	{ 2, 17, 133, "Product Flow" },
	{ 2, 17, 134, "Product Level" },
	{ 2, 17, 135, "Product Mass" },
	{ 2, 17, 136, "Vibration/Knock" },
	{ 2, 17, 137, "Weather Instruments" },
	{ 2, 17, 138, "Soil Scouting" },
	{ 2, 19, 132, "Timber Harvestors Machine Control" },
	{ 2, 20, 132, "Forwarders Machine Control" },
	{ 2, 21, 132, "Timber Loaders Machine Control" },
	{ 2, 22, 132, "Timber Processing Machine Control" },
	{ 2, 23, 132, "Mulcher Machine Control" },
	{ 2, 24, 132, "Utility Machine Control" },
	{ 2, 25, 128, "Slurry/Manure Rate Control" },
	{ 2, 25, 129, "Section On/Off Control" },
	{ 2, 25, 130, "Product Pressure" },
	{ 2, 25, 132, "Slurry/Manure Machine Control" },
	{ 2, 25, 133, "Product Flow" },
	{ 2, 25, 134, "Product Level" },
	{ 2, 25, 135, "Boom Height Control" },
	{ 2, 26, 128, "Feeder/Mixer Rate Control" },
	{ 2, 26, 129, "Section On/Off Control" },
	{ 2, 26, 130, "Product Pressure" },
	{ 2, 26, 132, "Feeder/Mixer Machine Control" },
	{ 2, 26, 133, "Product Flow" },
	{ 2, 26, 134, "Product Level" },
	{ 2, 26, 135, "Boom Height Control" },
	{ 2, 27, 132, "Weeder Machine Control" },
	{ 2, 28, 132, "Turf and Lawn Care Mowers Machine Control" },
	{ 2, 29, 132, "Product/Material Handling Machine Control" },
	{ 2, 29, 133, "Product/Material Handling Product Flow" },
	{ 2, 29, 134, "Product/Material Handling Product Level" },
	{ 2, 127, 255, "Not Available" },

	// IG 3
	{ 3, 0, 128, "Supplemental Engine Control Sensing" },
	{ 3, 0, 129, "Laser Receiver" },
	{ 3, 0, 130, "Land Leveling System Operator Interface" },
	{ 3, 0, 131, "Land Leveling Electric Mast" },
	{ 3, 0, 132, "Single Land Leveling System Supervisor" },
	{ 3, 0, 133, "Land Leveling System Display" },
	{ 3, 0, 134, "Laser Tracer" },
	{ 3, 0, 135, "Loader Control" },
	{ 3, 0, 136, "Slope Sensor" },
	{ 3, 0, 137, "Liftarm Control" },
	{ 3, 0, 138, "Supplemental Sensor Processing Units" },
	{ 3, 0, 139, "Hydraulic System Planner" },
	{ 3, 0, 140, "Hydraulic Valve Controller" },
	{ 3, 0, 141, "Joystick Control" },
	{ 3, 0, 142, "Rotation Sensor" },
	{ 3, 0, 143, "Sonic Sensor" },
	{ 3, 0, 144, "Survey Total Station Target" },
	{ 3, 0, 145, "Heading Sensor" },
	{ 3, 0, 146, "Alarm device" },
	{ 3, 0, 255, "Not Available" },
	{ 3, 1, 128, "Main Controller" },
	{ 3, 1, 255, "Not Available" },
	{ 3, 2, 255, "Not Available" },
	{ 3, 3, 255, "Not Available" },
	{ 3, 4, 128, "Blade Controller" },
	{ 3, 4, 255, "Not Available" },
	{ 3, 5, 128, "Slope Sensor" },
	{ 3, 5, 255, "Not Available" },
	{ 3, 6, 255, "Not Available" },
	{ 3, 7, 255, "Not Available" },
	{ 3, 8, 128, "HFWD Controller" },
	{ 3, 8, 255, "Not Available" },
	{ 3, 9, 255, "Not Available" },
	{ 3, 10, 255, "Not Available" },
	{ 3, 11, 255, "Not Available" },
	{ 3, 12, 255, "Not Available" },
	{ 3, 13, 255, "Not Available" },
	{ 3, 14, 255, "Not Available" },
	{ 3, 15, 255, "Not Available" },
	{ 3, 16, 255, "Not Available" },
	{ 3, 17, 255, "Not Available" },
	{ 3, 127, 255, "Not Available" },

	// IG 4
	{ 4, 0, 128, "Alarm System Control for Marine Engines" },
	{ 4, 0, 129, "Protection System for Marine Engines" },
	{ 4, 0, 130, "Display for Protection System for Marine Engines" },
	{ 4, 0, 255, "Not Available" },
	{ 4, 10, 255, "Not Available" },
	{ 4, 20, 255, "Not Available" },
	{ 4, 25, 130, "" },
	{ 4, 30, 130, "Switch" },
	{ 4, 30, 140, "Load" },
	{ 4, 40, 130, "Follow-up Controller" },
	{ 4, 40, 140, "Mode Controller" },
	{ 4, 40, 150, "Automatic Steering Controller" },
	{ 4, 40, 160, "Heading Sensors" },
	{ 4, 50, 130, "Engineroom monitoring" },
	{ 4, 50, 140, "Engine Interface" },
	{ 4, 50, 150, "Engine Controller" },
	{ 4, 50, 160, "Engine Gateway" },
	{ 4, 50, 170, "Control Head" },
	{ 4, 50, 180, "Actuator" },
	{ 4, 50, 190, "Gauge Interface" },
	{ 4, 50, 200, "Gauge Large" },
	{ 4, 50, 210, "Gauge Small" },
	{ 4, 50, 220, "Propulsion Sensors & Gateway" },
	{ 4, 60, 130, "Sounder, depth" },
	{ 4, 60, 140, "" },
	{ 4, 60, 145, "Global Navigation Satellite System (GNSS)" },
	{ 4, 60, 150, "Loran C" },
	{ 4, 60, 155, "Speed Sensors" },
	{ 4, 60, 160, "Turn Rate Indicator" },
	{ 4, 60, 170, "Integrated Navigation" },
	{ 4, 60, 200, "Radar and/or Radar Plotting" },
	{ 4, 60, 205, "Electronic Chart Display & Information System (ECDIS)" },
	{ 4, 60, 210, "Electronic Chart System (ECS)" },
	{ 4, 60, 220, "Direction Finder" },
	{ 4, 70, 130, "Emergency Position Indicating Beacon (EPIRB)" },
	{ 4, 70, 140, "Automatic Identification System" },
	{ 4, 70, 150, "Digital Selective Calling (DSC)" },
	{ 4, 70, 160, "Data Receiver" },
	{ 4, 70, 170, "Satellite" },
	{ 4, 70, 180, "Radio-Telephone (MF/HF)" },
	{ 4, 70, 190, "Radio-Telephone (VHF)" },
	{ 4, 80, 130, "Time/Date systems" },
	{ 4, 80, 140, "Voyage Data Recorder" },
	{ 4, 80, 150, "Integrated Instrumentation" },
	{ 4, 80, 160, "General Purpose Displays" },
	{ 4, 80, 170, "General Sensor Box" },
	{ 4, 80, 180, "Weather Instruments" },
	{ 4, 80, 190, "Transducer/general" },
	{ 4, 80, 200, "NMEA 0183 Converter" },
	{ 4, 90, 255, "Not Available" },
	{ 4, 100, 255, "Not Available" },
	{ 4, 127, 255, "Not Available" },

	// IG 5
	{ 5, 0, 128, "Supplemental Engine Control Sensing" },
	{ 5, 0, 129, "Generator Set Controller" },
	{ 5, 0, 130, "Generator Voltage Regulator" },
	{ 5, 0, 131, "Choke Actuator" },
	{ 5, 0, 132, "Well Stimulation Pump" },
	{ 5, 0, 255, "Not Available" },
	{ 5, 127, 255, "Not Available" },
};

static const char *lookup_function_description(std::uint8_t industryGroup, std::uint8_t vehicleSystem, std::uint8_t functionCode)
{
	for (const auto &entry : kFunctionNames)
	{
		if ((entry.fc == functionCode) && (entry.ig == industryGroup) && (entry.vs == vehicleSystem))
		{
			return entry.name;
		}
	}
	for (const auto &entry : kFunctionNames)
	{
		if ((entry.fc == functionCode) && (255 == entry.ig) && (255 == entry.vs))
		{
			return entry.name;
		}
	}
	return "Unknown";
}
extern "C" void app_main()
{
	isobus::CANStackLogger::set_can_stack_logger_sink(&g_logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info);
	setvbuf(stdout, nullptr, _IONBF, 0);
	printf("app_main start\n");

	// ADC setup (ESP32-S3 ADC1 channels)
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO7
	adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); // GPIO3
	adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11); // GPIO6s

	esp_adc_cal_characteristics_t adc_chars;
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);


	twai_general_config_t gcfg = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_4, GPIO_NUM_5, TWAI_MODE_NORMAL);
	twai_timing_config_t tcfg = TWAI_TIMING_CONFIG_250KBITS();
	twai_filter_config_t fcfg = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	auto driver = std::make_shared<isobus::TWAIPlugin>(&gcfg, &tcfg, &fcfg);
	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, driver);
	isobus::CANHardwareInterface::set_periodic_update_interval(10);
	if ((!isobus::CANHardwareInterface::start()) || (!driver->get_is_valid()))
	{
		printf("CAN start failed\n");
		while (true) vTaskDelay(pdMS_TO_TICKS(1000));
	}

	vTaskDelay(pdMS_TO_TICKS(250));

	
	isobus::NAME myNAME(0);
	myNAME.set_arbitrary_address_capable(true);
	myNAME.set_industry_group(2);  
	myNAME.set_device_class(0);
	myNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::MachineControl));
	myNAME.set_identity_number(2);
	myNAME.set_manufacturer_code(1407);
	auto internal = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(myNAME, 0);

	std::vector<isobus::NAMEFilter> vtFilters = { isobus::NAMEFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal)) };
	auto partnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtFilters);


	auto vt = std::make_shared<isobus::VirtualTerminalClient>(partnerVT, internal);

	
	const uint8_t *poolStart = object_pool_start;
	const uint8_t *poolEnd   = object_pool_end;
	std::size_t poolSize = static_cast<std::size_t>(poolEnd - poolStart);
	vt->set_object_pool(0, poolStart, static_cast<std::uint32_t>(poolSize), "VTPOOL");
	vt->initialize(true);

	bool prevConnected = false;
	TickType_t lastLogTick = 0;
	TickType_t lastMeterUpdateTick = 0;
	TickType_t lastTextUpdateTick = 0;
	TickType_t lastAdcUpdateTick = 0;
	std::string lastParticipantsText;
	std::string lastAdcText11009;
	std::string lastAdcText11010;
	std::string lastAdcText11011;

	auto send_text_if_changed = [vt](std::uint16_t objectId, const std::string &text, std::string &cache) {
		if (text == cache)
		{
			return;
		}
		bool ok = vt->send_change_string_value(objectId, text);
		if (!ok)
		{
			printf("VT send_change_string_value failed for ID %u\n", static_cast<unsigned int>(objectId));
		}
		else
		{
			cache = text;
		}
	};

	auto send_text_force = [vt](std::uint16_t objectId, const std::string &text, std::string &cache) {
		bool ok = vt->send_change_string_value(objectId, text);
		if (!ok)
		{
			printf("VT send_change_string_value failed for ID %u\n", static_cast<unsigned int>(objectId));
		}
		cache = text;
	};

	while (true)
	{
		bool connected = vt->get_is_connected();
		if (connected && !prevConnected)
		{
			vt->set_object_pool(0, poolStart, static_cast<std::uint32_t>(poolSize), "VTPOOL");
			printf("VT connected, pool assigned\n");
			if (!lastParticipantsText.empty())
			{
				send_text_force(11012, lastParticipantsText, lastParticipantsText);
			}
			if (!lastAdcText11009.empty())
			{
				send_text_force(11009, lastAdcText11009, lastAdcText11009);
			}
			if (!lastAdcText11010.empty())
			{
				send_text_force(11010, lastAdcText11010, lastAdcText11010);
			}
			if (!lastAdcText11011.empty())
			{
				send_text_force(11011, lastAdcText11011, lastAdcText11011);
			}
		}
		if (connected)
		{
			TickType_t now = xTaskGetTickCount();
			if ((now - lastMeterUpdateTick) >= pdMS_TO_TICKS(500))
			{
				float busLoad = isobus::CANNetworkManager::CANNetwork.get_estimated_busload(0);
				if (busLoad < 0.0f) busLoad = 0.0f;
				if (busLoad > 100.0f) busLoad = 100.0f;
				std::uint32_t meterValue = static_cast<std::uint32_t>(busLoad + 0.5f);
				vt->send_change_numeric_value(17001, meterValue);
				lastMeterUpdateTick = now;
			}
			if ((now - lastTextUpdateTick) >= pdMS_TO_TICKS(1000))
			{
				std::string participantsText;
				auto controlFunctions = isobus::CANNetworkManager::CANNetwork.get_control_functions(false);
				for (const auto &cf : controlFunctions)
				{
					if ((nullptr == cf) || (!cf->get_address_valid()))
					{
						continue;
					}
					isobus::NAME name = cf->get_NAME();
					const char *funcName = lookup_function_description(name.get_industry_group(), name.get_device_class(), name.get_function_code());
					char line[96];
					std::snprintf(line,
					              sizeof(line),
					              "A%u FC%u M%u ID%lu %s\n",
					              static_cast<unsigned int>(cf->get_address()),
					              static_cast<unsigned int>(name.get_function_code()),
					              static_cast<unsigned int>(name.get_manufacturer_code()),
					              static_cast<unsigned long>(name.get_identity_number()),
					              funcName);
					participantsText += line;
				}
				if (participantsText.empty())
				{
					participantsText = "keine Teilnehmer\n";
				}
				if (participantsText.size() > 480)
				{
					participantsText.resize(480);
				}
				// OutputString 11012 now uses its own value (no string variable)
				send_text_if_changed(11012, participantsText, lastParticipantsText);
				lastTextUpdateTick = now;
			}
			if ((now - lastAdcUpdateTick) >= pdMS_TO_TICKS(500))
			{
				auto read_adc_voltage = [&adc_chars](adc1_channel_t channel) -> float {
					int raw = adc1_get_raw(channel);
					uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
					return static_cast<float>(mv) / 1000.0f;
				};

				const float dividerRatio = 5.0f; // 30k / 7.5k divider => V_in = V_adc * 5
				float v7 = read_adc_voltage(ADC1_CHANNEL_6) * dividerRatio;
				float v3 = read_adc_voltage(ADC1_CHANNEL_2) * dividerRatio;
				float v6 = read_adc_voltage(ADC1_CHANNEL_5) * dividerRatio;

				char buf[16];
				// These output strings reference string variables in the object pool.
				// 11009 -> StringVariable 22000, 11010 -> 22001, 11011 -> 22002
				std::snprintf(buf, sizeof(buf), "%.2f V", v6);
				send_text_if_changed(22000, buf, lastAdcText11009);
				std::snprintf(buf, sizeof(buf), "%.2f V", v7);
				send_text_if_changed(22001, buf, lastAdcText11010);
				std::snprintf(buf, sizeof(buf), "%.2f V", v3);
				send_text_if_changed(22002, buf, lastAdcText11011);
				lastAdcUpdateTick = now;
			}
		}
		TickType_t now = xTaskGetTickCount();
		if ((now - lastLogTick) >= pdMS_TO_TICKS(2000))
		{
			printf("VT %s\n", connected ? "connected" : "not connected");
			lastLogTick = now;
		}
		prevConnected = connected;
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}
