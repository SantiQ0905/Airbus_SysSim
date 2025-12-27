#include <SDL2/SDL.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include "sim_types.h"
#include "alerts.h"
#include "prim_core.h"
#include "ui_panels.h"

#include <algorithm>
#include <cmath>

static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;

    SDL_Window* window = SDL_CreateWindow(
        "AIRBUS PRIM FLIGHT CONTROL SIMULATOR",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1300, 730,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) return 1;

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    bool running = true;

    PilotInput pilot{};
    Sensors sensors{};
    Faults faults{};
    AlertManager alerts{};
    PrimCore prim{};
    SimulationSettings sim_settings{};
    FlapsPosition flaps = FlapsPosition::RETRACTED;
    AutopilotState autopilot{};
    TrimSystem trim{};
    Speedbrakes speedbrakes{};
    LandingGear gear{};
    FlightPhase flight_phase = FlightPhase::PREFLIGHT;
    HydraulicSystem hydraulics{};
    EngineState engines{};
    Weather weather{};

    // Startup scenario selection
    bool scenario_selected = false;
    StartupScenario selected_scenario = StartupScenario::CRUISE_10000FT;

    uint64_t lastCounter = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    while (running) {
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = (float)((now - lastCounter) / freq);
        lastCounter = now;
        dt = clampf(dt, 0.0f, 0.05f);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Show startup scenario selection dialog
        if (!scenario_selected) {
            ImGui::OpenPopup("Select Startup Scenario");

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Select Startup Scenario", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Choose initial flight conditions:");
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Ground Level - Parked", ImVec2(250, 40))) {
                    selected_scenario = StartupScenario::GROUND_PARKED;
                    applyStartupScenario(selected_scenario, sensors, pilot, autopilot, gear, engines);
                    scenario_selected = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Text("  Aircraft on ground, engines off");
                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::Button("10,000 ft - Cruise", ImVec2(250, 40))) {
                    selected_scenario = StartupScenario::CRUISE_10000FT;
                    applyStartupScenario(selected_scenario, sensors, pilot, autopilot, gear, engines);
                    scenario_selected = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Text("  In flight at 10,000 ft, 250 knots");
                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::Button("37,000 ft - High Altitude Cruise", ImVec2(250, 40))) {
                    selected_scenario = StartupScenario::CRUISE_37000FT;
                    applyStartupScenario(selected_scenario, sensors, pilot, autopilot, gear, engines);
                    scenario_selected = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Text("  In flight at 37,000 ft, Mach 0.78");

                ImGui::EndPopup();
            }
        }

        // Only run simulation after scenario is selected
        if (scenario_selected) {
            // Detect flight phase
            flight_phase = prim.detectFlightPhase(sensors, gear, engines);

            prim.update(pilot, sensors, faults, dt, alerts, autopilot, trim, gear, hydraulics, engines);

            // Update flight dynamics unless in manual override mode (for QF72-style scenarios)
            if (!sim_settings.manual_sensor_override) {
                prim.updateFlightDynamics(sensors, pilot, flaps, dt, autopilot, speedbrakes, gear, weather, engines, trim);
            }

            // Update GPWS callouts
            prim.updateGPWS(sensors, gear, weather, dt);

            DrawMasterPanel(alerts);
            DrawEcamPanel(alerts, sensors, pilot, faults, prim, flaps);
            DrawPFDPanel(sensors, prim, pilot, autopilot);
            DrawFctlPanel(prim, faults);
            DrawControlInputPanel(pilot, sensors, faults, sim_settings, flaps);
            DrawAutopilotPanel(autopilot, sensors);
            DrawSystemsPanel(trim, speedbrakes, gear, flight_phase, hydraulics, engines, weather, faults);
        }

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 12, 12, 12, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
