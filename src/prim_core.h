//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.
#pragma once
#include "sim_types.h"
#include "alerts.h"

class PrimCore {
public:
    float elevator_max_deg = 25.0f;
    float aileron_max_deg  = 20.0f;

    const Surfaces& surfaces() const { return surfaces_; }
    const FlightControlStatus& fctl_status() const { return fctl_status_; }
    const EngineData& engine_data() const { return engine_data_; }

    void update(const PilotInput& pilot, const Sensors& s, const Faults& f, float dt_sec, AlertManager& am, AutopilotState& ap);
    void updateFlightDynamics(Sensors& s, const PilotInput& pilot, FlapsPosition flaps, float dt_sec, const AutopilotState& ap);

private:
    Surfaces surfaces_{};
    FlightControlStatus fctl_status_{};
    EngineData engine_data_{};

    float elevator_cmd_deg_ = 0.0f;
    float aileron_cmd_deg_  = 0.0f;
};
