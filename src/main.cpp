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

        prim.update(pilot, sensors, faults, dt, alerts);

        // Update flight dynamics unless in manual override mode (for QF72-style scenarios)
        if (!sim_settings.manual_sensor_override) {
            prim.updateFlightDynamics(sensors, pilot, flaps, dt);
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        DrawMasterPanel(alerts);
        DrawEcamPanel(alerts, sensors, pilot, faults, prim, flaps);
        DrawPFDPanel(sensors, prim, pilot);
        DrawFctlPanel(prim, faults);
        DrawControlInputPanel(pilot, sensors, faults, sim_settings, flaps);

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
