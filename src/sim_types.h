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

struct EngineData {
    float n1_percent = 50.0f;   // Engine N1 (fan speed)
    float n2_percent = 75.0f;   // Engine N2 (core speed)
    float egt_c = 450.0f;       // Exhaust Gas Temperature
    float fuel_flow = 1200.0f;  // kg/hr per engine
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
