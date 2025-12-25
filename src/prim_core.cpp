//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.

#include "prim_core.h"
#include <algorithm>
#include <cmath>

static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

void PrimCore::update(const PilotInput& pilot, const Sensors& s, const Faults& f, float dt_sec, AlertManager& am) {
    // ========== Update Flight Control Status ==========
    fctl_status_.elac1_avail = !f.elac1_fail;
    fctl_status_.elac2_avail = !f.elac2_fail;
    fctl_status_.sec1_avail = !f.sec1_fail;

    // Determine control law based on failures
    // NORMAL LAW: All ELACs and SECs operational
    // ALTERNATE LAW: One or more ELAC failures
    // DIRECT LAW: Multiple critical failures (simplified)
    if (f.elac1_fail && f.elac2_fail) {
        fctl_status_.law = ControlLaw::DIRECT;
    } else if (f.elac1_fail || f.elac2_fail || f.sec1_fail) {
        fctl_status_.law = ControlLaw::ALTERNATE;
    } else {
        fctl_status_.law = ControlLaw::NORMAL;
    }

    // Alpha protection: active when AoA high and speed low (only in normal law)
    fctl_status_.alpha_prot = (fctl_status_.law == ControlLaw::NORMAL) &&
                               (s.aoa_deg > 11.0f) && (s.ias_knots < 200.0f);

    // Alpha floor: triggers when AoA very high (unless system failed)
    fctl_status_.alpha_floor = !f.alpha_floor_fail && (s.aoa_deg > 14.0f) && (s.ias_knots < 160.0f);

    // High speed protection
    fctl_status_.high_speed_prot = (s.ias_knots > 340.0f) || (s.mach > 0.82f);

    // ========== Publish Alerts ==========
    am.set(100, AlertLevel::CAUTION, "ADR 1 FAULT", f.adr1_fail, true);

    bool overspeed = (!f.overspeed_sensor_bad) && (s.ias_knots > 330.0f);
    am.set(200, AlertLevel::WARNING, "OVERSPEED", overspeed, false);
    am.set(210, AlertLevel::CAUTION, "SPD SENS FAULT", f.overspeed_sensor_bad, true);

    bool stall = (!f.adr1_fail) && (s.ias_knots < 140.0f) && (s.aoa_deg > 12.0f);
    am.set(300, AlertLevel::WARNING, "STALL", stall, false);

    am.set(400, AlertLevel::CAUTION, "ELAC 1 FAULT", f.elac1_fail, true);
    am.set(401, AlertLevel::CAUTION, "ELAC 2 FAULT", f.elac2_fail, true);
    am.set(402, AlertLevel::CAUTION, "SEC 1 FAULT", f.sec1_fail, true);

    // Control law degradation messages
    bool alt_law = (fctl_status_.law == ControlLaw::ALTERNATE);
    bool direct_law = (fctl_status_.law == ControlLaw::DIRECT);
    am.set(410, AlertLevel::MEMO, "ALTN LAW", alt_law, false);
    am.set(411, AlertLevel::WARNING, "DIRECT LAW", direct_law, false);

    am.set(500, AlertLevel::WARNING, "ELEV JAM", f.elevator_jam, true);
    am.set(510, AlertLevel::WARNING, "AIL JAM", f.aileron_jam, true);

    am.set(600, AlertLevel::CAUTION, "ALPHA FLOOR INOP", f.alpha_floor_fail, true);

    // Protection messages
    am.set(700, AlertLevel::MEMO, "ALPHA PROT", fctl_status_.alpha_prot, false);
    am.set(710, AlertLevel::MEMO, "ALPHA FLOOR ACTIVE", fctl_status_.alpha_floor, false);

    // NORMAL memo only if no cautions/warnings shown
    bool haveAnyNonMemo = false;
    for (const auto& a : am.all()) {
        bool shown = (a.active || a.latched);
        if (shown && a.level != AlertLevel::MEMO) { haveAnyNonMemo = true; break; }
    }
    am.set(900, AlertLevel::MEMO, "NORMAL", !haveAnyNonMemo, false);

    // ========== Control Law Implementation ==========
    float elevator_authority = 1.0f;
    float aileron_authority = 1.0f;

    // Authority reduction based on control law
    switch (fctl_status_.law) {
        case ControlLaw::NORMAL:
            elevator_authority = 1.0f;
            aileron_authority = 1.0f;
            break;
        case ControlLaw::ALTERNATE:
            elevator_authority = 0.65f; // Reduced authority
            aileron_authority = 0.70f;
            break;
        case ControlLaw::DIRECT:
            elevator_authority = 0.45f; // Minimal authority
            aileron_authority = 0.50f;
            break;
    }

    // Alpha floor adds nose-up pitch (auto pitch up when alpha floor active)
    float alpha_floor_pitch = fctl_status_.alpha_floor ? 0.3f : 0.0f;

    elevator_cmd_deg_ = (pilot.pitch + alpha_floor_pitch) * elevator_max_deg * elevator_authority;
    aileron_cmd_deg_ = pilot.roll * aileron_max_deg * aileron_authority;

    // Apply jams
    if (f.elevator_jam) elevator_cmd_deg_ = surfaces_.elevator_deg;
    if (f.aileron_jam) aileron_cmd_deg_ = surfaces_.aileron_deg;

    // Response dynamics (faster in direct law, slower in normal law with protections)
    float response_hz = (fctl_status_.law == ControlLaw::DIRECT) ? 12.0f : 8.0f;
    const float alpha = 1.0f - std::exp(-response_hz * dt_sec);

    surfaces_.elevator_deg = lerpf(surfaces_.elevator_deg, elevator_cmd_deg_, alpha);
    surfaces_.aileron_deg = lerpf(surfaces_.aileron_deg, aileron_cmd_deg_, alpha);

    surfaces_.elevator_deg = clampf(surfaces_.elevator_deg, -elevator_max_deg, elevator_max_deg);
    surfaces_.aileron_deg = clampf(surfaces_.aileron_deg, -aileron_max_deg, aileron_max_deg);
}

void PrimCore::updateFlightDynamics(Sensors& s, const PilotInput& pilot, FlapsPosition flaps, float dt_sec) {
    // Improved flight dynamics model - more realistic and less aggressive

    // ========== Flaps Effects ==========
    // Flaps increase lift (allows slower flight) and increase drag
    float flaps_drag_mult = 1.0f;
    float flaps_lift_bonus = 0.0f;
    float stall_speed_reduction = 0.0f;

    switch (flaps) {
        case FlapsPosition::RETRACTED:
            flaps_drag_mult = 1.0f;
            flaps_lift_bonus = 0.0f;
            stall_speed_reduction = 0.0f;
            break;
        case FlapsPosition::CONF_1:
            flaps_drag_mult = 1.15f;
            flaps_lift_bonus = 3.0f;
            stall_speed_reduction = 15.0f;
            break;
        case FlapsPosition::CONF_2:
            flaps_drag_mult = 1.35f;
            flaps_lift_bonus = 6.0f;
            stall_speed_reduction = 30.0f;
            break;
        case FlapsPosition::CONF_3:
            flaps_drag_mult = 1.60f;
            flaps_lift_bonus = 9.0f;
            stall_speed_reduction = 45.0f;
            break;
        case FlapsPosition::CONF_FULL:
            flaps_drag_mult = 2.0f;
            flaps_lift_bonus = 12.0f;
            stall_speed_reduction = 60.0f;
            break;
    }

    // ========== Pitch Dynamics ==========
    // Elevator affects pitch rate
    float pitch_rate_dps = surfaces_.elevator_deg * 2.0f; // deg/sec
    s.pitch_deg += pitch_rate_dps * dt_sec;
    s.pitch_deg = clampf(s.pitch_deg, -30.0f, 30.0f);

    // ========== Roll Dynamics ==========
    float roll_rate_dps = surfaces_.aileron_deg * 3.0f; // deg/sec
    s.roll_deg += roll_rate_dps * dt_sec;
    s.roll_deg = clampf(s.roll_deg, -60.0f, 60.0f);

    // ========== Altitude & Vertical Speed ==========
    float target_vs = s.pitch_deg * 200.0f; // fpm
    float vs_alpha = 1.0f - std::exp(-2.0f * dt_sec);
    s.vs_fpm = lerpf(s.vs_fpm, target_vs, vs_alpha);
    s.altitude_ft += s.vs_fpm * (dt_sec / 60.0f);
    s.altitude_ft = clampf(s.altitude_ft, 0.0f, 45000.0f);

    // ========== IMPROVED Speed & Thrust Dynamics ==========
    // Much stronger thrust, less aggressive drag

    // Thrust force (significantly increased from before)
    float thrust_force = pilot.thrust * 60.0f; // kt/sec max acceleration

    // Base drag (much reduced)
    float speed_ratio = s.ias_knots / 280.0f;
    float base_drag = speed_ratio * speed_ratio * 3.0f; // Reduced from 8.0

    // Flaps increase drag
    float total_drag = base_drag * flaps_drag_mult;

    // Induced drag from pitch (MUCH less aggressive)
    // Only significant at high AoA
    float induced_drag = 0.0f;
    if (s.aoa_deg > 5.0f) {
        float aoa_factor = (s.aoa_deg - 5.0f) / 10.0f;
        induced_drag = aoa_factor * aoa_factor * 2.0f; // Much gentler
    }

    // Gravity component (energy trade)
    // Climbing costs energy, descending gains it
    float gravity_effect = -s.pitch_deg * 0.12f; // Reduced from 0.2

    // Net acceleration
    float speed_change_rate = thrust_force - total_drag - induced_drag + gravity_effect;

    s.ias_knots += speed_change_rate * dt_sec;
    s.ias_knots = clampf(s.ias_knots, 60.0f, 380.0f);

    // Update Mach (simplified: IAS → Mach conversion, altitude dependent)
    float altitude_factor = 1.0f - (s.altitude_ft / 100000.0f); // Higher alt → higher mach for same IAS
    s.mach = (s.ias_knots / 600.0f) / clampf(altitude_factor, 0.5f, 1.0f);
    s.mach = clampf(s.mach, 0.0f, 0.95f);

    // ========== Angle of Attack ==========
    // AoA depends on pitch, speed, and flaps
    // Flaps allow higher AoA at lower speeds
    float speed_factor = clampf((250.0f - s.ias_knots) / 150.0f, -1.0f, 1.0f);
    float target_aoa = s.pitch_deg * 0.4f + speed_factor * 8.0f + flaps_lift_bonus;

    // Smooth AoA changes
    float aoa_alpha = 1.0f - std::exp(-3.0f * dt_sec);
    s.aoa_deg = lerpf(s.aoa_deg, target_aoa, aoa_alpha);
    s.aoa_deg = clampf(s.aoa_deg, -5.0f, 25.0f);

    // ========== Temperature ==========
    float isa_temp = 15.0f - (s.altitude_ft / 1000.0f) * 2.0f;
    s.tat_c = lerpf(s.tat_c, isa_temp, 0.1f * dt_sec);

    // ========== Load Factor ==========
    float pitch_g = std::abs(pitch_rate_dps) * 0.01f;
    float roll_g = std::abs(s.roll_deg) * 0.005f;
    s.nz = 1.0f + pitch_g + roll_g;
    s.nz = clampf(s.nz, -1.0f, 3.0f);

    // ========== Engine Simulation ==========
    // Update engine parameters based on thrust setting
    float target_n1 = 20.0f + pilot.thrust * 80.0f; // 20% idle to 100% TOGA
    float target_n2 = 50.0f + pilot.thrust * 50.0f; // 50% idle to 100% TOGA
    float target_egt = 300.0f + pilot.thrust * 600.0f; // EGT increases with thrust
    float target_ff = 300.0f + pilot.thrust * 2700.0f; // Fuel flow kg/hr per engine

    // Smooth engine spool-up/down
    float engine_alpha = 1.0f - std::exp(-1.5f * dt_sec);
    engine_data_.n1_percent = lerpf(engine_data_.n1_percent, target_n1, engine_alpha);
    engine_data_.n2_percent = lerpf(engine_data_.n2_percent, target_n2, engine_alpha);
    engine_data_.egt_c = lerpf(engine_data_.egt_c, target_egt, engine_alpha);
    engine_data_.fuel_flow = lerpf(engine_data_.fuel_flow, target_ff, engine_alpha * 0.5f);
}
