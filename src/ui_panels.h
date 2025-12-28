//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.
#pragma once
#include "alerts.h"
#include "sim_types.h"
#include "prim_core.h"

void DrawMasterPanel(AlertManager& alerts);
void DrawEcamPanel(AlertManager& alerts, Sensors& sensors, PilotInput& pilot, Faults& faults, const PrimCore& prim, FlapsPosition flaps, EngineState& engines, APUState& apu);
void DrawFctlPanel(const PrimCore& prim, Faults& faults);
void DrawPFDPanel(const Sensors& sensors, const PrimCore& prim, const PilotInput& pilot, const AutopilotState& ap, const Faults& faults);
void DrawControlInputPanel(PilotInput& pilot, Sensors& sensors, Faults& faults, SimulationSettings& sim_settings, FlapsPosition& flaps);
void DrawAutopilotPanel(AutopilotState& ap, const Sensors& sensors);
void DrawSimOperationPanel(Weather& weather, Faults& faults);
void DrawAircraftSystemsPanel(PilotInput& pilot, FlapsPosition& flaps, TrimSystem& trim,
                               Speedbrakes& speedbrakes, LandingGear& gear,
                               HydraulicSystem& hydraulics, EngineState& engines, APUState& apu,
                               AlertManager& alerts, const AutopilotState& ap);
// Deprecated - use DrawSimOperationPanel and DrawAircraftSystemsPanel instead
void DrawSystemsPanel(TrimSystem& trim, Speedbrakes& speedbrakes, LandingGear& gear, FlightPhase phase,
                      HydraulicSystem& hydraulics, EngineState& engines, Weather& weather, Faults& faults);
