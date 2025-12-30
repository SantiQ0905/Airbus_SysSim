//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.
#pragma once
#include <string>

struct Sensors {
    float ias_knots = 250.0f;
    float aoa_deg   = 3.0f;
    float nz        = 1.0f;
    float altitude_ft = 10000.0f;
    float vs_fpm = 0.0f;
    float mach = 0.45f;
    float tat_c = -10.0f;
    float pitch_deg = 0.0f;
    float roll_deg = 0.0f;
    float heading_deg = 0.0f;  // Magnetic heading (0-359)

    // Smoothed flaps effects to prevent oscillation
    float smoothed_flaps_lift_bonus = 0.0f;
    float smoothed_flaps_drag_mult = 1.0f;
};

struct PilotInput {
    float pitch = 0.0f;  // -1..+1 (stick)
    float roll  = 0.0f;  // -1..+1 (stick)
    float thrust = 0.5f; // 0..1 (thrust levers: 0=idle, 0.5=climb, 1.0=TOGA)
};

enum class FlapsPosition {
    RETRACTED = 0,  // Clean config
    CONF_1 = 1,     // Flaps 1
    CONF_2 = 2,     // Flaps 2
    CONF_3 = 3,     // Flaps 3
    CONF_FULL = 4   // Flaps Full
};

struct VSpeeds {
    float v1 = 0.0f;        // Decision speed (knots)
    float vr = 0.0f;        // Rotation speed (knots)
    float v2 = 0.0f;        // Takeoff safety speed (knots)
    float vapp = 0.0f;      // Approach speed (knots)
    float vls = 0.0f;       // Lowest selectable speed (auto-computed)
    float vmax = 0.0f;      // Maximum speed (auto-computed)
    float green_dot = 0.0f; // Best L/D speed (auto-computed)
    bool display_takeoff_speeds = false;
    bool display_approach_speeds = false;
};

struct ILSData {
    bool localizer_valid = false;
    bool glideslope_valid = false;
    float localizer_deviation = 0.0f;  // -2.5 to +2.5 dots
    float glideslope_deviation = 0.0f; // -2.5 to +2.5 dots
};

struct BUSSData {
    bool active = false;
    float target_pitch_min = 0.0f;
    float target_pitch_max = 0.0f;
    float target_thrust_min = 0.0f;
    float target_thrust_max = 0.0f;
    bool pitch_too_low = false;
    bool pitch_too_high = false;
    bool thrust_too_low = false;
    bool thrust_too_high = false;
};

struct EngineData {
    float n1_percent = 50.0f;   // Engine N1 (fan speed)
    float n2_percent = 75.0f;   // Engine N2 (core speed)
    float egt_c = 450.0f;       // Exhaust Gas Temperature
    float fuel_flow = 1200.0f;  // kg/hr per engine
};

struct TrimSystem {
    float pitch_trim_deg = 0.0f;  // -13.5 to +4.0 degrees (Airbus range)
    bool auto_trim = true;         // Autopilot auto-trim
};

struct Speedbrakes {
    float position = 0.0f;  // 0.0 = retracted, 1.0 = fully extended
    bool armed = false;     // Armed for automatic ground deployment
};

enum class GearPosition {
    UP,
    DOWN,
    TRANSIT
};

struct LandingGear {
    GearPosition position = GearPosition::DOWN;
    GearPosition target_position = GearPosition::DOWN;  // Where gear is commanded to go
    bool weight_on_wheels = false;
    float transit_timer = 0.0f;  // Animation timer
};

enum class FlightPhase {
    PREFLIGHT,   // On ground, engines off
    TAXI,        // On ground, engines running
    TAKEOFF,     // Ground roll + initial climb
    CLIMB,       // Climbing to cruise
    CRUISE,      // Level cruise
    DESCENT,     // Descending
    APPROACH,    // Final approach
    LANDING,     // Flare and touchdown
    ROLLOUT      // Ground deceleration
};

struct HydraulicSystem {
    bool green_avail = true;
    bool blue_avail = true;
    bool yellow_avail = true;
};

struct EngineState {
    bool engine1_running = true;
    bool engine2_running = true;
    bool engine1_fire = false;
    bool engine2_fire = false;
    bool engine1_squib_released = false;  // Fire extinguisher agent released
    bool engine2_squib_released = false;
};

struct APUState {
    bool running = false;
    bool fire = false;
    bool squib_released = false;
};

struct Weather {
    float wind_speed_knots = 0.0f;
    float wind_direction_deg = 0.0f;  // Direction wind is coming FROM
    float turbulence_intensity = 0.0f;  // 0.0 = none, 1.0 = severe
    float windshear_intensity = 0.0f;   // 0.0 = none, 1.0 = severe
};

struct Faults {
    bool adr1_fail = false;
    bool elac1_fail = false;
    bool elac2_fail = false;
    bool sec1_fail = false;
    bool elevator_jam = false;
    bool aileron_jam  = false;
    bool overspeed_sensor_bad = false;
    bool alpha_floor_fail = false;
    bool trim_runaway = false;
    bool green_hyd_fail = false;
    bool blue_hyd_fail = false;
    bool yellow_hyd_fail = false;

    // Electrical faults
    bool total_electrical_fail = false;   // Complete electrical failure (very rare)
    bool partial_electrical_fail = false; // One bus failure (AC BUS 1 or similar)

    // Pitot/static system faults
    bool pitot_blocked = false;           // Pitot tube blockage -> unreliable airspeed

    // ========== Engine Sensor Failures ==========
    bool eng1_n1_sensor_fail = false;
    bool eng1_n2_sensor_fail = false;
    bool eng1_egt_sensor_fail = false;
    bool eng2_n1_sensor_fail = false;
    bool eng2_n2_sensor_fail = false;
    bool eng2_egt_sensor_fail = false;

    // ========== Engine Mechanical Failures ==========
    bool eng1_vibration_high = false;
    bool eng2_vibration_high = false;
    bool eng1_oil_pressure_low = false;
    bool eng2_oil_pressure_low = false;
    bool eng1_compressor_stall = false;
    bool eng2_compressor_stall = false;

    // ========== Electrical System Failures ==========
    // Generators
    bool gen1_fail = false;
    bool gen2_fail = false;
    bool apu_gen_fail = false;

    // Batteries
    bool bat1_fail = false;
    bool bat2_fail = false;

    // Buses
    bool ac_bus1_fail = false;
    bool ac_bus2_fail = false;

    // RAT (Ram Air Turbine - emergency generator)
    bool rat_deployed = false;
    bool rat_fault = false;

    // ========== Hydraulic System Failures (Granular) ==========
    // Pumps
    bool green_eng1_pump_fail = false;
    bool blue_elec_pump_fail = false;
    bool yellow_eng1_pump_fail = false;

    // Reservoirs
    bool green_reservoir_low = false;
    bool blue_reservoir_low = false;
    bool yellow_reservoir_low = false;

    // ========== Flight Control Actuator Failures ==========
    bool elevator_left_actuator_fail = false;
    bool elevator_right_actuator_fail = false;
    bool aileron_left_actuator_fail = false;
    bool aileron_right_actuator_fail = false;
};

struct SimulationSettings {
    bool manual_sensor_override = false; // When true, physics disabled for QF72-style scenarios
};

struct Surfaces {
    float elevator_deg = 0.0f;
    float aileron_deg  = 0.0f;
    float rudder_deg   = 0.0f;
};

enum class ControlLaw { NORMAL, ALTERNATE, DIRECT };

struct FlightControlStatus {
    ControlLaw law = ControlLaw::NORMAL;
    bool elac1_avail = true;
    bool elac2_avail = true;
    bool sec1_avail = true;
    bool sec2_avail = true;
    bool sec3_avail = true;
    bool alpha_prot = false;
    bool alpha_floor = false;
    bool high_speed_prot = false;
};

struct AutopilotState {
    bool spd_mode = false;      // Speed hold mode
    bool hdg_mode = false;      // Heading hold mode
    bool alt_mode = false;      // Altitude hold mode
    bool vs_mode = false;       // Vertical speed mode
    bool autothrust = false;    // Autothrust mode (A/THR)

    float target_spd_knots = 250.0f;  // Target speed
    float target_hdg_deg = 0.0f;      // Target heading
    float target_alt_ft = 10000.0f;   // Target altitude
    float target_vs_fpm = 0.0f;       // Target vertical speed

    bool was_active_last_frame = false; // Internal tracking for disconnect detection
};

// GPWS (Ground Proximity Warning System) callouts
struct GPWSCallouts {
    std::string current_callout = "";  // Current active callout
    float callout_timer = 0.0f;        // Time since callout started
    bool pull_up_active = false;       // PULL UP warning
    bool windshear_active = false;     // WINDSHEAR warning
    bool retard_active = false;        // RETARD callout (below 20ft)

    // Altitude callouts already made (to avoid repeating)
    bool called_2500 = false;
    bool called_1000 = false;
    bool called_500 = false;
    bool called_400 = false;
    bool called_300 = false;
    bool called_200 = false;
    bool called_100 = false;
    bool called_50 = false;
    bool called_40 = false;
    bool called_30 = false;
    bool called_20 = false;
    bool called_10 = false;
};

enum class StartupScenario {
    GROUND_PARKED,   // On ground, engines idle, ready for startup
    CRUISE_10000FT,  // In flight at 10,000 ft
    CRUISE_37000FT   // In flight at 37,000 ft (typical cruise altitude)
};

// Initialize simulation state based on selected startup scenario
inline void applyStartupScenario(StartupScenario scenario, Sensors& sensors, PilotInput& pilot, AutopilotState& autopilot,
                                  LandingGear& gear, EngineState& engines) {
    switch (scenario) {
        case StartupScenario::GROUND_PARKED:
            sensors.ias_knots = 0.0f;
            sensors.aoa_deg = 0.0f;
            sensors.nz = 1.0f;
            sensors.altitude_ft = 0.0f;
            sensors.vs_fpm = 0.0f;
            sensors.mach = 0.0f;
            sensors.tat_c = 15.0f;
            sensors.pitch_deg = 0.0f;
            sensors.roll_deg = 0.0f;
            sensors.heading_deg = 0.0f;
            pilot.pitch = 0.0f;
            pilot.roll = 0.0f;
            pilot.thrust = 0.0f;  // Engines at idle
            autopilot.target_alt_ft = 0.0f;
            autopilot.target_spd_knots = 0.0f;
            gear.position = GearPosition::DOWN;
            gear.weight_on_wheels = true;
            engines.engine1_running = false;
            engines.engine2_running = false;
            break;

        case StartupScenario::CRUISE_10000FT:
            sensors.ias_knots = 250.0f;
            sensors.aoa_deg = 3.0f;
            sensors.nz = 1.0f;
            sensors.altitude_ft = 10000.0f;
            sensors.vs_fpm = 0.0f;
            sensors.mach = 0.45f;
            sensors.tat_c = -10.0f;
            sensors.pitch_deg = 0.0f;
            sensors.roll_deg = 0.0f;
            sensors.heading_deg = 0.0f;
            pilot.pitch = 0.0f;
            pilot.roll = 0.0f;
            pilot.thrust = 0.60f;  // Balanced thrust for 250kt cruise
            autopilot.target_alt_ft = 10000.0f;
            autopilot.target_spd_knots = 250.0f;
            gear.position = GearPosition::UP;
            gear.weight_on_wheels = false;
            engines.engine1_running = true;
            engines.engine2_running = true;
            break;

        case StartupScenario::CRUISE_37000FT:
            sensors.ias_knots = 280.0f;  // Lower IAS at high altitude
            sensors.aoa_deg = 2.0f;
            sensors.nz = 1.0f;
            sensors.altitude_ft = 37000.0f;
            sensors.vs_fpm = 0.0f;
            sensors.mach = 0.78f;  // Typical cruise Mach
            sensors.tat_c = -54.0f;  // ISA at FL370
            sensors.pitch_deg = 0.0f;  // Level flight
            sensors.roll_deg = 0.0f;
            sensors.heading_deg = 0.0f;
            pilot.pitch = 0.0f;
            pilot.roll = 0.0f;
            pilot.thrust = 0.75f;  // Balanced thrust for 280kt cruise
            autopilot.target_alt_ft = 37000.0f;
            autopilot.target_spd_knots = 280.0f;
            gear.position = GearPosition::UP;
            gear.weight_on_wheels = false;
            engines.engine1_running = true;
            engines.engine2_running = true;
            break;
    }
}
