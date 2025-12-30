//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.
#include "ui_panels.h"
#include "imgui.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

// Airbus color palette
namespace AirbusColors {
    const ImU32 GREEN   = IM_COL32(0, 255, 0, 255);      // Normal/OK
    const ImU32 CYAN    = IM_COL32(0, 255, 255, 255);    // Info/Memo
    const ImU32 AMBER   = IM_COL32(255, 191, 0, 255);    // Caution
    const ImU32 RED     = IM_COL32(255, 0, 0, 255);      // Warning
    const ImU32 WHITE   = IM_COL32(255, 255, 255, 255);  // Generic text
    const ImU32 MAGENTA = IM_COL32(255, 0, 255, 255);    // Special/Disagree
    const ImU32 DARK_BG = IM_COL32(10, 10, 15, 255);     // Background
}

static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }

static ImU32 alertColor(AlertLevel lvl) {
    switch (lvl) {
        case AlertLevel::WARNING: return AirbusColors::RED;
        case AlertLevel::CAUTION: return AirbusColors::AMBER;
        case AlertLevel::MEMO:    return AirbusColors::CYAN;
    }
    return AirbusColors::WHITE;
}

// Draw centered text
static void TextCentered(const char* text, ImU32 color = AirbusColors::WHITE) {
    float width = ImGui::GetContentRegionAvail().x;
    float text_width = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - text_width) * 0.5f);
    ImGui::TextColored(ImColor(color), "%s", text);
}

// Draw a titled box
static void BeginAirbusBox(const char* title, ImU32 title_color = AirbusColors::GREEN) {
    ImGui::PushStyleColor(ImGuiCol_Border, title_color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::BeginChild(title, ImVec2(0, 0), true);
    ImGui::PushStyleColor(ImGuiCol_Text, title_color);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

static void EndAirbusBox() {
    ImGui::EndChild();
}

// ================================
// MASTER WARNING/CAUTION Panel
// ================================
void DrawMasterPanel(AlertManager& alerts) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(350, 90), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("MASTER WARNING/CAUTION", nullptr);

    bool mw = alerts.masterWarningOn();
    bool mc = alerts.masterCautionOn();

    // Flashing master warning/caution (in real aircraft these flash)
    static float blink_timer = 0.0f;
    blink_timer += ImGui::GetIO().DeltaTime;
    bool blink_on = (fmod(blink_timer, 1.0f) < 0.5f);

    if (mw && blink_on) {
        ImGui::PushStyleColor(ImGuiCol_Button, AirbusColors::RED);
        ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::WHITE);
        ImGui::Button("MASTER WARNING", ImVec2(200, 40));
        ImGui::PopStyleColor(2);
    } else if (mc && blink_on) {
        ImGui::PushStyleColor(ImGuiCol_Button, AirbusColors::AMBER);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,0,0,255));
        ImGui::Button("MASTER CAUTION", ImVec2(200, 40));
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(30,30,30,255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80,80,80,255));
        ImGui::Button("NORMAL", ImVec2(200, 40));
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();
    if (ImGui::Button("CLR", ImVec2(60, 40))) alerts.acknowledgeAllVisible();
    ImGui::SameLine();
    if (ImGui::Button("RCL", ImVec2(60, 40))) alerts.clearAllLatched();

    ImGui::End();
    ImGui::PopStyleColor();
}

// ================================
// ECAM E/WD Display
// ================================
void DrawEcamPanel(AlertManager& alerts, Sensors& sensors, PilotInput& pilot, Faults& faults, const PrimCore& prim, FlapsPosition flaps, EngineState& engines, APUState& apu) {
    ImGui::SetNextWindowPos(ImVec2(10, 110), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(350, 480), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("ECAM E/WD", nullptr);

    // Title
    TextCentered("ENGINE / WARNING DISPLAY", AirbusColors::WHITE);
    ImGui::Separator();
    ImGui::Spacing();

    // Engine parameters section
    const auto& eng = prim.engine_data();
    ImGui::BeginChild("Engines", ImVec2(0, 120), true);
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "ENGINES (CFM56-5B)");
    ImGui::Separator();

    ImGui::Columns(3, nullptr, false);
    ImGui::Text("PARAM");
    ImGui::NextColumn();
    ImGui::Text("ENG 1");
    ImGui::NextColumn();
    ImGui::Text("ENG 2");
    ImGui::NextColumn();

    // N1
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "N1");
    ImGui::NextColumn();
    ImU32 n1_color = (eng.n1_percent > 95.0f) ? AirbusColors::RED : AirbusColors::GREEN;
    ImGui::TextColored(ImColor(n1_color), "%.1f%%", eng.n1_percent);
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(n1_color), "%.1f%%", eng.n1_percent);
    ImGui::NextColumn();

    // EGT
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "EGT");
    ImGui::NextColumn();
    ImU32 egt_color = (eng.egt_c > 800.0f) ? AirbusColors::RED : AirbusColors::GREEN;
    ImGui::TextColored(ImColor(egt_color), "%.0f C", eng.egt_c);
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(egt_color), "%.0f C", eng.egt_c);
    ImGui::NextColumn();

    // N2
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "N2");
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(AirbusColors::GREEN), "%.1f%%", eng.n2_percent);
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(AirbusColors::GREEN), "%.1f%%", eng.n2_percent);
    ImGui::NextColumn();

    // Fuel Flow
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "FF");
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(AirbusColors::GREEN), "%.0f kg/h", eng.fuel_flow);
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(AirbusColors::GREEN), "%.0f kg/h", eng.fuel_flow);

    ImGui::Columns(1);
    ImGui::EndChild();
    ImGui::Spacing();

    // Fire status display
    if (engines.engine1_fire || engines.engine2_fire || apu.fire) {
        ImGui::BeginChild("Fire", ImVec2(0, 60), true);
        ImGui::TextColored(ImColor(AirbusColors::RED), "FIRE DETECTION:");
        if (engines.engine1_fire) {
            ImGui::TextColored(ImColor(AirbusColors::RED), "  ENG 1 FIRE");
            if (engines.engine1_squib_released) {
                ImGui::SameLine();
                ImGui::TextColored(ImColor(AirbusColors::AMBER), "(AGENT DISCH)");
            }
        }
        if (engines.engine2_fire) {
            ImGui::TextColored(ImColor(AirbusColors::RED), "  ENG 2 FIRE");
            if (engines.engine2_squib_released) {
                ImGui::SameLine();
                ImGui::TextColored(ImColor(AirbusColors::AMBER), "(AGENT DISCH)");
            }
        }
        if (apu.fire) {
            ImGui::TextColored(ImColor(AirbusColors::RED), "  APU FIRE");
            if (apu.squib_released) {
                ImGui::SameLine();
                ImGui::TextColored(ImColor(AirbusColors::AMBER), "(AGENT DISCH)");
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();
    }

    // Flaps/Config display
    ImGui::BeginChild("Config", ImVec2(0, 40), true);
    const char* flaps_text;
    switch (flaps) {
        case FlapsPosition::RETRACTED: flaps_text = "CLEAN (0)"; break;
        case FlapsPosition::CONF_1: flaps_text = "CONF 1"; break;
        case FlapsPosition::CONF_2: flaps_text = "CONF 2"; break;
        case FlapsPosition::CONF_3: flaps_text = "CONF 3"; break;
        case FlapsPosition::CONF_FULL: flaps_text = "CONF FULL"; break;
    }
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "FLAPS:");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(AirbusColors::GREEN), "%s", flaps_text);
    ImGui::EndChild();
    ImGui::Spacing();

    // Warnings section
    auto warnings = alerts.getShownSorted(AlertLevel::WARNING);
    if (!warnings.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::RED);
        for (auto* a : warnings) {
            ImGui::Text("  * %s", a->text.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    // Cautions section
    auto cautions = alerts.getShownSorted(AlertLevel::CAUTION);
    if (!cautions.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::AMBER);
        for (auto* a : cautions) {
            ImGui::Text("  * %s", a->text.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    // ECAM Actions section (all alerts with actions, stacked)
    std::vector<const Alert*> alerts_with_actions;

    // Collect all warnings with actions first (highest priority)
    for (auto* a : warnings) {
        if (!a->ecam_actions.empty()) {
            alerts_with_actions.push_back(a);
        }
    }

    // Then collect all cautions with actions
    for (auto* a : cautions) {
        if (!a->ecam_actions.empty()) {
            alerts_with_actions.push_back(a);
        }
    }

    if (!alerts_with_actions.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::WHITE);
        ImGui::Text("ECAM ACTIONS:");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // Display all actions from all active alerts
        for (const auto* alert : alerts_with_actions) {
            for (const auto& action : alert->ecam_actions) {
                ImGui::TextColored(ImColor(AirbusColors::WHITE), "  %s", action.c_str());
            }
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    // Memos section
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::CYAN);
    auto memos = alerts.getShownSorted(AlertLevel::MEMO);
    for (auto* a : memos) {
        ImGui::Text("  %s", a->text.c_str());
    }
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor();
}

// Helper function to draw artificial horizon
static void DrawArtificialHorizon(ImVec2 center, float radius, float pitch_deg, float roll_deg) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Limit pitch display
    pitch_deg = clampf(pitch_deg, -30.0f, 30.0f);

    // Calculate horizon line position (pitch affects Y offset)
    float pitch_pixels_per_deg = radius / 15.0f; // 15 degrees fills half the ball
    float pitch_offset = pitch_deg * pitch_pixels_per_deg;

    // Roll rotation
    float roll_rad = roll_deg * 3.14159f / 180.0f;
    float cos_roll = std::cos(roll_rad);
    float sin_roll = std::sin(roll_rad);

    // Draw circle background (clipping region)
    draw_list->AddCircleFilled(center, radius, IM_COL32(20, 20, 30, 255), 64);

    // Push clip rect for horizon
    draw_list->PushClipRect(
        ImVec2(center.x - radius, center.y - radius),
        ImVec2(center.x + radius, center.y + radius),
        true
    );

    // Draw sky (blue) and ground (brown)
    ImVec2 horizon_start = ImVec2(center.x - radius * 2, center.y + pitch_offset);
    ImVec2 horizon_end = ImVec2(center.x + radius * 2, center.y + pitch_offset);

    // Rotate horizon points around center
    auto rotate_point = [&](ImVec2 p) -> ImVec2 {
        float dx = p.x - center.x;
        float dy = p.y - center.y;
        return ImVec2(
            center.x + dx * cos_roll - dy * sin_roll,
            center.y + dx * sin_roll + dy * cos_roll
        );
    };

    // Sky (above horizon)
    ImVec2 sky_corners[4] = {
        rotate_point(ImVec2(center.x - radius * 2, center.y - radius * 2)),
        rotate_point(ImVec2(center.x + radius * 2, center.y - radius * 2)),
        rotate_point(horizon_end),
        rotate_point(horizon_start)
    };
    draw_list->AddConvexPolyFilled(sky_corners, 4, IM_COL32(0, 120, 200, 255));

    // Ground (below horizon)
    ImVec2 ground_corners[4] = {
        rotate_point(horizon_start),
        rotate_point(horizon_end),
        rotate_point(ImVec2(center.x + radius * 2, center.y + radius * 2)),
        rotate_point(ImVec2(center.x - radius * 2, center.y + radius * 2))
    };
    draw_list->AddConvexPolyFilled(ground_corners, 4, IM_COL32(120, 80, 40, 255));

    // Draw horizon line (white)
    draw_list->AddLine(
        rotate_point(horizon_start),
        rotate_point(horizon_end),
        IM_COL32(255, 255, 255, 255),
        3.0f
    );

    // Draw pitch ladder
    for (int pitch_line = -30; pitch_line <= 30; pitch_line += 10) {
        if (pitch_line == 0) continue; // Skip horizon line

        float line_y = center.y + pitch_offset + pitch_line * pitch_pixels_per_deg;
        float line_half_width = (pitch_line % 20 == 0) ? 40.0f : 25.0f;

        ImVec2 left = rotate_point(ImVec2(center.x - line_half_width, line_y));
        ImVec2 right = rotate_point(ImVec2(center.x + line_half_width, line_y));

        draw_list->AddLine(left, right, IM_COL32(255, 255, 255, 200), 2.0f);
    }

    draw_list->PopClipRect();

    // Draw outer circle border
    draw_list->AddCircle(center, radius, IM_COL32(255, 255, 255, 255), 64, 2.0f);

    // Draw aircraft symbol (fixed in center)
    float wing_width = 35.0f;
    float wing_height = 3.0f;
    draw_list->AddRectFilled(
        ImVec2(center.x - wing_width, center.y - wing_height),
        ImVec2(center.x + wing_width, center.y + wing_height),
        IM_COL32(255, 255, 0, 255)
    );
    draw_list->AddCircleFilled(center, 4.0f, IM_COL32(255, 255, 0, 255));

    // Draw roll indicator arc at top
    for (int angle = -60; angle <= 60; angle += 10) {
        float a = (angle - 90) * 3.14159f / 180.0f;
        float tick_length = (angle % 30 == 0) ? 15.0f : 10.0f;
        ImVec2 outer = ImVec2(center.x + std::cos(a) * (radius + 5), center.y + std::sin(a) * (radius + 5));
        ImVec2 inner = ImVec2(center.x + std::cos(a) * (radius + 5 + tick_length), center.y + std::sin(a) * (radius + 5 + tick_length));
        draw_list->AddLine(outer, inner, IM_COL32(255, 255, 255, 255), 2.0f);
    }

    // Draw roll pointer (triangle pointing to current roll)
    float roll_angle = (-roll_deg - 90) * 3.14159f / 180.0f;
    ImVec2 roll_ptr = ImVec2(center.x + std::cos(roll_angle) * (radius + 20), center.y + std::sin(roll_angle) * (radius + 20));
    ImVec2 tri[3] = {
        roll_ptr,
        ImVec2(roll_ptr.x - 6, roll_ptr.y - 8),
        ImVec2(roll_ptr.x + 6, roll_ptr.y - 8)
    };
    draw_list->AddTriangleFilled(tri[0], tri[1], tri[2], IM_COL32(255, 255, 0, 255));
}

// Helper to draw BUSS (Backup Speed Scale) - Airbus style vertical tape
static void DrawBUSS(ImVec2 pos, ImVec2 size, const BUSSData& buss, float current_pitch, float current_thrust) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(10, 10, 15, 240));

    // Draw vertical pitch tape with colored zones
    float pitch_pixels_per_deg = size.y / 30.0f;  // Display -5 to +25 degrees
    float center_y = pos.y + size.y * 0.6f;  // Pitch reference point

    // BUSS zones (Airbus style - stacked colored rectangles)
    // Red zone (too low pitch)
    float red_low_top = center_y + (5.0f - buss.target_pitch_min) * pitch_pixels_per_deg;
    draw_list->AddRectFilled(
        ImVec2(pos.x + 5, red_low_top),
        ImVec2(pos.x + size.x - 5, pos.y + size.y - 5),
        IM_COL32(180, 0, 0, 100)
    );

    // Green zone (safe pitch range)
    float green_top = center_y + (5.0f - buss.target_pitch_max) * pitch_pixels_per_deg;
    float green_bottom = center_y + (5.0f - buss.target_pitch_min) * pitch_pixels_per_deg;
    draw_list->AddRectFilled(
        ImVec2(pos.x + 5, green_top),
        ImVec2(pos.x + size.x - 5, green_bottom),
        IM_COL32(0, 150, 0, 120)
    );

    // Amber zone (too high pitch)
    float amber_top = pos.y + 5;
    draw_list->AddRectFilled(
        ImVec2(pos.x + 5, amber_top),
        ImVec2(pos.x + size.x - 5, green_top),
        IM_COL32(200, 120, 0, 100)
    );

    // Draw pitch scale marks
    for (int pitch = 0; pitch <= 20; pitch += 5) {
        float y = center_y + (5.0f - pitch) * pitch_pixels_per_deg;
        if (y >= pos.y && y <= pos.y + size.y) {
            draw_list->AddLine(ImVec2(pos.x + 5, y), ImVec2(pos.x + 15, y), AirbusColors::WHITE, 1.0f);
            char pitch_label[8];
            snprintf(pitch_label, sizeof(pitch_label), "%d°", pitch);
            draw_list->AddText(ImVec2(pos.x + 18, y - 7), AirbusColors::WHITE, pitch_label);
        }
    }

    // Current pitch indicator (large triangle)
    float current_y = center_y + (5.0f - current_pitch) * pitch_pixels_per_deg;
    current_y = clampf(current_y, pos.y + 10, pos.y + size.y - 10);

    ImVec2 tri[3] = {
        ImVec2(pos.x + size.x - 5, current_y),
        ImVec2(pos.x + size.x - 15, current_y - 6),
        ImVec2(pos.x + size.x - 15, current_y + 6)
    };
    ImU32 pitch_color = (buss.pitch_too_low || buss.pitch_too_high) ? AirbusColors::AMBER : AirbusColors::GREEN;
    draw_list->AddTriangleFilled(tri[0], tri[1], tri[2], pitch_color);
    draw_list->AddTriangle(tri[0], tri[1], tri[2], AirbusColors::WHITE, 2.0f);

    // Thrust bar on the side (vertical)
    float thrust_bar_x = pos.x + size.x + 5;
    float thrust_bar_height = size.y - 40;
    float thrust_bar_y = pos.y + 20;

    // Thrust background
    draw_list->AddRectFilled(
        ImVec2(thrust_bar_x, thrust_bar_y),
        ImVec2(thrust_bar_x + 15, thrust_bar_y + thrust_bar_height),
        IM_COL32(30, 30, 30, 200)
    );
    draw_list->AddRect(
        ImVec2(thrust_bar_x, thrust_bar_y),
        ImVec2(thrust_bar_x + 15, thrust_bar_y + thrust_bar_height),
        AirbusColors::WHITE, 0.0f, 0, 1.0f
    );

    // Thrust target zone (green)
    float thrust_range = 1.0f;
    float thrust_min_y = thrust_bar_y + thrust_bar_height * (1.0f - buss.target_thrust_max / thrust_range);
    float thrust_max_y = thrust_bar_y + thrust_bar_height * (1.0f - buss.target_thrust_min / thrust_range);
    draw_list->AddRectFilled(
        ImVec2(thrust_bar_x + 1, thrust_min_y),
        ImVec2(thrust_bar_x + 14, thrust_max_y),
        IM_COL32(0, 150, 0, 150)
    );

    // Current thrust indicator
    float current_thrust_y = thrust_bar_y + thrust_bar_height * (1.0f - current_thrust);
    draw_list->AddLine(
        ImVec2(thrust_bar_x - 3, current_thrust_y),
        ImVec2(thrust_bar_x + 18, current_thrust_y),
        pitch_color, 3.0f
    );

    // Labels
    draw_list->AddText(ImVec2(pos.x + 5, pos.y + 5), AirbusColors::AMBER, "BUSS");
    char thrust_label[16];
    snprintf(thrust_label, sizeof(thrust_label), "THR");
    draw_list->AddText(ImVec2(thrust_bar_x - 5, thrust_bar_y - 15), AirbusColors::CYAN, thrust_label);

    // Border
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), AirbusColors::AMBER, 0.0f, 0, 2.0f);
}

// Helper to draw speed tape with V-speeds and colored bands
static void DrawSpeedTape(ImVec2 pos, ImVec2 size, float speed_knots, const BUSSData& buss, float current_pitch, float current_thrust, const VSpeeds& vspeeds) {
    // If BUSS is active, show BUSS instead of normal speed tape
    if (buss.active) {
        DrawBUSS(pos, size, buss, current_pitch, current_thrust);
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(20, 20, 30, 230));

    float center_y = pos.y + size.y * 0.5f;
    float pixels_per_knot = 2.0f;

    // ========== Draw colored speed bands (Airbus style) ==========
    // Red band: Below stall speed (below VLS - 5)
    float stall_speed = vspeeds.vls - 5.0f;
    float red_zone_top = center_y + (speed_knots - stall_speed) * pixels_per_knot;
    if (red_zone_top < pos.y + size.y) {
        draw_list->AddRectFilled(
            ImVec2(pos.x + 2, clampf(red_zone_top, pos.y, pos.y + size.y)),
            ImVec2(pos.x + size.x - 2, pos.y + size.y),
            IM_COL32(180, 0, 0, 100)
        );
        // Red/black stripes
        for (float y = clampf(red_zone_top, pos.y, pos.y + size.y); y < pos.y + size.y; y += 8.0f) {
            draw_list->AddRectFilled(
                ImVec2(pos.x + 2, y),
                ImVec2(pos.x + size.x - 2, clampf(y + 4.0f, pos.y, pos.y + size.y)),
                IM_COL32(0, 0, 0, 150)
            );
        }
    }

    // Amber band: Low speed awareness (VLS to VLS + 10)
    float amber_zone_bottom = center_y + (speed_knots - vspeeds.vls) * pixels_per_knot;
    float amber_zone_top = center_y + (speed_knots - (vspeeds.vls + 10.0f)) * pixels_per_knot;
    draw_list->AddRectFilled(
        ImVec2(pos.x + 2, clampf(amber_zone_top, pos.y, pos.y + size.y)),
        ImVec2(pos.x + size.x - 2, clampf(amber_zone_bottom, pos.y, pos.y + size.y)),
        IM_COL32(200, 120, 0, 80)
    );

    // Green band: Normal operating range (VLS + 10 to VMAX - 6)
    float green_zone_bottom = amber_zone_top;
    float green_zone_top = center_y + (speed_knots - (vspeeds.vmax - 6.0f)) * pixels_per_knot;
    draw_list->AddRectFilled(
        ImVec2(pos.x + 2, clampf(green_zone_top, pos.y, pos.y + size.y)),
        ImVec2(pos.x + size.x - 2, clampf(green_zone_bottom, pos.y, pos.y + size.y)),
        IM_COL32(0, 150, 0, 60)
    );

    // Red/black barber pole: Overspeed (above VMAX)
    float overspeed_zone_bottom = center_y + (speed_knots - vspeeds.vmax) * pixels_per_knot;
    if (overspeed_zone_bottom > pos.y) {
        for (float y = pos.y; y < clampf(overspeed_zone_bottom, pos.y, pos.y + size.y); y += 8.0f) {
            draw_list->AddRectFilled(
                ImVec2(pos.x + 2, y),
                ImVec2(pos.x + size.x - 2, clampf(y + 4.0f, pos.y, pos.y + size.y)),
                IM_COL32(180, 0, 0, 120)
            );
        }
    }

    // Draw speed markings
    float center_y_copy = center_y;
    float pixels_per_knot_copy = pixels_per_knot;

    for (int spd = 0; spd <= 400; spd += 10) {
        float offset_y = center_y + (speed_knots - spd) * pixels_per_knot;
        if (offset_y < pos.y || offset_y > pos.y + size.y) continue;

        bool major = (spd % 20 == 0);
        float tick_len = major ? 15.0f : 8.0f;

        draw_list->AddLine(
            ImVec2(pos.x + size.x - tick_len, offset_y),
            ImVec2(pos.x + size.x, offset_y),
            IM_COL32(255, 255, 255, 255),
            1.5f
        );

        if (major && spd > 0) {
            char label[8];
            snprintf(label, sizeof(label), "%d", spd);
            draw_list->AddText(ImVec2(pos.x + 5, offset_y - 7), IM_COL32(255, 255, 255, 255), label);
        }
    }

    // Calculate speed trend (acceleration/deceleration)
    static float prev_speed = speed_knots;
    static float trend_filter = 0.0f;
    float dt = ImGui::GetIO().DeltaTime;

    if (dt > 0.0f) {
        float speed_change_rate = (speed_knots - prev_speed) / dt;  // knots/sec
        // Low-pass filter for smoother trend
        trend_filter = trend_filter * 0.9f + speed_change_rate * 0.1f;
        prev_speed = speed_knots;
    }

    // Speed trend arrow (shows predicted speed in 10 seconds)
    float trend_prediction = trend_filter * 10.0f;  // Speed change in 10 seconds
    if (std::abs(trend_prediction) > 2.0f) {  // Only show if significant trend
        float trend_y = center_y - trend_prediction * pixels_per_knot;
        trend_y = clampf(trend_y, pos.y + 20, pos.y + size.y - 20);

        // Draw trend arrow (triangle)
        ImVec2 arrow_tip = ImVec2(pos.x + size.x - 5, trend_y);
        ImVec2 arrow_base_top = ImVec2(pos.x + size.x - 15, trend_y - 8);
        ImVec2 arrow_base_bot = ImVec2(pos.x + size.x - 15, trend_y + 8);

        ImU32 trend_color = (trend_prediction > 0) ? AirbusColors::MAGENTA : AirbusColors::AMBER;
        draw_list->AddTriangleFilled(arrow_tip, arrow_base_top, arrow_base_bot, trend_color);

        // Draw trend line from current speed to trend arrow
        draw_list->AddLine(
            ImVec2(pos.x + size.x - 5, center_y),
            ImVec2(pos.x + size.x - 5, trend_y),
            trend_color,
            2.0f
        );
    }

    // ========== Draw V-speed bugs (Airbus style) ==========
    // Helper lambda to draw a speed bug
    auto drawSpeedBug = [&](float speed, ImU32 color, const char* label, bool is_dot = false) {
        if (speed <= 0.0f) return;  // Skip if not set
        float bug_y = center_y + (speed_knots - speed) * pixels_per_knot;
        if (bug_y < pos.y || bug_y > pos.y + size.y) return;  // Off screen

        if (is_dot) {
            // Green dot - small circle
            draw_list->AddCircleFilled(ImVec2(pos.x - 3, bug_y), 4.0f, color);
            draw_list->AddCircle(ImVec2(pos.x - 3, bug_y), 4.0f, AirbusColors::WHITE, 0, 1.5f);
        } else {
            // Standard bug - triangle or line with text
            draw_list->AddLine(ImVec2(pos.x - 8, bug_y), ImVec2(pos.x + 2, bug_y), color, 2.5f);
            if (label && strlen(label) > 0) {
                draw_list->AddText(ImVec2(pos.x - 18, bug_y - 7), color, label);
            }
        }
    };

    // VLS (Lowest Selectable) - Amber = symbol
    if (vspeeds.vls > 0.0f) {
        float vls_y = center_y + (speed_knots - vspeeds.vls) * pixels_per_knot;
        if (vls_y >= pos.y && vls_y <= pos.y + size.y) {
            draw_list->AddLine(ImVec2(pos.x - 8, vls_y - 3), ImVec2(pos.x + 2, vls_y - 3), AirbusColors::AMBER, 2.0f);
            draw_list->AddLine(ImVec2(pos.x - 8, vls_y + 3), ImVec2(pos.x + 2, vls_y + 3), AirbusColors::AMBER, 2.0f);
        }
    }

    // Green dot (best L/D)
    drawSpeedBug(vspeeds.green_dot, AirbusColors::GREEN, "", true);

    // V1 - Cyan circle with "1"
    if (vspeeds.v1 > 0.0f && vspeeds.display_takeoff_speeds) {
        float v1_y = center_y + (speed_knots - vspeeds.v1) * pixels_per_knot;
        if (v1_y >= pos.y && v1_y <= pos.y + size.y) {
            draw_list->AddCircleFilled(ImVec2(pos.x - 3, v1_y), 6.0f, AirbusColors::CYAN);
            draw_list->AddCircle(ImVec2(pos.x - 3, v1_y), 6.0f, AirbusColors::WHITE, 0, 1.5f);
            draw_list->AddText(ImVec2(pos.x - 18, v1_y - 7), AirbusColors::CYAN, "1");
        }
    }

    // VR - Cyan dot
    if (vspeeds.vr > 0.0f && vspeeds.display_takeoff_speeds) {
        drawSpeedBug(vspeeds.vr, AirbusColors::CYAN, "");
    }

    // V2 - Magenta "2"
    if (vspeeds.v2 > 0.0f && vspeeds.display_takeoff_speeds) {
        drawSpeedBug(vspeeds.v2, AirbusColors::MAGENTA, "2");
    }

    // VAPP - Magenta triangle
    if (vspeeds.vapp > 0.0f && vspeeds.display_approach_speeds) {
        float vapp_y = center_y + (speed_knots - vspeeds.vapp) * pixels_per_knot;
        if (vapp_y >= pos.y && vapp_y <= pos.y + size.y) {
            ImVec2 tri[3] = {
                ImVec2(pos.x - 10, vapp_y),
                ImVec2(pos.x - 2, vapp_y - 5),
                ImVec2(pos.x - 2, vapp_y + 5)
            };
            draw_list->AddTriangleFilled(tri[0], tri[1], tri[2], AirbusColors::MAGENTA);
        }
    }

    // Border
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

    // Current speed box
    ImVec2 box_pos = ImVec2(pos.x + size.x + 5, center_y - 15);
    ImVec2 box_size = ImVec2(60, 30);
    ImU32 box_color = AirbusColors::GREEN;
    if (speed_knots > 330.0f) box_color = AirbusColors::RED;
    else if (speed_knots < 140.0f) box_color = AirbusColors::AMBER;

    draw_list->AddRectFilled(box_pos, ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y), IM_COL32(0, 0, 0, 255));
    draw_list->AddRect(box_pos, ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y), box_color, 0.0f, 0, 2.0f);

    char speed_text[8];
    snprintf(speed_text, sizeof(speed_text), "%03.0f", speed_knots);
    ImVec2 text_size = ImGui::CalcTextSize(speed_text);
    draw_list->AddText(
        ImVec2(box_pos.x + (box_size.x - text_size.x) * 0.5f, box_pos.y + (box_size.y - text_size.y) * 0.5f),
        box_color,
        speed_text
    );
}

// Helper to draw altitude tape
static void DrawAltitudeTape(ImVec2 pos, ImVec2 size, float altitude_ft, float vs_fpm) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(20, 20, 30, 230));
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

    // Draw altitude markings
    float center_y = pos.y + size.y * 0.5f;
    float pixels_per_foot = 0.1f;

    for (int alt = 0; alt <= 50000; alt += 100) {
        float offset_y = center_y + (altitude_ft - alt) * pixels_per_foot;
        if (offset_y < pos.y || offset_y > pos.y + size.y) continue;

        bool major = (alt % 500 == 0);
        float tick_len = major ? 15.0f : 8.0f;

        draw_list->AddLine(
            ImVec2(pos.x, offset_y),
            ImVec2(pos.x + tick_len, offset_y),
            IM_COL32(255, 255, 255, 255),
            1.5f
        );

        if (major) {
            char label[16];
            snprintf(label, sizeof(label), "%d", alt / 100);
            ImVec2 text_size = ImGui::CalcTextSize(label);
            draw_list->AddText(ImVec2(pos.x + size.x - text_size.x - 5, offset_y - 7), IM_COL32(255, 255, 255, 255), label);
        }
    }

    // Current altitude box
    ImVec2 box_pos = ImVec2(pos.x - 70, center_y - 15);
    ImVec2 box_size = ImVec2(65, 30);

    draw_list->AddRectFilled(box_pos, ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y), IM_COL32(0, 0, 0, 255));
    draw_list->AddRect(box_pos, ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y), AirbusColors::GREEN, 0.0f, 0, 2.0f);

    char alt_text[16];
    snprintf(alt_text, sizeof(alt_text), "%05.0f", altitude_ft);
    ImVec2 text_size = ImGui::CalcTextSize(alt_text);
    draw_list->AddText(
        ImVec2(box_pos.x + (box_size.x - text_size.x) * 0.5f, box_pos.y + (box_size.y - text_size.y) * 0.5f),
        AirbusColors::GREEN,
        alt_text
    );

    // VS indicator
    ImVec2 vs_box_pos = ImVec2(box_pos.x, box_pos.y + box_size.y + 5);
    char vs_text[16];
    snprintf(vs_text, sizeof(vs_text), "%+05.0f", vs_fpm);
    ImU32 vs_color = (std::abs(vs_fpm) > 2000.0f) ? AirbusColors::AMBER : AirbusColors::CYAN;
    draw_list->AddText(vs_box_pos, vs_color, vs_text);
}

// ================================
// Primary Flight Display Elements
// ================================
void DrawPFDPanel(const Sensors& sensors, const PrimCore& prim, const PilotInput& pilot, const AutopilotState& ap, const Faults& faults) {
    ImGui::SetNextWindowPos(ImVec2(370, 10), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(420, 420), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("PRIMARY FLIGHT DISPLAY", nullptr);

    const auto& fctl = prim.fctl_status();
    const auto& gpws = prim.gpws_callouts();

    // Check for electrical failures that would disable PFD
    bool pfd_unreliable = faults.total_electrical_fail || faults.partial_electrical_fail;

    // FMA (Flight Mode Annunciator) - top line with automation status
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 255));
    ImGui::BeginChild("FMA", ImVec2(0, 55), true);

    // First row: Thrust / Vertical / Lateral modes (like real A320 FMA)
    ImGui::SetCursorPosY(5);

    // Column 1: Thrust mode
    ImGui::SetCursorPosX(10);
    if (ap.autothrust && ap.spd_mode) {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "SPD");
    } else if (ap.autothrust) {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "A/THR");
    } else {
        ImGui::TextColored(ImColor(AirbusColors::WHITE), "MAN THR");
    }

    // Column 2: Vertical mode
    ImGui::SameLine(140);
    if (ap.alt_mode) {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "ALT");
    } else if (ap.vs_mode) {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "V/S");
    } else {
        ImGui::TextColored(ImColor(IM_COL32(100,100,100,255)), "---");
    }

    // Column 3: Lateral mode
    ImGui::SameLine(240);
    if (ap.hdg_mode) {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "HDG");
    } else {
        ImGui::TextColored(ImColor(IM_COL32(100,100,100,255)), "---");
    }

    // Second row: Control law and protections
    ImGui::SetCursorPosY(28);
    const char* law_text = (fctl.law == ControlLaw::NORMAL) ? "NORMAL" :
                           (fctl.law == ControlLaw::ALTERNATE) ? "ALT LAW" : "DIRECT";
    ImU32 law_color = (fctl.law == ControlLaw::NORMAL) ? AirbusColors::GREEN : AirbusColors::AMBER;

    ImGui::PushStyleColor(ImGuiCol_Text, law_color);
    ImGui::SetCursorPosX(10);
    ImGui::Text("%s", law_text);
    ImGui::PopStyleColor();

    ImGui::SameLine(240);
    if (fctl.alpha_prot) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "A.PROT");
    } else if (fctl.alpha_floor) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "A.FLOOR");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Main PFD display area
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = 380.0f; // Fixed height for PFD

    ImGui::InvisibleButton("canvas", canvas_size);

    // If electrical failure, show red unreliable message instead of normal display
    if (pfd_unreliable) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Draw red background
        draw_list->AddRectFilled(canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(100, 0, 0, 200));

        // Draw large "UNRELIABLE" message
        ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

        const char* msg = "UNRELIABLE";
        ImFont* font = ImGui::GetFont();
        float prev_scale = font->Scale;
        font->Scale = 3.0f;
        ImGui::PushFont(font);

        ImVec2 text_size = ImGui::CalcTextSize(msg);
        ImVec2 text_pos = ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f);

        draw_list->AddText(text_pos, AirbusColors::RED, msg);

        font->Scale = prev_scale;
        ImGui::PopFont();

        // Draw warning message below
        const char* warn_msg = "ELEC FAULT - USE STANDBY INSTRUMENTS";
        draw_list->AddText(
            ImVec2(center.x - ImGui::CalcTextSize(warn_msg).x * 0.5f, center.y + 50),
            AirbusColors::AMBER, warn_msg);

        ImGui::Spacing();
        ImGui::End();
        ImGui::PopStyleColor();
        return;
    }

    // Draw artificial horizon in center
    ImVec2 horizon_center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f - 20);
    DrawArtificialHorizon(horizon_center, 100.0f, sensors.pitch_deg, sensors.roll_deg);

    // ========== Draw Flight Path Vector (FPV) - Green circle showing actual flight path ==========
    if (sensors.ias_knots > 60.0f) {  // Only show FPV above 60 knots
        // Calculate Flight Path Angle (FPA) from vertical speed and groundspeed
        float groundspeed_fps = sensors.ias_knots * 101.269f / 60.0f;  // Convert knots to ft/sec (approximation)
        float vs_fps = sensors.vs_fpm / 60.0f;  // Convert ft/min to ft/sec
        float fpa_deg = atan2f(vs_fps, groundspeed_fps) * 57.2958f;  // Flight path angle in degrees

        // FPV position relative to horizon center (offset by difference between pitch and FPA)
        float pixels_per_deg = 100.0f / 15.0f;  // Horizon radius / 15 degrees
        float fpv_offset_y = (sensors.pitch_deg - fpa_deg) * pixels_per_deg;

        ImVec2 fpv_center = ImVec2(horizon_center.x, horizon_center.y + fpv_offset_y);

        // Only draw if on-screen
        if (fpv_center.y >= canvas_pos.y && fpv_center.y <= canvas_pos.y + canvas_size.y) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Draw FPV symbol - green circle with horizontal wings
            draw_list->AddCircle(fpv_center, 8.0f, AirbusColors::GREEN, 16, 2.0f);

            // Horizontal wings (left and right)
            draw_list->AddLine(
                ImVec2(fpv_center.x - 18, fpv_center.y),
                ImVec2(fpv_center.x - 9, fpv_center.y),
                AirbusColors::GREEN, 2.0f
            );
            draw_list->AddLine(
                ImVec2(fpv_center.x + 9, fpv_center.y),
                ImVec2(fpv_center.x + 18, fpv_center.y),
                AirbusColors::GREEN, 2.0f
            );

            // Vertical line at bottom (optional - some Airbus variants have this)
            draw_list->AddLine(
                ImVec2(fpv_center.x, fpv_center.y + 8),
                ImVec2(fpv_center.x, fpv_center.y + 12),
                AirbusColors::GREEN, 2.0f
            );
        }
    }

    // Draw speed tape on left (or BUSS if airspeed unreliable)
    ImVec2 speed_tape_pos = ImVec2(canvas_pos.x + 10, canvas_pos.y + 40);
    DrawSpeedTape(speed_tape_pos, ImVec2(50, 300), sensors.ias_knots, prim.buss_data(), sensors.pitch_deg, pilot.thrust, prim.vspeeds());

    // Draw altitude tape on right
    ImVec2 alt_tape_pos = ImVec2(canvas_pos.x + canvas_size.x - 60, canvas_pos.y + 40);
    DrawAltitudeTape(alt_tape_pos, ImVec2(50, 300), sensors.altitude_ft, sensors.vs_fpm);

    // Draw AoA indicator (top right)
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 aoa_pos = ImVec2(canvas_pos.x + canvas_size.x - 150, canvas_pos.y + 10);
    char aoa_text[32];
    snprintf(aoa_text, sizeof(aoa_text), "AoA: %.1f°", sensors.aoa_deg);
    ImU32 aoa_color = (sensors.aoa_deg > 12.0f) ? AirbusColors::RED :
                      (sensors.aoa_deg > 8.0f) ? AirbusColors::AMBER : AirbusColors::GREEN;
    draw_list->AddText(aoa_pos, aoa_color, aoa_text);

    // Heading (top center)
    ImVec2 hdg_pos = ImVec2(canvas_pos.x + canvas_size.x * 0.5f - 30, canvas_pos.y + 10);
    char hdg_text[32];
    snprintf(hdg_text, sizeof(hdg_text), "HDG %03.0f°", sensors.heading_deg);
    draw_list->AddText(hdg_pos, AirbusColors::CYAN, hdg_text);

    // Mach number (bottom center)
    ImVec2 mach_pos = ImVec2(canvas_pos.x + canvas_size.x * 0.5f - 40, canvas_pos.y + canvas_size.y - 30);
    char mach_text[32];
    snprintf(mach_text, sizeof(mach_text), "M %.3f", sensors.mach);
    draw_list->AddText(mach_pos, AirbusColors::CYAN, mach_text);

    // GPWS Callouts (center of display, very prominent)
    if (!gpws.current_callout.empty()) {
        ImVec2 callout_pos = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.35f);

        // Determine color based on callout type
        ImU32 callout_color = AirbusColors::GREEN;
        float font_scale = 1.5f;

        if (gpws.current_callout == "PULL UP") {
            callout_color = AirbusColors::RED;
            font_scale = 2.5f;
            // Flashing effect
            static float blink_timer = 0.0f;
            blink_timer += ImGui::GetIO().DeltaTime;
            if (fmod(blink_timer, 0.5f) < 0.25f) {
                callout_color = IM_COL32(0, 0, 0, 0);  // Flash off
            }
        } else if (gpws.current_callout == "WINDSHEAR") {
            callout_color = AirbusColors::RED;
            font_scale = 2.0f;
        } else if (gpws.current_callout == "RETARD") {
            callout_color = AirbusColors::AMBER;
            font_scale = 2.0f;
        } else {
            // Altitude callouts
            callout_color = AirbusColors::GREEN;
            font_scale = 1.8f;
        }

        // Draw callout with larger font
        ImFont* font = ImGui::GetFont();
        float prev_scale = font->Scale;
        font->Scale = font_scale;
        ImGui::PushFont(font);

        ImVec2 text_size = ImGui::CalcTextSize(gpws.current_callout.c_str());
        ImVec2 text_pos = ImVec2(callout_pos.x - text_size.x * 0.5f, callout_pos.y - text_size.y * 0.5f);

        // Draw background box for better visibility
        ImVec2 box_padding = ImVec2(15, 10);
        draw_list->AddRectFilled(
            ImVec2(text_pos.x - box_padding.x, text_pos.y - box_padding.y),
            ImVec2(text_pos.x + text_size.x + box_padding.x, text_pos.y + text_size.y + box_padding.y),
            IM_COL32(0, 0, 0, 200)
        );

        draw_list->AddText(text_pos, callout_color, gpws.current_callout.c_str());

        font->Scale = prev_scale;
        ImGui::PopFont();
    }

    ImGui::Spacing();

    // Thrust display at bottom
    ImGui::BeginChild("Thrust", ImVec2(0, 0), true);
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "THRUST:");
    ImGui::SameLine();

    const char* thrust_label;
    ImU32 thrust_color;
    if (pilot.thrust < 0.2f) {
        thrust_label = "IDLE";
        thrust_color = AirbusColors::AMBER;
    } else if (pilot.thrust < 0.4f) {
        thrust_label = "LOW";
        thrust_color = AirbusColors::WHITE;
    } else if (pilot.thrust < 0.7f) {
        thrust_label = "CLIMB";
        thrust_color = AirbusColors::GREEN;
    } else if (pilot.thrust < 0.9f) {
        thrust_label = "MAX";
        thrust_color = AirbusColors::GREEN;
    } else {
        thrust_label = "TOGA";
        thrust_color = AirbusColors::MAGENTA;
    }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, thrust_color);
    ImGui::ProgressBar(pilot.thrust, ImVec2(-1, 20), "");
    ImGui::PopStyleColor();

    ImGui::SameLine();
    char thrust_text[16];
    snprintf(thrust_text, sizeof(thrust_text), "%s", thrust_label);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 80);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
    ImGui::TextColored(ImColor(AirbusColors::WHITE), "%s", thrust_text);

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
}

// ================================
// F/CTL - Flight Control System Display
// ================================
void DrawFctlPanel(const PrimCore& prim, Faults& faults) {
    ImGui::SetNextWindowPos(ImVec2(370, 440), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(420, 150), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("F/CTL STATUS", nullptr);

    const auto& fctl = prim.fctl_status();

    // Control law
    const char* law_name = (fctl.law == ControlLaw::NORMAL) ? "NORMAL LAW" :
                           (fctl.law == ControlLaw::ALTERNATE) ? "ALTERNATE LAW" : "DIRECT LAW";
    ImU32 law_color = (fctl.law == ControlLaw::NORMAL) ? AirbusColors::GREEN : AirbusColors::AMBER;
    TextCentered(law_name, law_color);

    ImGui::Separator();

    // Computers status (compact)
    ImGui::Columns(3, nullptr, false);
    ImGui::TextColored(ImColor(fctl.elac1_avail ? AirbusColors::GREEN : AirbusColors::RED), "ELAC1");
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(fctl.elac2_avail ? AirbusColors::GREEN : AirbusColors::RED), "ELAC2");
    ImGui::NextColumn();
    ImGui::TextColored(ImColor(fctl.sec1_avail ? AirbusColors::GREEN : AirbusColors::RED), "SEC1");
    ImGui::Columns(1);

    ImGui::Separator();

    // Protections (compact)
    if (fctl.alpha_prot) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "ALPHA PROT");
    }
    if (fctl.alpha_floor) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "ALPHA FLOOR");
    }
    if (fctl.high_speed_prot) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "HIGH SPEED PROT");
    }
    if (!fctl.alpha_prot && !fctl.alpha_floor && !fctl.high_speed_prot) {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "PROTECTIONS NORMAL");
    }

    ImGui::Separator();

    // Surface deflections (compact)
    const auto& surf = prim.surfaces();
    ImGui::Text("ELEV: %+.1f°", surf.elevator_deg);
    ImGui::SameLine(150);
    if (faults.elevator_jam) ImGui::TextColored(ImColor(AirbusColors::RED), "JAM");

    ImGui::Text("AIL:  %+.1f°", surf.aileron_deg);
    ImGui::SameLine(150);
    if (faults.aileron_jam) ImGui::TextColored(ImColor(AirbusColors::RED), "JAM");

    ImGui::End();
    ImGui::PopStyleColor();
}

// ================================
// Control Input Panel (Simplified - Sensor Override Only)
// ================================
void DrawControlInputPanel(PilotInput& pilot, Sensors& sensors, Faults& faults, SimulationSettings& sim_settings, FlapsPosition& flaps) {
    ImGui::SetNextWindowPos(ImVec2(800, 10), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(280, 580), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("SENSOR OVERRIDE", nullptr);

    TextCentered("MANUAL SENSOR CONTROL", AirbusColors::AMBER);
    ImGui::Separator();
    ImGui::Spacing();

    // Manual sensor override toggle (for QF72-style scenarios)
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::AMBER);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, AirbusColors::RED);
    bool override_changed = ImGui::Checkbox("MANUAL SENSOR OVERRIDE", &sim_settings.manual_sensor_override);
    ImGui::PopStyleColor(2);

    ImGui::Spacing();

    if (sim_settings.manual_sensor_override) {
        ImGui::TextColored(ImColor(AirbusColors::RED), "WARNING: Physics disabled!");
        ImGui::TextColored(ImColor(IM_COL32(180,180,180,255)), "You can inject false sensor data");
        ImGui::TextColored(ImColor(IM_COL32(180,180,180,255)), "for QF72-style scenarios.");
    } else {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "Physics simulation active");
        ImGui::TextColored(ImColor(IM_COL32(180,180,180,255)), "Sensors respond to control inputs");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Flight parameters
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "FLIGHT PARAMETERS:");

    if (sim_settings.manual_sensor_override) {
        // Manual mode: Show editable sliders
        ImGui::PushItemWidth(220);
        ImGui::SliderFloat("IAS (kt)", &sensors.ias_knots, 60.0f, 380.0f, "%.0f");
        ImGui::SliderFloat("Altitude (ft)", &sensors.altitude_ft, 0.0f, 45000.0f, "%.0f");
        ImGui::SliderFloat("V/S (fpm)", &sensors.vs_fpm, -6000.0f, 6000.0f, "%.0f");
        ImGui::SliderFloat("AoA (deg)", &sensors.aoa_deg, -5.0f, 25.0f, "%.1f");
        ImGui::SliderFloat("Heading (deg)", &sensors.heading_deg, 0.0f, 359.0f, "%.0f");
        ImGui::SliderFloat("Mach", &sensors.mach, 0.0f, 0.85f, "%.3f");
        ImGui::SliderFloat("Pitch (deg)", &sensors.pitch_deg, -30.0f, 30.0f, "%.1f");
        ImGui::SliderFloat("Roll (deg)", &sensors.roll_deg, -60.0f, 60.0f, "%.1f");
        ImGui::PopItemWidth();
    } else {
        // Physics mode: Show read-only displays
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(100, 100, 100, 255));
        ImGui::BeginDisabled();

        ImGui::PushItemWidth(220);
        ImGui::SliderFloat("IAS (kt)", &sensors.ias_knots, 60.0f, 380.0f, "%.0f");
        ImGui::SliderFloat("Altitude (ft)", &sensors.altitude_ft, 0.0f, 45000.0f, "%.0f");
        ImGui::SliderFloat("V/S (fpm)", &sensors.vs_fpm, -6000.0f, 6000.0f, "%.0f");
        ImGui::SliderFloat("AoA (deg)", &sensors.aoa_deg, -5.0f, 25.0f, "%.1f");
        ImGui::SliderFloat("Heading (deg)", &sensors.heading_deg, 0.0f, 359.0f, "%.0f");
        ImGui::SliderFloat("Mach", &sensors.mach, 0.0f, 0.85f, "%.3f");
        ImGui::SliderFloat("Pitch (deg)", &sensors.pitch_deg, -30.0f, 30.0f, "%.1f");
        ImGui::SliderFloat("Roll (deg)", &sensors.roll_deg, -60.0f, 60.0f, "%.1f");
        ImGui::PopItemWidth();

        ImGui::EndDisabled();
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        ImGui::TextColored(ImColor(IM_COL32(150,150,150,255)), "(Computed from flight dynamics)");
    }

    ImGui::End();
    ImGui::PopStyleColor();
}

// ================================
// Autopilot Control Panel (FCU-style)
// ================================
void DrawAutopilotPanel(AutopilotState& ap, const Sensors& sensors) {
    ImGui::SetNextWindowPos(ImVec2(10, 600), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(600, 120), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("AUTOPILOT / FCU", nullptr);

    TextCentered("FCU", AirbusColors::CYAN);
    ImGui::Separator();

    // Layout: 4 columns for SPD, HDG, ALT, VS
    ImGui::Columns(4, nullptr, false);

    // ===== SPEED COLUMN =====
    ImGui::BeginChild("SPD_Section", ImVec2(0, 65), true);
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "SPD");

    // Mode button
    ImU32 spd_btn_color = ap.spd_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, spd_btn_color);
    if (ImGui::Button("##SPD", ImVec2(40, 20))) {
        ap.spd_mode = !ap.spd_mode;
        if (ap.spd_mode) {
            ap.target_spd_knots = sensors.ias_knots;
        }
    }
    ImGui::PopStyleColor();

    // Target value
    ImGui::PushItemWidth(100);
    if (ap.spd_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##spd_target", &ap.target_spd_knots, 1.0f, 100.0f, 350.0f, "%.0f kt");
        ImGui::PopStyleColor();
    } else {
        ImGui::BeginDisabled();
        ImGui::DragFloat("##spd_target", &ap.target_spd_knots, 1.0f, 100.0f, 350.0f, "%.0f kt");
        ImGui::EndDisabled();
    }
    ImGui::PopItemWidth();

    ImGui::EndChild();
    ImGui::NextColumn();

    // ===== HEADING COLUMN =====
    ImGui::BeginChild("HDG_Section", ImVec2(0, 65), true);
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "HDG");

    ImU32 hdg_btn_color = ap.hdg_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, hdg_btn_color);
    if (ImGui::Button("##HDG", ImVec2(40, 20))) {
        ap.hdg_mode = !ap.hdg_mode;
        if (ap.hdg_mode) {
            ap.target_hdg_deg = sensors.heading_deg;
        }
    }
    ImGui::PopStyleColor();

    ImGui::PushItemWidth(100);
    if (ap.hdg_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##hdg_target", &ap.target_hdg_deg, 1.0f, 0.0f, 359.0f, "%.0f°");
        ImGui::PopStyleColor();
    } else {
        ImGui::BeginDisabled();
        ImGui::DragFloat("##hdg_target", &ap.target_hdg_deg, 1.0f, 0.0f, 359.0f, "%.0f°");
        ImGui::EndDisabled();
    }
    ImGui::PopItemWidth();

    ImGui::EndChild();
    ImGui::NextColumn();

    // ===== ALTITUDE COLUMN =====
    ImGui::BeginChild("ALT_Section", ImVec2(0, 65), true);
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "ALT");

    ImU32 alt_btn_color = ap.alt_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, alt_btn_color);
    if (ImGui::Button("##ALT", ImVec2(40, 20))) {
        ap.alt_mode = !ap.alt_mode;
        if (ap.alt_mode) {
            ap.target_alt_ft = sensors.altitude_ft;
        }
    }
    ImGui::PopStyleColor();

    ImGui::PushItemWidth(100);
    if (ap.alt_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##alt_target", &ap.target_alt_ft, 100.0f, 0.0f, 45000.0f, "%.0f ft");
        ImGui::PopStyleColor();
    } else {
        ImGui::BeginDisabled();
        ImGui::DragFloat("##alt_target", &ap.target_alt_ft, 100.0f, 0.0f, 45000.0f, "%.0f ft");
        ImGui::EndDisabled();
    }
    ImGui::PopItemWidth();

    ImGui::EndChild();
    ImGui::NextColumn();

    // ===== VERTICAL SPEED COLUMN =====
    ImGui::BeginChild("VS_Section", ImVec2(0, 65), true);
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "V/S");

    ImU32 vs_btn_color = ap.vs_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, vs_btn_color);
    if (ImGui::Button("##VS", ImVec2(40, 20))) {
        ap.vs_mode = !ap.vs_mode;
        if (ap.vs_mode) {
            ap.target_vs_fpm = sensors.vs_fpm;
        }
    }
    ImGui::PopStyleColor();

    ImGui::PushItemWidth(100);
    if (ap.vs_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##vs_target", &ap.target_vs_fpm, 100.0f, -6000.0f, 6000.0f, "%+.0f");
        ImGui::PopStyleColor();
    } else {
        ImGui::BeginDisabled();
        ImGui::DragFloat("##vs_target", &ap.target_vs_fpm, 100.0f, -6000.0f, 6000.0f, "%+.0f");
        ImGui::EndDisabled();
    }
    ImGui::PopItemWidth();

    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::Columns(1);

    // Autothrust and Disconnect buttons
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 280) * 0.5f);

    ImU32 athr_color = ap.autothrust ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, athr_color);
    if (ImGui::Button("A/THR", ImVec2(70, 25))) {
        ap.autothrust = !ap.autothrust;
        if (ap.autothrust) {
            ap.spd_mode = true;
            ap.target_spd_knots = sensors.ias_knots;
        }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 50, 50, 255));
    if (ImGui::Button("AP DISCONNECT", ImVec2(150, 25))) {
        ap.spd_mode = false;
        ap.hdg_mode = false;
        ap.alt_mode = false;
        ap.vs_mode = false;
        ap.autothrust = false;
    }
    ImGui::PopStyleColor(2);

    ImGui::End();
    ImGui::PopStyleColor();
}
// ================================
// Sim Operation Panel (Weather + Faults)
// ================================
void DrawSimOperationPanel(Weather& weather, Faults& faults) {
    ImGui::SetNextWindowPos(ImVec2(1090, 10), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(350, 480), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("SIM OPERATION", nullptr);

    TextCentered("SIMULATION CONTROLS", AirbusColors::AMBER);
    ImGui::Separator();
    ImGui::Spacing();

    // WEATHER
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "WEATHER CONDITIONS");
    ImGui::PushItemWidth(250);
    ImGui::SliderFloat("Wind Speed", &weather.wind_speed_knots, 0.0f, 100.0f, "%.0f kt");
    ImGui::SliderFloat("Wind Direction", &weather.wind_direction_deg, 0.0f, 359.0f, "%.0f deg");
    ImGui::SliderFloat("Turbulence", &weather.turbulence_intensity, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Windshear", &weather.windshear_intensity, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // FAULT INJECTION - All faults in collapsible categories
    ImGui::TextColored(ImColor(AirbusColors::RED), "FAULT INJECTION");
    ImGui::Separator();

    // Sensor faults
    if (ImGui::CollapsingHeader("Sensors")) {
        ImGui::Checkbox("ADR 1 Failure", &faults.adr1_fail);
        ImGui::Checkbox("Overspeed Sensor Bad", &faults.overspeed_sensor_bad);
        ImGui::Checkbox("Pitot Blockage (Unreliable Airspeed → BUSS)", &faults.pitot_blocked);
    }

    // Flight control computer faults
    if (ImGui::CollapsingHeader("Flight Control Computers")) {
        ImGui::Checkbox("ELAC 1 Fault", &faults.elac1_fail);
        ImGui::Checkbox("ELAC 2 Fault", &faults.elac2_fail);
        ImGui::Checkbox("SEC 1 Fault", &faults.sec1_fail);
    }

    // Control surface faults
    if (ImGui::CollapsingHeader("Control Surfaces & Actuators")) {
        ImGui::Checkbox("Elevator Jam", &faults.elevator_jam);
        ImGui::Checkbox("Aileron Jam", &faults.aileron_jam);
        ImGui::Checkbox("Elevator Left Actuator Fail", &faults.elevator_left_actuator_fail);
        ImGui::Checkbox("Elevator Right Actuator Fail", &faults.elevator_right_actuator_fail);
        ImGui::Checkbox("Aileron Left Actuator Fail", &faults.aileron_left_actuator_fail);
        ImGui::Checkbox("Aileron Right Actuator Fail", &faults.aileron_right_actuator_fail);
    }

    // Engine failures
    if (ImGui::CollapsingHeader("Engine Failures")) {
        ImGui::Text("Engine 1:");
        ImGui::Checkbox("ENG 1 N1 Sensor Fail", &faults.eng1_n1_sensor_fail);
        ImGui::Checkbox("ENG 1 N2 Sensor Fail", &faults.eng1_n2_sensor_fail);
        ImGui::Checkbox("ENG 1 EGT Sensor Fail", &faults.eng1_egt_sensor_fail);
        ImGui::Checkbox("ENG 1 Vibration High", &faults.eng1_vibration_high);
        ImGui::Checkbox("ENG 1 Oil Pressure Low", &faults.eng1_oil_pressure_low);
        ImGui::Checkbox("ENG 1 Compressor Stall", &faults.eng1_compressor_stall);
        ImGui::Separator();
        ImGui::Text("Engine 2:");
        ImGui::Checkbox("ENG 2 N1 Sensor Fail", &faults.eng2_n1_sensor_fail);
        ImGui::Checkbox("ENG 2 N2 Sensor Fail", &faults.eng2_n2_sensor_fail);
        ImGui::Checkbox("ENG 2 EGT Sensor Fail", &faults.eng2_egt_sensor_fail);
        ImGui::Checkbox("ENG 2 Vibration High", &faults.eng2_vibration_high);
        ImGui::Checkbox("ENG 2 Oil Pressure Low", &faults.eng2_oil_pressure_low);
        ImGui::Checkbox("ENG 2 Compressor Stall", &faults.eng2_compressor_stall);
    }

    // Hydraulic faults
    if (ImGui::CollapsingHeader("Hydraulic Systems")) {
        ImGui::Text("System Failures:");
        ImGui::Checkbox("Green Hyd Fail (Complete)", &faults.green_hyd_fail);
        ImGui::Checkbox("Blue Hyd Fail (Complete)", &faults.blue_hyd_fail);
        ImGui::Checkbox("Yellow Hyd Fail (Complete)", &faults.yellow_hyd_fail);
        ImGui::Separator();
        ImGui::Text("Pump Failures:");
        ImGui::Checkbox("Green Eng 1 Pump Fail", &faults.green_eng1_pump_fail);
        ImGui::Checkbox("Blue Elec Pump Fail", &faults.blue_elec_pump_fail);
        ImGui::Checkbox("Yellow Eng 1 Pump Fail", &faults.yellow_eng1_pump_fail);
        ImGui::Separator();
        ImGui::Text("Reservoir Levels:");
        ImGui::Checkbox("Green Reservoir Low", &faults.green_reservoir_low);
        ImGui::Checkbox("Blue Reservoir Low", &faults.blue_reservoir_low);
        ImGui::Checkbox("Yellow Reservoir Low", &faults.yellow_reservoir_low);
    }

    // Electrical faults
    if (ImGui::CollapsingHeader("Electrical System")) {
        ImGui::Text("Complete Failures:");
        ImGui::Checkbox("Total Electrical Fail", &faults.total_electrical_fail);
        ImGui::Checkbox("Partial Electrical Fail (AC BUS 1)", &faults.partial_electrical_fail);
        ImGui::Separator();
        ImGui::Text("Generators:");
        ImGui::Checkbox("GEN 1 Fail", &faults.gen1_fail);
        ImGui::Checkbox("GEN 2 Fail", &faults.gen2_fail);
        ImGui::Checkbox("APU GEN Fail", &faults.apu_gen_fail);
        ImGui::Separator();
        ImGui::Text("Batteries:");
        ImGui::Checkbox("BAT 1 Fail", &faults.bat1_fail);
        ImGui::Checkbox("BAT 2 Fail", &faults.bat2_fail);
        ImGui::Separator();
        ImGui::Text("Buses:");
        ImGui::Checkbox("AC BUS 1 Fail", &faults.ac_bus1_fail);
        ImGui::Checkbox("AC BUS 2 Fail", &faults.ac_bus2_fail);
        ImGui::Separator();
        ImGui::Text("Emergency:");
        ImGui::Checkbox("RAT Deployed", &faults.rat_deployed);
        ImGui::Checkbox("RAT Fault", &faults.rat_fault);
    }

    // Flight control system faults
    if (ImGui::CollapsingHeader("Flight Control Systems")) {
        ImGui::Checkbox("Trim Runaway", &faults.trim_runaway);
        ImGui::Checkbox("Alpha Floor Fail", &faults.alpha_floor_fail);
    }

    ImGui::End();
    ImGui::PopStyleColor();
}

// ================================
// Aircraft Systems and Control Panel
// ================================
void DrawAircraftSystemsPanel(PilotInput& pilot, FlapsPosition& flaps, TrimSystem& trim,
                               Speedbrakes& speedbrakes, LandingGear& gear,
                               HydraulicSystem& hydraulics, EngineState& engines, APUState& apu,
                               AlertManager& alerts, const AutopilotState& ap) {
    ImGui::SetNextWindowPos(ImVec2(1090, 500), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(350, 240), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("AIRCRAFT SYSTEMS AND CONTROL", nullptr);

    TextCentered("FLIGHT CONTROLS & SYSTEMS", AirbusColors::GREEN);
    ImGui::Separator();
    ImGui::Spacing();

    // FLIGHT CONTROL INPUTS
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "SIDESTICK INPUT:");
    ImGui::PushItemWidth(150);
    ImGui::SliderFloat("Pitch", &pilot.pitch, -1.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::SliderFloat("Roll", &pilot.roll, -1.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::Spacing();

    // FLAPS
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "FLAPS/SLATS:");
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::RETRACTED ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("0", ImVec2(45, 20))) flaps = FlapsPosition::RETRACTED;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_1 ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("1", ImVec2(45, 20))) flaps = FlapsPosition::CONF_1;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_2 ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("2", ImVec2(45, 20))) flaps = FlapsPosition::CONF_2;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_3 ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("3", ImVec2(45, 20))) flaps = FlapsPosition::CONF_3;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_FULL ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("FULL", ImVec2(55, 20))) flaps = FlapsPosition::CONF_FULL;
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // THRUST LEVERS
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "THRUST:");
    const char* thrust_label;
    ImU32 thrust_color;
    if (pilot.thrust < 0.2f) { thrust_label = "IDLE"; thrust_color = AirbusColors::AMBER; }
    else if (pilot.thrust < 0.4f) { thrust_label = "LOW"; thrust_color = AirbusColors::WHITE; }
    else if (pilot.thrust < 0.7f) { thrust_label = "CLIMB"; thrust_color = AirbusColors::GREEN; }
    else if (pilot.thrust < 0.9f) { thrust_label = "MAX"; thrust_color = AirbusColors::GREEN; }
    else { thrust_label = "TOGA"; thrust_color = AirbusColors::MAGENTA; }

    ImGui::PushItemWidth(180);
    ImGui::SliderFloat("##thrust", &pilot.thrust, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::TextColored(ImColor(thrust_color), "[%s]", thrust_label);

    // Show ATHR status indicator
    if (ap.autothrust) {
        ImGui::SameLine();
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "A/THR");
    }

    ImGui::Spacing();

    // Two-column layout for compact display
    ImGui::Columns(2, nullptr, false);

    // TRIM
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "PITCH TRIM");
    ImGui::PushItemWidth(120);
    ImGui::SliderFloat("##trim", &trim.pitch_trim_deg, -13.5f, 4.0f, "%.1f°");
    ImGui::PopItemWidth();
    ImGui::Checkbox("Auto##trim", &trim.auto_trim);

    ImGui::Spacing();

    // SPEEDBRAKES
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "SPEEDBRAKES");
    ImGui::PushItemWidth(120);
    ImGui::SliderFloat("##spdbr", &speedbrakes.position, 0.0f, 1.0f, "%.0f%%");
    ImGui::PopItemWidth();
    ImGui::Checkbox("Armed##spdbr", &speedbrakes.armed);

    ImGui::NextColumn();

    // LANDING GEAR
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "LANDING GEAR");
    const char* gear_pos_names[] = {"UP", "DOWN", "TRANSIT"};
    ImU32 gear_color = (gear.position == GearPosition::DOWN) ? AirbusColors::GREEN :
                       (gear.position == GearPosition::UP) ? AirbusColors::AMBER : AirbusColors::RED;
    ImGui::TextColored(ImColor(gear_color), "%s", gear_pos_names[(int)gear.position]);

    if (ImGui::Button("DN", ImVec2(55, 20))) {
        if (gear.position == GearPosition::UP) {
            gear.position = GearPosition::TRANSIT;
            gear.transit_timer = 0.0f;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("UP", ImVec2(55, 20))) {
        if (gear.position == GearPosition::DOWN && !gear.weight_on_wheels) {
            gear.position = GearPosition::TRANSIT;
            gear.transit_timer = 0.0f;
        }
    }
    ImGui::Text("WOW: %s", gear.weight_on_wheels ? "Y" : "N");

    ImGui::Columns(1);
    ImGui::Separator();

    // HYDRAULICS (compact)
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "HYDRAULICS:");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(hydraulics.green_avail ? AirbusColors::GREEN : AirbusColors::RED), "G");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(hydraulics.blue_avail ? AirbusColors::GREEN : AirbusColors::RED), "B");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(hydraulics.yellow_avail ? AirbusColors::GREEN : AirbusColors::RED), "Y");

    ImGui::Spacing();

    // ENGINES with fire squibs
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "ENGINES");
    ImGui::Columns(2, "eng_cols", false);

    // Engine 1
    ImU32 eng1_color = engines.engine1_fire ? AirbusColors::RED :
                       engines.engine1_running ? AirbusColors::GREEN : AirbusColors::AMBER;
    ImGui::TextColored(ImColor(eng1_color), "ENG 1: %s",
                       engines.engine1_fire ? "FIRE" :
                       engines.engine1_running ? "RUN" : "OFF");
    if (ImGui::Button("START##1", ImVec2(50, 18))) engines.engine1_running = true;
    ImGui::SameLine();
    if (ImGui::Button("STOP##1", ImVec2(50, 18))) engines.engine1_running = false;
    ImGui::Checkbox("FIRE##1", &engines.engine1_fire);

    // Fire squib button for Engine 1
    ImGui::PushStyleColor(ImGuiCol_Button, engines.engine1_squib_released ? AirbusColors::AMBER : IM_COL32(100,0,0,255));
    if (ImGui::Button("SQUIB##1", ImVec2(105, 20))) {
        engines.engine1_squib_released = true;
        if (engines.engine1_fire) {
            engines.engine1_fire = false;  // Extinguish fire
        }
    }
    ImGui::PopStyleColor();
    if (engines.engine1_squib_released) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "AGENT DISCH");
    }

    ImGui::NextColumn();

    // Engine 2
    ImU32 eng2_color = engines.engine2_fire ? AirbusColors::RED :
                       engines.engine2_running ? AirbusColors::GREEN : AirbusColors::AMBER;
    ImGui::TextColored(ImColor(eng2_color), "ENG 2: %s",
                       engines.engine2_fire ? "FIRE" :
                       engines.engine2_running ? "RUN" : "OFF");
    if (ImGui::Button("START##2", ImVec2(50, 18))) engines.engine2_running = true;
    ImGui::SameLine();
    if (ImGui::Button("STOP##2", ImVec2(50, 18))) engines.engine2_running = false;
    ImGui::Checkbox("FIRE##2", &engines.engine2_fire);

    // Fire squib button for Engine 2
    ImGui::PushStyleColor(ImGuiCol_Button, engines.engine2_squib_released ? AirbusColors::AMBER : IM_COL32(100,0,0,255));
    if (ImGui::Button("SQUIB##2", ImVec2(105, 20))) {
        engines.engine2_squib_released = true;
        if (engines.engine2_fire) {
            engines.engine2_fire = false;  // Extinguish fire
        }
    }
    ImGui::PopStyleColor();
    if (engines.engine2_squib_released) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "AGENT DISCH");
    }

    ImGui::Columns(1);
    ImGui::Separator();

    // APU
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "APU (Auxiliary Power Unit)");
    ImU32 apu_color = apu.fire ? AirbusColors::RED :
                      apu.running ? AirbusColors::GREEN : AirbusColors::AMBER;
    ImGui::TextColored(ImColor(apu_color), "Status: %s",
                       apu.fire ? "FIRE" :
                       apu.running ? "RUNNING" : "OFF");

    if (ImGui::Button("START APU", ImVec2(80, 20))) apu.running = true;
    ImGui::SameLine();
    if (ImGui::Button("STOP APU", ImVec2(80, 20))) apu.running = false;
    ImGui::SameLine();
    ImGui::Checkbox("FIRE##APU", &apu.fire);

    // APU fire squib
    ImGui::PushStyleColor(ImGuiCol_Button, apu.squib_released ? AirbusColors::AMBER : IM_COL32(100,0,0,255));
    if (ImGui::Button("APU SQUIB", ImVec2(105, 20))) {
        apu.squib_released = true;
        if (apu.fire) {
            apu.fire = false;  // Extinguish fire
        }
    }
    ImGui::PopStyleColor();
    if (apu.squib_released) {
        ImGui::SameLine();
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "DISCHARGED");
    }

    // ECAM status summary (simple)
    ImGui::Separator();
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "ECAM STATUS:");
    if (alerts.masterWarningOn()) {
        ImGui::TextColored(ImColor(AirbusColors::RED), "MASTER WARNING");
    } else if (alerts.masterCautionOn()) {
        ImGui::TextColored(ImColor(AirbusColors::AMBER), "MASTER CAUTION");
    } else {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "NORMAL");
    }

    ImGui::End();
    ImGui::PopStyleColor();
}

void DrawSystemsPanel(TrimSystem& trim, Speedbrakes& speedbrakes, LandingGear& gear, FlightPhase phase,
                      HydraulicSystem& hydraulics, EngineState& engines, Weather& weather, Faults& faults) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 20, 255));
    ImGui::SetNextWindowSize(ImVec2(350, 580), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(1090, 10), ImGuiCond_Once);
    ImGui::Begin("AIRCRAFT SYSTEMS", nullptr);

    // Flight Phase Display
    ImGui::Text("FLIGHT PHASE:");
    ImGui::SameLine();
    const char* phase_names[] = {"PREFLIGHT", "TAXI", "TAKEOFF", "CLIMB", "CRUISE", "DESCENT", "APPROACH", "LANDING", "ROLLOUT"};
    ImGui::TextColored(ImColor(AirbusColors::GREEN), "%s", phase_names[(int)phase]);

    ImGui::Separator();
    ImGui::Spacing();

    // TRIM SYSTEM
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "PITCH TRIM");
    ImGui::PushItemWidth(150);
    ImGui::SliderFloat("##trim", &trim.pitch_trim_deg, -13.5f, 4.0f, "%.1f deg");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Checkbox("Auto Trim", &trim.auto_trim);

    ImGui::Spacing();

    // SPEEDBRAKES
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "SPEEDBRAKES");
    ImGui::PushItemWidth(150);
    ImGui::SliderFloat("##speedbrake", &speedbrakes.position, 0.0f, 1.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Checkbox("Armed", &speedbrakes.armed);

    ImGui::Spacing();

    // LANDING GEAR
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "LANDING GEAR");
    const char* gear_pos_names[] = {"UP", "DOWN", "TRANSIT"};
    ImU32 gear_color = (gear.position == GearPosition::DOWN) ? AirbusColors::GREEN :
                       (gear.position == GearPosition::UP) ? AirbusColors::AMBER :
                       AirbusColors::RED;
    ImGui::TextColored(ImColor(gear_color), "Position: %s", gear_pos_names[(int)gear.position]);

    if (ImGui::Button("GEAR DOWN", ImVec2(100, 25))) {
        if (gear.position == GearPosition::UP) {
            gear.position = GearPosition::TRANSIT;
            gear.transit_timer = 0.0f;
        } else if (gear.position == GearPosition::TRANSIT) {
            gear.position = GearPosition::DOWN;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("GEAR UP", ImVec2(100, 25))) {
        if (gear.position == GearPosition::DOWN && !gear.weight_on_wheels) {
            gear.position = GearPosition::TRANSIT;
            gear.transit_timer = 0.0f;
        } else if (gear.position == GearPosition::TRANSIT) {
            gear.position = GearPosition::UP;
        }
    }
    ImGui::Text("Weight on Wheels: %s", gear.weight_on_wheels ? "YES" : "NO");

    ImGui::Separator();
    ImGui::Spacing();

    // HYDRAULIC SYSTEMS
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "HYDRAULICS");
    ImGui::TextColored(ImColor(hydraulics.green_avail ? AirbusColors::GREEN : AirbusColors::AMBER), 
                       "GREEN:  %s", hydraulics.green_avail ? "AVAIL" : "FAULT");
    ImGui::TextColored(ImColor(hydraulics.blue_avail ? AirbusColors::GREEN : AirbusColors::AMBER), 
                       "BLUE:   %s", hydraulics.blue_avail ? "AVAIL" : "FAULT");
    ImGui::TextColored(ImColor(hydraulics.yellow_avail ? AirbusColors::GREEN : AirbusColors::AMBER), 
                       "YELLOW: %s", hydraulics.yellow_avail ? "AVAIL" : "FAULT");

    ImGui::Spacing();

    // ENGINE STATUS
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "ENGINES");
    ImGui::Columns(2, "engine_cols", false);

    ImU32 eng1_color = engines.engine1_fire ? AirbusColors::RED :
                       engines.engine1_running ? AirbusColors::GREEN : AirbusColors::AMBER;
    ImGui::TextColored(ImColor(eng1_color), "ENG 1: %s", 
                       engines.engine1_fire ? "FIRE" :
                       engines.engine1_running ? "RUN" : "OFF");
    if (ImGui::Button("START##1", ImVec2(60, 20))) engines.engine1_running = true;
    ImGui::SameLine();
    if (ImGui::Button("STOP##1", ImVec2(60, 20))) engines.engine1_running = false;
    ImGui::Checkbox("FIRE##1", &engines.engine1_fire);

    ImGui::NextColumn();

    ImU32 eng2_color = engines.engine2_fire ? AirbusColors::RED :
                       engines.engine2_running ? AirbusColors::GREEN : AirbusColors::AMBER;
    ImGui::TextColored(ImColor(eng2_color), "ENG 2: %s", 
                       engines.engine2_fire ? "FIRE" :
                       engines.engine2_running ? "RUN" : "OFF");
    if (ImGui::Button("START##2", ImVec2(60, 20))) engines.engine2_running = true;
    ImGui::SameLine();
    if (ImGui::Button("STOP##2", ImVec2(60, 20))) engines.engine2_running = false;
    ImGui::Checkbox("FIRE##2", &engines.engine2_fire);

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::Spacing();

    // WEATHER
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "WEATHER");
    ImGui::PushItemWidth(200);
    ImGui::SliderFloat("Wind Speed", &weather.wind_speed_knots, 0.0f, 100.0f, "%.0f kt");
    ImGui::SliderFloat("Wind Direction", &weather.wind_direction_deg, 0.0f, 359.0f, "%.0f deg");
    ImGui::SliderFloat("Turbulence", &weather.turbulence_intensity, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Windshear", &weather.windshear_intensity, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::Separator();
    ImGui::Spacing();

    // FAULT INJECTION
    ImGui::TextColored(ImColor(AirbusColors::RED), "FAULT INJECTION");
    ImGui::Checkbox("Trim Runaway", &faults.trim_runaway);
    ImGui::Checkbox("Green Hyd Fail", &faults.green_hyd_fail);
    ImGui::Checkbox("Blue Hyd Fail", &faults.blue_hyd_fail);
    ImGui::Checkbox("Yellow Hyd Fail", &faults.yellow_hyd_fail);

    ImGui::End();
    ImGui::PopStyleColor();
}
