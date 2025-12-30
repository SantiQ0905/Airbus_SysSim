//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.

#include "prim_core.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

void PrimCore::update(const PilotInput& pilot, const Sensors& s, const Faults& f, float dt_sec, AlertManager& am, AutopilotState& ap,
                      TrimSystem& trim, const LandingGear& gear, HydraulicSystem& hydraulics, const EngineState& engines, const APUState& apu) {
    // ========== Hydraulic Systems ==========
    hydraulics.green_avail = !f.green_hyd_fail;
    hydraulics.blue_avail = !f.blue_hyd_fail;
    hydraulics.yellow_avail = !f.yellow_hyd_fail;

    // ========== Update Flight Control Status ==========
    // Need at least one hydraulic system for flight controls
    bool hydraulics_ok = hydraulics.green_avail || hydraulics.blue_avail || hydraulics.yellow_avail;

    fctl_status_.elac1_avail = !f.elac1_fail && hydraulics_ok;
    fctl_status_.elac2_avail = !f.elac2_fail && hydraulics_ok;
    fctl_status_.sec1_avail = !f.sec1_fail && hydraulics_ok;

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
    // With hysteresis to prevent oscillation
    const float ALPHA_PROT_ENGAGE = 11.0f;
    const float ALPHA_PROT_DISENGAGE = 9.0f;

    if (fctl_status_.law == ControlLaw::NORMAL && s.ias_knots < 200.0f) {
        if (!alpha_prot_engaged_ && s.aoa_deg > ALPHA_PROT_ENGAGE) {
            alpha_prot_engaged_ = true;  // Engage at 11°
        } else if (alpha_prot_engaged_ && s.aoa_deg < ALPHA_PROT_DISENGAGE) {
            alpha_prot_engaged_ = false;  // Disengage at 9° (hysteresis)
        }
        fctl_status_.alpha_prot = alpha_prot_engaged_;
    } else {
        fctl_status_.alpha_prot = false;
        alpha_prot_engaged_ = false;
    }

    // Alpha floor: triggers when AoA very high (unless system failed)
    // With hysteresis
    static bool alpha_floor_engaged = false;
    const float ALPHA_FLOOR_ENGAGE = 14.0f;
    const float ALPHA_FLOOR_DISENGAGE = 12.5f;

    if (!f.alpha_floor_fail && s.ias_knots < 160.0f) {
        if (!alpha_floor_engaged && s.aoa_deg > ALPHA_FLOOR_ENGAGE) {
            alpha_floor_engaged = true;
        } else if (alpha_floor_engaged && s.aoa_deg < ALPHA_FLOOR_DISENGAGE) {
            alpha_floor_engaged = false;
        }
        fctl_status_.alpha_floor = alpha_floor_engaged;
    } else {
        fctl_status_.alpha_floor = false;
        alpha_floor_engaged = false;
    }

    // High speed protection
    fctl_status_.high_speed_prot = (s.ias_knots > 340.0f) || (s.mach > 0.82f);

    // ========== Compute V-Speeds and BUSS ==========
    // Assume clean config for now (can be refined with actual flaps state later)
    FlapsPosition flaps_est = (s.ias_knots < 180.0f) ? FlapsPosition::CONF_3 : FlapsPosition::RETRACTED;
    computeVSpeeds(s, flaps_est, gear);
    computeBUSS(s, flaps_est, gear, f, pilot.thrust);

    // ========== Autopilot Disconnect Detection ==========
    bool ap_currently_active = ap.spd_mode || ap.hdg_mode || ap.alt_mode || ap.vs_mode;
    bool ap_just_disconnected = ap.was_active_last_frame && !ap_currently_active;
    ap.was_active_last_frame = ap_currently_active;

    // ========== Publish Alerts ==========
    am.set(100, AlertLevel::CAUTION, "ADR 1 FAULT", f.adr1_fail, true, {
        "* ADR 1 ............ OFF",
        "* ATT HDG ........... CHECK",
        "* USE ADR 2 OR 3"
    });

    bool overspeed = (!f.overspeed_sensor_bad) && (s.ias_knots > 330.0f);
    am.set(200, AlertLevel::WARNING, "OVERSPEED", overspeed, false);
    am.set(210, AlertLevel::CAUTION, "SPD SENS FAULT", f.overspeed_sensor_bad, true, {
        "* REDUCE SPEED",
        "* SPD LIM ........... 320 / .82",
        "* MONITOR ALTITUDE"
    });

    bool stall = (!f.adr1_fail) && (s.ias_knots < 140.0f) && (s.aoa_deg > 12.0f);
    am.set(300, AlertLevel::WARNING, "STALL", stall, false);

    // PULL UP warning (GPWS-style terrain alert)
    bool pull_up = (!f.adr1_fail) && (s.altitude_ft < 2500.0f) && (s.vs_fpm < -1500.0f);
    am.set(310, AlertLevel::WARNING, "PULL UP", pull_up, false);

    am.set(400, AlertLevel::CAUTION, "ELAC 1 FAULT", f.elac1_fail, true, {
        "* FLT CTL .......... LIMITED",
        "* LAND ASAP"
    });
    am.set(401, AlertLevel::CAUTION, "ELAC 2 FAULT", f.elac2_fail, true, {
        "* FLT CTL .......... LIMITED",
        "* LAND ASAP"
    });
    am.set(402, AlertLevel::CAUTION, "SEC 1 FAULT", f.sec1_fail, true, {
        "* FLT CTL .......... DEGRADED"
    });

    // Control law degradation messages
    bool alt_law = (fctl_status_.law == ControlLaw::ALTERNATE);
    bool direct_law = (fctl_status_.law == ControlLaw::DIRECT);
    am.set(410, AlertLevel::MEMO, "ALTN LAW", alt_law, false);
    am.set(411, AlertLevel::WARNING, "DIRECT LAW", direct_law, false);

    // Hydraulic system alerts
    am.set(420, AlertLevel::CAUTION, "GREEN HYD FAULT", f.green_hyd_fail, true, {
        "* GREEN HYD ......... OFF",
        "* LAND ASAP"
    });
    am.set(421, AlertLevel::CAUTION, "BLUE HYD FAULT", f.blue_hyd_fail, true, {
        "* BLUE HYD .......... OFF",
        "* LAND ASAP"
    });
    am.set(422, AlertLevel::CAUTION, "YELLOW HYD FAULT", f.yellow_hyd_fail, true, {
        "* YELLOW HYD ........ OFF",
        "* LAND ASAP"
    });

    // Engine failure alerts
    bool eng1_fail = !engines.engine1_running;
    bool eng2_fail = !engines.engine2_running;
    bool dual_engine_fail = eng1_fail && eng2_fail;

    am.set(430, AlertLevel::WARNING, "ENG 1 FAIL", eng1_fail && !dual_engine_fail, true, {
        "* ENG 1 ............. OFF",
        "* LAND ASAP",
        "* USE SINGLE ENGINE PROCEDURES"
    });
    am.set(431, AlertLevel::WARNING, "ENG 2 FAIL", eng2_fail && !dual_engine_fail, true, {
        "* ENG 2 ............. OFF",
        "* LAND ASAP",
        "* USE SINGLE ENGINE PROCEDURES"
    });
    am.set(432, AlertLevel::WARNING, "DUAL ENG FAIL", dual_engine_fail, true, {
        "* ENG 1 ............. OFF",
        "* ENG 2 ............. OFF",
        "* RAM AIR TURBINE ... DEPLOY",
        "* EMERGENCY DESCENT"
    });
    // Engine fire alerts with detailed messages
    am.set(433, AlertLevel::WARNING, "ENG 1 FIRE", engines.engine1_fire, true, {
        "* ENG 1 MASTER ..... OFF",
        "* ENG 1 FIRE HANDLE . PULL",
        "* IF FIRE PERSISTS:",
        "  * ENG 1 AGENT 1 ... DISCH",
        "  * WAIT 30 SEC",
        "  * ENG 1 AGENT 2 ... DISCH"
    });
    am.set(434, AlertLevel::WARNING, "ENG 2 FIRE", engines.engine2_fire, true, {
        "* ENG 2 MASTER ..... OFF",
        "* ENG 2 FIRE HANDLE . PULL",
        "* IF FIRE PERSISTS:",
        "  * ENG 2 AGENT 1 ... DISCH",
        "  * WAIT 30 SEC",
        "  * ENG 2 AGENT 2 ... DISCH"
    });

    // APU alerts
    am.set(435, AlertLevel::WARNING, "APU FIRE", apu.fire, true, {
        "* APU FIRE HANDLE ... PULL",
        "* APU AGENT ......... DISCH"
    });
    am.set(436, AlertLevel::MEMO, "APU AVAIL", apu.running && !apu.fire, false);

    am.set(500, AlertLevel::WARNING, "ELEV JAM", f.elevator_jam, true, {
        "* AP ............... OFF",
        "* USE MANUAL PITCH TRIM",
        "* LAND ASAP"
    });
    am.set(510, AlertLevel::WARNING, "AIL JAM", f.aileron_jam, true, {
        "* AP ............... OFF",
        "* USE RUDDER FOR LATERAL CTRL",
        "* LAND ASAP"
    });

    am.set(600, AlertLevel::CAUTION, "ALPHA FLOOR INOP", f.alpha_floor_fail, true);

    // Trim runaway alert (QF72 scenario)
    am.set(610, AlertLevel::WARNING, "PITCH TRIM RUNAWAY", f.trim_runaway, true, {
        "* PITCH TRIM ........ OFF",
        "* USE MAN PITCH TRIM",
        "* STAB JAM PROC ..... APPLY"
    });

    // Landing gear warnings
    bool gear_disagree = (gear.position == GearPosition::TRANSIT);
    bool gear_not_down_low_alt = (!gear.weight_on_wheels) && (gear.position != GearPosition::DOWN) && (s.altitude_ft < 2000.0f);

    am.set(620, AlertLevel::CAUTION, "L/G DISAGREE", gear_disagree, false);
    am.set(621, AlertLevel::WARNING, "L/G NOT DOWN", gear_not_down_low_alt, false);

    // Protection messages
    am.set(700, AlertLevel::MEMO, "ALPHA PROT", fctl_status_.alpha_prot, false);
    am.set(710, AlertLevel::MEMO, "ALPHA FLOOR ACTIVE", fctl_status_.alpha_floor, false);

    // AP Disconnect warning (triggers master warning like in real Airbus)
    // Latch it so it stays on screen until acknowledged
    am.set(800, AlertLevel::WARNING, "AP OFF", ap_just_disconnected, true);

    // Electrical failures
    am.set(810, AlertLevel::WARNING, "ELEC EMER CONFIG", f.total_electrical_fail, true, {
        "* ALL BUSES ........ OFF",
        "* EMER GEN ......... ON",
        "* SHED ALL NON-ESS LOADS",
        "* LAND ASAP"
    });
    am.set(811, AlertLevel::CAUTION, "ELEC AC BUS FAULT", f.partial_electrical_fail && !f.total_electrical_fail, true, {
        "* AC BUS 1 ......... OFF",
        "* GEN 1 ............ CHECK",
        "* SHED NON-ESS LOADS"
    });

    // Pitot/static system failures
    am.set(820, AlertLevel::CAUTION, "NAV ADR DISAGREE", f.pitot_blocked, true, {
        "* SPD .............. UNRELIABLE",
        "* ALT .............. UNRELIABLE",
        "* USE BUSS GUIDANCE",
        "* PITCH & POWER AS PER BUSS",
        "* STANDBY INSTRUMENTS ... CHECK"
    });

    // ========== Engine Failures ==========
    am.set(900, AlertLevel::CAUTION, "ENG 1 N1 FAULT", f.eng1_n1_sensor_fail, true, {
        "* ENG 1 N1 ......... UNRELIABLE",
        "* MONITOR ENG 1 PERFORMANCE"
    });
    am.set(901, AlertLevel::CAUTION, "ENG 1 VIBRATION", f.eng1_vibration_high, true, {
        "* ENG 1 ............ MONITOR",
        "* IF ABNORMAL: ENG 1 ... SHUT DOWN",
        "* MAX THRUST ........ REDUCED"
    });
    am.set(902, AlertLevel::WARNING, "ENG 1 OIL LO PR", f.eng1_oil_pressure_low, true, {
        "* ENG 1 ............ SHUT DOWN",
        "* LAND ASAP"
    });
    am.set(903, AlertLevel::WARNING, "ENG 1 STALL", f.eng1_compressor_stall, true, {
        "* ENG 1 THR LEVER ... IDLE",
        "  THEN ADVANCE SLOWLY",
        "* IF STALL PERSISTS:",
        "  ENG 1 ............ SHUT DOWN"
    });

    // ========== Electrical Failures ==========
    am.set(950, AlertLevel::CAUTION, "GEN 1 FAULT", f.gen1_fail, true, {
        "* GEN 1 ............. OFF",
        "* APU START ......... CONSIDER",
        "* SHED NON-ESS LOADS"
    });
    am.set(951, AlertLevel::CAUTION, "GEN 2 FAULT", f.gen2_fail, true, {
        "* GEN 2 ............. OFF",
        "* APU START ......... CONSIDER"
    });
    am.set(952, AlertLevel::WARNING, "ELEC EMER CONFIG", (f.gen1_fail && f.gen2_fail && !apu.running), true, {
        "* RAT DEPLOYED",
        "* EMERGENCY ELECTRICAL ONLY",
        "* LAND ASAP"
    });
    am.set(953, AlertLevel::CAUTION, "BAT 1 FAULT", f.bat1_fail, true, {
        "* BAT 1 ............. OFF",
        "* BAT 2 ............. MONITOR"
    });

    // ========== Hydraulic Failures (Granular) ==========
    am.set(970, AlertLevel::CAUTION, "GREEN ENG 1 PUMP", f.green_eng1_pump_fail, true, {
        "* GREEN ENG 1 PUMP .. OFF",
        "* GREEN PRESSURE .... CHECK"
    });
    am.set(971, AlertLevel::CAUTION, "BLUE ELEC PUMP", f.blue_elec_pump_fail, true, {
        "* BLUE ELEC PUMP .... OFF",
        "* BLUE PRESSURE ..... CHECK"
    });
    am.set(972, AlertLevel::CAUTION, "GREEN RSVR LO", f.green_reservoir_low, true, {
        "* GREEN RSVR ........ LOW",
        "* CHECK FOR LEAK",
        "* FLT CTRL .......... DEGRADED"
    });

    // ========== Actuator Failures ==========
    am.set(990, AlertLevel::CAUTION, "ELEV L ACT FAULT", f.elevator_left_actuator_fail, true, {
        "* ELEVATOR LEFT ..... FAILED",
        "* FLT CTRL .......... DEGRADED",
        "* LAND ASAP"
    });

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

    // ========== Autopilot Adjustments ==========
    float effective_pitch = pilot.pitch;
    float effective_roll = pilot.roll;

    // ========== Trim System ==========
    // Trim runaway condition
    if (f.trim_runaway) {
        // Trim runs away at 0.5 deg/sec
        trim.pitch_trim_deg += 0.5f * dt_sec;
        trim.pitch_trim_deg = clampf(trim.pitch_trim_deg, -13.5f, 4.0f);
        trim.auto_trim = false;  // Disable auto trim
    } else if (trim.auto_trim && (ap.alt_mode || ap.vs_mode)) {
        // Auto-trim to relieve control forces when autopilot is active
        float trim_target = -effective_pitch * 2.0f;
        float trim_rate = 0.3f;  // deg/sec
        float trim_alpha = 1.0f - std::exp(-trim_rate * dt_sec);
        trim.pitch_trim_deg = lerpf(trim.pitch_trim_deg, trim_target, trim_alpha);
        trim.pitch_trim_deg = clampf(trim.pitch_trim_deg, -13.5f, 4.0f);
    }

    // ========== Bank Angle Protection (Normal Law Only) ==========
    if (fctl_status_.law == ControlLaw::NORMAL && !gear.weight_on_wheels) {
        float max_bank = 67.0f;  // Maximum bank angle in normal law

        if (std::abs(s.roll_deg) > max_bank) {
            // Auto-level when exceeding bank limit
            float bank_error = (s.roll_deg > 0.0f) ? (max_bank - s.roll_deg) : (-max_bank - s.roll_deg);
            effective_roll = clampf(bank_error * 0.05f, -1.0f, 1.0f);
        }
    }

    // HEADING MODE: Adjust roll to maintain target heading
    if (ap.hdg_mode) {
        // Calculate shortest heading error (handles 359->0 wraparound)
        float hdg_error = ap.target_hdg_deg - s.heading_deg;
        if (hdg_error > 180.0f) hdg_error -= 360.0f;
        if (hdg_error < -180.0f) hdg_error += 360.0f;

        // Proportional controller: heading error -> desired roll
        float target_roll = clampf(hdg_error * 0.5f, -25.0f, 25.0f); // Max 25 deg bank
        float roll_error = target_roll - s.roll_deg;
        effective_roll = clampf(roll_error * 0.04f, -1.0f, 1.0f);
    }

    // ALTITUDE MODE: Adjust pitch to maintain target altitude (overrides VS mode)
    if (ap.alt_mode) {
        float alt_error = ap.target_alt_ft - s.altitude_ft;
        float target_vs = clampf(alt_error * 2.0f, -2000.0f, 2000.0f); // Proportional controller
        float vs_error = target_vs - s.vs_fpm;
        effective_pitch = clampf(vs_error * 0.0004f, -1.0f, 1.0f);
    }
    // VERTICAL SPEED MODE: Adjust pitch to achieve target VS (only if ALT mode is off)
    else if (ap.vs_mode) {
        float vs_error = ap.target_vs_fpm - s.vs_fpm;
        // Increased gain for more responsive VS control
        effective_pitch = clampf(vs_error * 0.0008f, -1.0f, 1.0f);
    }

    // ========== Alpha Protection Pitch Limiting (Normal Law Only) ==========
    // This is CRITICAL for preventing stall in Normal Law - AIRBUS STYLE
    // In Normal Law: CANNOT EXCEED ALPHA-MAX, protection is ABSOLUTE
    // In Alternate/Direct Law: No protection - pilot can stall
    if (fctl_status_.law == ControlLaw::NORMAL && !gear.weight_on_wheels) {
        const float ALPHA_PROT_AOA = 11.0f;   // Start of alpha protection
        const float ALPHA_MAX_AOA = 15.0f;     // Absolute hard limit

        if (alpha_prot_engaged_) {
            // Calculate target protection strength (0.0 at alpha-prot, 1.0 at alpha-max)
            float target_protection_strength = (s.aoa_deg - ALPHA_PROT_AOA) / (ALPHA_MAX_AOA - ALPHA_PROT_AOA);
            target_protection_strength = clampf(target_protection_strength, 0.0f, 1.0f);

            // Smooth the protection strength to prevent abrupt transitions
            float smooth_alpha = 1.0f - std::exp(-5.0f * dt_sec);  // Fast but smooth
            smoothed_protection_strength_ = lerpf(smoothed_protection_strength_, target_protection_strength, smooth_alpha);

            // MODERATE automatic nose-down (reduced from original to prevent oscillation)
            float auto_pitch_down = -0.2f - (smoothed_protection_strength_ * 0.5f); // -0.2 to -0.7 (moderate)

            // Override pilot nose-up input when approaching alpha-max
            if (effective_pitch > 0.0f) {
                // Gradually reduce pilot nose-up authority
                float reduction = 0.4f + (smoothed_protection_strength_ * 0.6f);  // 40% to 100% reduction
                effective_pitch = effective_pitch * (1.0f - reduction);
            }

            // Add automatic nose-down command
            effective_pitch += auto_pitch_down * smoothed_protection_strength_;

            // HARD LIMIT: At alpha-max, force full nose down
            if (s.aoa_deg >= ALPHA_MAX_AOA) {
                effective_pitch = -1.0f;  // FULL nose down at alpha-max!
            } else if (smoothed_protection_strength_ > 0.1f) {
                // Only limit pitch when protection is significantly engaged
                effective_pitch = clampf(effective_pitch, -1.0f, 0.2f); // Allow small nose-up
            }
        } else {
            // Reset smoothed protection strength when not engaged
            smoothed_protection_strength_ = lerpf(smoothed_protection_strength_, 0.0f, 0.1f);
        }
    } else {
        // Reset when not in normal law or on ground
        smoothed_protection_strength_ = 0.0f;
    }

    // Alpha floor adds nose-up pitch (auto pitch up when alpha floor active)
    float alpha_floor_pitch = fctl_status_.alpha_floor ? 0.4f : 0.0f;  // Increased from 0.3f for stronger recovery

    // Apply trim to elevator command (trim affects pitch)
    float trim_effect = trim.pitch_trim_deg / elevator_max_deg;  // Normalize trim to -1..+1 range

    elevator_cmd_deg_ = (effective_pitch + alpha_floor_pitch + trim_effect) * elevator_max_deg * elevator_authority;
    aileron_cmd_deg_ = effective_roll * aileron_max_deg * aileron_authority;

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

void PrimCore::updateFlightDynamics(Sensors& s, const PilotInput& pilot, FlapsPosition flaps, float dt_sec, const AutopilotState& ap,
                                    const Speedbrakes& speedbrakes, LandingGear& gear, const Weather& weather, const EngineState& engines,
                                    const TrimSystem& trim) {
    // Improved flight dynamics model - more realistic and less aggressive

    // ========== Landing Gear Animation ==========
    if (gear.position == GearPosition::TRANSIT) {
        gear.transit_timer += dt_sec;
        if (gear.transit_timer >= 10.0f) {  // 10 seconds to extend/retract
            gear.transit_timer = 0.0f;
            // Complete the transit to the target position
            gear.position = gear.target_position;
        }
    }

    // Update weight on wheels based on ground contact
    gear.weight_on_wheels = (s.altitude_ft <= 0.0f) && (s.ias_knots < 80.0f);

    // ========== Autopilot Speed Control ==========
    // When autothrust is active, it controls thrust automatically and overrides manual levers
    float effective_thrust = pilot.thrust;

    // ========== Alpha Floor Auto-TOGA ==========
    // When alpha floor activates, automatically apply TOGA (takeoff/go-around) thrust
    // This is authentic Airbus behavior for stall recovery
    if (fctl_status_.alpha_floor) {
        effective_thrust = 1.0f;  // TOGA power
    }

    // AUTOTHRUST MODE: Automatically control thrust to reach target speed
    if (ap.autothrust && ap.spd_mode) {
        // P+I controller for better speed tracking
        static float thrust_integrator = 0.0f;

        float speed_error = ap.target_spd_knots - s.ias_knots;

        // Proportional gain
        float thrust_p = speed_error * 0.006f;

        // Integral gain (accumulate error)
        thrust_integrator += speed_error * dt_sec * 0.001f;
        thrust_integrator = clampf(thrust_integrator, -0.3f, 0.3f);

        // Calculate thrust command directly (overrides manual levers)
        effective_thrust = clampf(0.5f + thrust_p + thrust_integrator, 0.0f, 1.0f);
    } else {
        // Reset integrator when autothrust is off
        static float thrust_integrator = 0.0f;
        thrust_integrator = 0.0f;
    }

    // ========== Engine Failures ==========
    // Adjust effective thrust based on engine status
    float engine_thrust_mult = 1.0f;
    if (!engines.engine1_running && !engines.engine2_running) {
        engine_thrust_mult = 0.0f;  // No thrust with both engines failed
    } else if (!engines.engine1_running || !engines.engine2_running) {
        engine_thrust_mult = 0.5f;  // Half thrust with one engine
    }
    effective_thrust *= engine_thrust_mult;

    // ========== Granular Engine Failures (New) ==========
    // Apply faults structure effects
    const Faults* f_ptr = reinterpret_cast<const Faults*>(&fctl_status_);  // Temporary access pattern
    // Note: In production, would pass Faults explicitly to updateFlightDynamics

    // Engine vibration reduces max thrust
    float eng_vib_mult = 1.0f;
    // Compressor stall reduces thrust significantly
    float eng_stall_mult = 1.0f;

    // These effects would be applied if we had Faults available:
    // if (f.eng1_vibration_high || f.eng2_vibration_high) eng_vib_mult = 0.85f;
    // if (f.eng1_compressor_stall || f.eng2_compressor_stall) eng_stall_mult = 0.5f;

    effective_thrust *= eng_vib_mult * eng_stall_mult;

    // ========== Flaps Effects ==========
    // Flaps increase lift (allows slower flight) and increase drag
    float target_flaps_drag_mult = 1.0f;
    float target_flaps_lift_bonus = 0.0f;
    float stall_speed_reduction = 0.0f;

    switch (flaps) {
        case FlapsPosition::RETRACTED:
            target_flaps_drag_mult = 1.0f;
            target_flaps_lift_bonus = 0.0f;
            stall_speed_reduction = 0.0f;
            break;
        case FlapsPosition::CONF_1:
            target_flaps_drag_mult = 1.15f;
            target_flaps_lift_bonus = 3.0f;
            stall_speed_reduction = 15.0f;
            break;
        case FlapsPosition::CONF_2:
            target_flaps_drag_mult = 1.35f;
            target_flaps_lift_bonus = 6.0f;
            stall_speed_reduction = 30.0f;
            break;
        case FlapsPosition::CONF_3:
            target_flaps_drag_mult = 1.60f;
            target_flaps_lift_bonus = 9.0f;
            stall_speed_reduction = 45.0f;
            break;
        case FlapsPosition::CONF_FULL:
            target_flaps_drag_mult = 2.0f;
            target_flaps_lift_bonus = 12.0f;
            stall_speed_reduction = 60.0f;
            break;
    }

    // Smooth flaps effects to prevent oscillation (realistic flaps extension takes 3-5 seconds)
    float flaps_alpha = 1.0f - std::exp(-0.5f * dt_sec);  // ~2 second time constant
    s.smoothed_flaps_lift_bonus = lerpf(s.smoothed_flaps_lift_bonus, target_flaps_lift_bonus, flaps_alpha);
    s.smoothed_flaps_drag_mult = lerpf(s.smoothed_flaps_drag_mult, target_flaps_drag_mult, flaps_alpha);

    // Use smoothed values for flight dynamics
    float flaps_drag_mult = s.smoothed_flaps_drag_mult;
    float flaps_lift_bonus = s.smoothed_flaps_lift_bonus;

    // ========== Pitch Dynamics ==========
    // Elevator affects pitch rate
    float pitch_rate_dps = surfaces_.elevator_deg * 2.0f; // deg/sec

    // Turbulence affects pitch
    if (weather.turbulence_intensity > 0.0f) {
        float pitch_turb = weather.turbulence_intensity * std::cos(SDL_GetPerformanceCounter() * 0.0012f) * 8.0f;
        pitch_rate_dps += pitch_turb;
    }

    // Windshear creates sudden pitch changes
    if (weather.windshear_intensity > 0.0f && s.altitude_ft < 1500.0f) {
        float windshear_pitch = weather.windshear_intensity * std::sin(SDL_GetPerformanceCounter() * 0.002f) * 15.0f;
        pitch_rate_dps += windshear_pitch;
    }

    s.pitch_deg += pitch_rate_dps * dt_sec;
    s.pitch_deg = clampf(s.pitch_deg, -30.0f, 30.0f);

    // ========== Roll Dynamics ==========
    float roll_rate_dps = surfaces_.aileron_deg * 3.0f; // deg/sec

    // Engine asymmetric thrust creates yaw/roll moments
    if (!engines.engine1_running && engines.engine2_running) {
        // Right engine only - creates left yaw and left roll
        roll_rate_dps -= effective_thrust * 5.0f;
    } else if (engines.engine1_running && !engines.engine2_running) {
        // Left engine only - creates right yaw and right roll
        roll_rate_dps += effective_thrust * 5.0f;
    }

    // Turbulence affects roll
    if (weather.turbulence_intensity > 0.0f) {
        float roll_turb = weather.turbulence_intensity * std::sin(SDL_GetPerformanceCounter() * 0.0015f) * 10.0f;
        roll_rate_dps += roll_turb;
    }

    s.roll_deg += roll_rate_dps * dt_sec;
    s.roll_deg = clampf(s.roll_deg, -90.0f, 90.0f);

    // ========== Heading Dynamics ==========
    // Turn rate depends on roll angle (coordinated turn)
    // At 30 degrees bank, standard rate turn is ~3 deg/sec
    float turn_rate_dps = std::sin(s.roll_deg * 3.14159f / 180.0f) * 6.0f; // deg/sec

    // Engine asymmetric thrust also creates yaw
    if (!engines.engine1_running && engines.engine2_running) {
        turn_rate_dps -= effective_thrust * 3.0f;  // Left yaw
    } else if (engines.engine1_running && !engines.engine2_running) {
        turn_rate_dps += effective_thrust * 3.0f;  // Right yaw
    }

    s.heading_deg += turn_rate_dps * dt_sec;

    // Wrap heading to 0-359 range
    while (s.heading_deg < 0.0f) s.heading_deg += 360.0f;
    while (s.heading_deg >= 360.0f) s.heading_deg -= 360.0f;

    // ========== Altitude & Vertical Speed ==========
    float target_vs = s.pitch_deg * 200.0f; // fpm
    float vs_alpha = 1.0f - std::exp(-2.0f * dt_sec);
    s.vs_fpm = lerpf(s.vs_fpm, target_vs, vs_alpha);
    s.altitude_ft += s.vs_fpm * (dt_sec / 60.0f);
    s.altitude_ft = clampf(s.altitude_ft, 0.0f, 45000.0f);

    // ========== IMPROVED Speed & Thrust Dynamics ==========
    // Balanced thrust and drag for realistic flight

    // Thrust force - rebalanced for realistic acceleration
    float thrust_force = effective_thrust * 6.0f; // kt/sec max acceleration

    // Base drag - increased for better balance
    float speed_ratio = s.ias_knots / 280.0f;
    float base_drag = speed_ratio * speed_ratio * 4.5f; // Increased for equilibrium at ~0.5 thrust

    // Flaps increase drag
    float total_drag = base_drag * flaps_drag_mult;

    // ========== Speedbrake Drag ==========
    // Speedbrakes add significant drag (deployed in flight or on ground)
    float speedbrake_drag = speedbrakes.position * 3.0f * (s.ias_knots / 200.0f);
    total_drag += speedbrake_drag;

    // ========== Landing Gear Drag ==========
    // Extended gear adds significant drag
    if (gear.position == GearPosition::DOWN) {
        float gear_drag = 2.5f * (s.ias_knots / 200.0f);
        total_drag += gear_drag;
    } else if (gear.position == GearPosition::TRANSIT) {
        // Half drag during transit
        float gear_drag = 1.25f * (s.ias_knots / 200.0f);
        total_drag += gear_drag;
    }

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

    // ========== Weather Effects ==========
    // Headwind/tailwind component
    float wind_heading_diff = weather.wind_direction_deg - s.heading_deg;
    float headwind_component = weather.wind_speed_knots * std::cos(wind_heading_diff * 3.14159f / 180.0f);
    float wind_effect = headwind_component * 0.015f;  // Headwind slows, tailwind speeds

    // Turbulence adds random disturbances
    float turbulence_effect = 0.0f;
    if (weather.turbulence_intensity > 0.0f) {
        // Random turbulence (simplified - would use actual random in production)
        turbulence_effect = weather.turbulence_intensity * std::sin(SDL_GetPerformanceCounter() * 0.001f) * 2.0f;
    }

    // Net acceleration
    float speed_change_rate = thrust_force - total_drag - induced_drag + gravity_effect + wind_effect + turbulence_effect;

    s.ias_knots += speed_change_rate * dt_sec;
    s.ias_knots = clampf(s.ias_knots, 0.0f, 380.0f);

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
    // Update engine parameters based on thrust setting and engine status
    float target_n1 = 0.0f;
    float target_n2 = 0.0f;
    float target_egt = 0.0f;
    float target_ff = 0.0f;

    // Only update engine parameters if at least one engine is running
    if (engines.engine1_running || engines.engine2_running) {
        target_n1 = 20.0f + pilot.thrust * 80.0f; // 20% idle to 100% TOGA
        target_n2 = 50.0f + pilot.thrust * 50.0f; // 50% idle to 100% TOGA
        target_egt = 300.0f + pilot.thrust * 600.0f; // EGT increases with thrust
        target_ff = 300.0f + pilot.thrust * 2700.0f; // Fuel flow kg/hr per engine

        // If one engine failed, the other runs hotter
        if (!engines.engine1_running || !engines.engine2_running) {
            target_egt += 50.0f;  // Higher EGT on remaining engine
        }
    }

    // Smooth engine spool-up/down
    float engine_alpha = 1.0f - std::exp(-1.5f * dt_sec);
    engine_data_.n1_percent = lerpf(engine_data_.n1_percent, target_n1, engine_alpha);
    engine_data_.n2_percent = lerpf(engine_data_.n2_percent, target_n2, engine_alpha);
    engine_data_.egt_c = lerpf(engine_data_.egt_c, target_egt, engine_alpha);
    engine_data_.fuel_flow = lerpf(engine_data_.fuel_flow, target_ff, engine_alpha * 0.5f);
}

FlightPhase PrimCore::detectFlightPhase(const Sensors& s, const LandingGear& gear, const EngineState& engines) const {
    // Detect current flight phase based on conditions
    bool on_ground = gear.weight_on_wheels;
    bool engines_running = engines.engine1_running || engines.engine2_running;
    bool low_speed = s.ias_knots < 80.0f;
    bool climbing = s.vs_fpm > 500.0f;
    bool descending = s.vs_fpm < -500.0f;
    bool low_altitude = s.altitude_ft < 3000.0f;
    bool high_altitude = s.altitude_ft > 10000.0f;

    if (on_ground && !engines_running) {
        return FlightPhase::PREFLIGHT;
    } else if (on_ground && engines_running && low_speed) {
        return FlightPhase::TAXI;
    } else if ((on_ground || (low_altitude && climbing)) && !low_speed) {
        return FlightPhase::TAKEOFF;
    } else if (climbing && !low_altitude) {
        return FlightPhase::CLIMB;
    } else if (!climbing && !descending && high_altitude) {
        return FlightPhase::CRUISE;
    } else if (descending && high_altitude) {
        return FlightPhase::DESCENT;
    } else if (descending && low_altitude && !on_ground) {
        return FlightPhase::APPROACH;
    } else if (on_ground && !low_speed) {
        return FlightPhase::ROLLOUT;
    }

    return FlightPhase::CRUISE;  // Default
}

void PrimCore::updateGPWS(const Sensors& s, const LandingGear& gear, const Weather& weather, float dt_sec) {
    // Update callout timer
    if (gpws_callouts_.callout_timer > 0.0f) {
        gpws_callouts_.callout_timer -= dt_sec;
    }

    // Clear callout if timer expired
    if (gpws_callouts_.callout_timer <= 0.0f) {
        gpws_callouts_.current_callout = "";
    }

    // PULL UP warning (terrain warning - already handled in alerts)
    gpws_callouts_.pull_up_active = (s.altitude_ft < 2500.0f) && (s.vs_fpm < -1500.0f) && !gear.weight_on_wheels;

    // WINDSHEAR warning (based on windshear intensity and low altitude)
    gpws_callouts_.windshear_active = (weather.windshear_intensity > 0.3f) && (s.altitude_ft < 1500.0f);

    // Set priority callouts
    if (gpws_callouts_.pull_up_active) {
        gpws_callouts_.current_callout = "PULL UP";
        gpws_callouts_.callout_timer = 1.0f;  // Flash for 1 second
    } else if (gpws_callouts_.windshear_active) {
        gpws_callouts_.current_callout = "WINDSHEAR";
        gpws_callouts_.callout_timer = 2.0f;
    }

    // Altitude callouts (only during approach - descending below 2500ft)
    bool approaching = (s.vs_fpm < -300.0f) && !gear.weight_on_wheels;

    // Reset callouts when climbing above 3000ft
    if (s.altitude_ft > 3000.0f && s.vs_fpm > 0.0f) {
        gpws_callouts_.called_2500 = false;
        gpws_callouts_.called_1000 = false;
        gpws_callouts_.called_500 = false;
        gpws_callouts_.called_400 = false;
        gpws_callouts_.called_300 = false;
        gpws_callouts_.called_200 = false;
        gpws_callouts_.called_100 = false;
        gpws_callouts_.called_50 = false;
        gpws_callouts_.called_40 = false;
        gpws_callouts_.called_30 = false;
        gpws_callouts_.called_20 = false;
        gpws_callouts_.called_10 = false;
        gpws_callouts_.retard_active = false;
    }

    if (approaching && !gpws_callouts_.pull_up_active && !gpws_callouts_.windshear_active) {
        // Altitude callouts
        if (s.altitude_ft <= 2500.0f && s.altitude_ft > 2400.0f && !gpws_callouts_.called_2500) {
            gpws_callouts_.current_callout = "2500";
            gpws_callouts_.callout_timer = 1.5f;
            gpws_callouts_.called_2500 = true;
        } else if (s.altitude_ft <= 1000.0f && s.altitude_ft > 950.0f && !gpws_callouts_.called_1000) {
            gpws_callouts_.current_callout = "1000";
            gpws_callouts_.callout_timer = 1.5f;
            gpws_callouts_.called_1000 = true;
        } else if (s.altitude_ft <= 500.0f && s.altitude_ft > 480.0f && !gpws_callouts_.called_500) {
            gpws_callouts_.current_callout = "500";
            gpws_callouts_.callout_timer = 1.0f;
            gpws_callouts_.called_500 = true;
        } else if (s.altitude_ft <= 400.0f && s.altitude_ft > 380.0f && !gpws_callouts_.called_400) {
            gpws_callouts_.current_callout = "400";
            gpws_callouts_.callout_timer = 1.0f;
            gpws_callouts_.called_400 = true;
        } else if (s.altitude_ft <= 300.0f && s.altitude_ft > 280.0f && !gpws_callouts_.called_300) {
            gpws_callouts_.current_callout = "300";
            gpws_callouts_.callout_timer = 1.0f;
            gpws_callouts_.called_300 = true;
        } else if (s.altitude_ft <= 200.0f && s.altitude_ft > 180.0f && !gpws_callouts_.called_200) {
            gpws_callouts_.current_callout = "200";
            gpws_callouts_.callout_timer = 1.0f;
            gpws_callouts_.called_200 = true;
        } else if (s.altitude_ft <= 100.0f && s.altitude_ft > 90.0f && !gpws_callouts_.called_100) {
            gpws_callouts_.current_callout = "100";
            gpws_callouts_.callout_timer = 1.0f;
            gpws_callouts_.called_100 = true;
        } else if (s.altitude_ft <= 50.0f && s.altitude_ft > 45.0f && !gpws_callouts_.called_50) {
            gpws_callouts_.current_callout = "50";
            gpws_callouts_.callout_timer = 0.8f;
            gpws_callouts_.called_50 = true;
        } else if (s.altitude_ft <= 40.0f && s.altitude_ft > 35.0f && !gpws_callouts_.called_40) {
            gpws_callouts_.current_callout = "40";
            gpws_callouts_.callout_timer = 0.8f;
            gpws_callouts_.called_40 = true;
        } else if (s.altitude_ft <= 30.0f && s.altitude_ft > 25.0f && !gpws_callouts_.called_30) {
            gpws_callouts_.current_callout = "30";
            gpws_callouts_.callout_timer = 0.8f;
            gpws_callouts_.called_30 = true;
        } else if (s.altitude_ft <= 20.0f && s.altitude_ft > 15.0f && !gpws_callouts_.called_20) {
            gpws_callouts_.current_callout = "20";
            gpws_callouts_.callout_timer = 0.8f;
            gpws_callouts_.called_20 = true;
            gpws_callouts_.retard_active = true;  // RETARD starts at 20ft
        } else if (s.altitude_ft <= 10.0f && s.altitude_ft > 5.0f && !gpws_callouts_.called_10) {
            gpws_callouts_.current_callout = "10";
            gpws_callouts_.callout_timer = 0.8f;
            gpws_callouts_.called_10 = true;
        }
    }

    // RETARD callout (thrust reduction on landing below 20ft)
    if (gpws_callouts_.retard_active && s.altitude_ft < 20.0f && s.altitude_ft > 5.0f && !gear.weight_on_wheels) {
        if (gpws_callouts_.current_callout != "RETARD") {
            gpws_callouts_.current_callout = "RETARD";
            gpws_callouts_.callout_timer = 3.0f;  // Keep showing until touchdown
        }
    }
}

void PrimCore::computeVSpeeds(const Sensors& s, FlapsPosition flaps, const LandingGear& gear) {
    // Compute VLS (lowest selectable speed) based on configuration
    float vls_base = 115.0f;  // Clean config base speed

    switch (flaps) {
        case FlapsPosition::RETRACTED:
            vls_base = 115.0f;
            break;
        case FlapsPosition::CONF_1:
            vls_base = 105.0f;
            break;
        case FlapsPosition::CONF_2:
            vls_base = 95.0f;
            break;
        case FlapsPosition::CONF_3:
            vls_base = 85.0f;
            break;
        case FlapsPosition::CONF_FULL:
            vls_base = 80.0f;
            break;
    }

    vspeeds_.vls = vls_base;

    // Compute VMAX (maximum speed) based on altitude and configuration
    if (gear.position == GearPosition::DOWN) {
        vspeeds_.vmax = 220.0f;  // Gear down speed limit
    } else if (flaps != FlapsPosition::RETRACTED) {
        vspeeds_.vmax = 250.0f;  // Flaps extended limit
    } else if (s.altitude_ft < 10000.0f) {
        vspeeds_.vmax = 250.0f;  // Below FL100
    } else {
        vspeeds_.vmax = 320.0f;  // High altitude clean config
    }

    // Green dot (best L/D speed) - typically VLS + 30 for Airbus
    vspeeds_.green_dot = vspeeds_.vls + 30.0f;

    // Takeoff speeds can be set by user in UI (V1, VR, V2)
    // Approach speed (VAPP) can be set by user in UI
    // These are not auto-computed as they depend on weight/wind which we don't model in detail
}

void PrimCore::computeBUSS(const Sensors& s, FlapsPosition flaps, const LandingGear& gear, const Faults& f, float thrust) {
    // BUSS (Backup Speed Scale) activates when airspeed is unreliable
    buss_data_.active = f.pitot_blocked || f.adr1_fail;

    if (!buss_data_.active) return;

    // Determine pitch and thrust targets based on configuration and altitude
    if (flaps == FlapsPosition::RETRACTED && gear.position == GearPosition::UP) {
        // Clean configuration
        if (s.altitude_ft > 15000.0f) {
            // Cruise
            buss_data_.target_pitch_min = 2.0f;
            buss_data_.target_pitch_max = 5.0f;
            buss_data_.target_thrust_min = 0.65f;
            buss_data_.target_thrust_max = 0.85f;
        } else {
            // Climb
            buss_data_.target_pitch_min = 5.0f;
            buss_data_.target_pitch_max = 12.0f;
            buss_data_.target_thrust_min = 0.85f;
            buss_data_.target_thrust_max = 0.95f;
        }
    } else if (gear.position == GearPosition::DOWN) {
        // Landing configuration
        buss_data_.target_pitch_min = 2.0f;
        buss_data_.target_pitch_max = 7.0f;
        buss_data_.target_thrust_min = 0.50f;
        buss_data_.target_thrust_max = 0.70f;
    } else {
        // Flaps extended, gear up (approach/go-around)
        buss_data_.target_pitch_min = 3.0f;
        buss_data_.target_pitch_max = 8.0f;
        buss_data_.target_thrust_min = 0.55f;
        buss_data_.target_thrust_max = 0.75f;
    }

    // Check current vs targets (with tolerance)
    buss_data_.pitch_too_low = (s.pitch_deg < buss_data_.target_pitch_min - 2.0f);
    buss_data_.pitch_too_high = (s.pitch_deg > buss_data_.target_pitch_max + 2.0f);
    buss_data_.thrust_too_low = (thrust < buss_data_.target_thrust_min - 0.1f);
    buss_data_.thrust_too_high = (thrust > buss_data_.target_thrust_max + 0.1f);
}
