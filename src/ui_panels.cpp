//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.
#include "ui_panels.h"
#include "imgui.h"
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
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(420, 100), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("MASTER WARNING/CAUTION", nullptr, ImGuiWindowFlags_NoResize);

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
void DrawEcamPanel(AlertManager& alerts, Sensors& sensors, PilotInput& pilot, Faults& faults, const PrimCore& prim, FlapsPosition flaps) {
    ImGui::SetNextWindowPos(ImVec2(20, 140), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(420, 560), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("ECAM E/WD", nullptr, ImGuiWindowFlags_NoResize);

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

// Helper to draw speed tape
static void DrawSpeedTape(ImVec2 pos, ImVec2 size, float speed_knots) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(20, 20, 30, 230));
    draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(100, 100, 100, 255), 0.0f, 0, 2.0f);

    // Draw speed markings
    float center_y = pos.y + size.y * 0.5f;
    float pixels_per_knot = 2.0f;

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
void DrawPFDPanel(const Sensors& sensors, const PrimCore& prim, const PilotInput& pilot) {
    ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("PRIMARY FLIGHT DISPLAY", nullptr, ImGuiWindowFlags_NoResize);

    const auto& fctl = prim.fctl_status();

    // FMA (Flight Mode Annunciator) - top line
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 255));
    ImGui::BeginChild("FMA", ImVec2(0, 35), true);

    const char* law_text = (fctl.law == ControlLaw::NORMAL) ? "NORMAL LAW" :
                           (fctl.law == ControlLaw::ALTERNATE) ? "ALT LAW" : "DIRECT LAW";
    ImU32 law_color = (fctl.law == ControlLaw::NORMAL) ? AirbusColors::GREEN : AirbusColors::AMBER;

    ImGui::PushStyleColor(ImGuiCol_Text, law_color);
    ImGui::SetCursorPosX(10);
    ImGui::Text("%s", law_text);
    ImGui::PopStyleColor();

    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
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

    // Draw artificial horizon in center
    ImVec2 horizon_center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f - 20);
    DrawArtificialHorizon(horizon_center, 100.0f, sensors.pitch_deg, sensors.roll_deg);

    // Draw speed tape on left
    ImVec2 speed_tape_pos = ImVec2(canvas_pos.x + 10, canvas_pos.y + 40);
    DrawSpeedTape(speed_tape_pos, ImVec2(50, 300), sensors.ias_knots);

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
    ImGui::SetNextWindowPos(ImVec2(460, 540), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(500, 170), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("F/CTL STATUS", nullptr, ImGuiWindowFlags_NoResize);

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
// Control Input Panel
// ================================
void DrawControlInputPanel(PilotInput& pilot, Sensors& sensors, Faults& faults, SimulationSettings& sim_settings, FlapsPosition& flaps) {
    ImGui::SetNextWindowPos(ImVec2(980, 20), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 690), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("SIMULATOR CONTROLS", nullptr, ImGuiWindowFlags_NoResize);

    TextCentered("FLIGHT SIMULATOR INPUTS", AirbusColors::AMBER);
    ImGui::Separator();
    ImGui::Spacing();

    // Pilot inputs (always available)
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "SIDESTICK INPUT:");
    ImGui::SliderFloat("Pitch", &pilot.pitch, -1.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Roll", &pilot.roll, -1.0f, 1.0f, "%.2f");

    ImGui::Spacing();
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "FLAPS/SLATS LEVER:");

    // Airbus-style flaps buttons (better than dropdown)
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::RETRACTED ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("0", ImVec2(45, 25))) flaps = FlapsPosition::RETRACTED;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_1 ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("1", ImVec2(45, 25))) flaps = FlapsPosition::CONF_1;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_2 ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("2", ImVec2(45, 25))) flaps = FlapsPosition::CONF_2;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_3 ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("3", ImVec2(45, 25))) flaps = FlapsPosition::CONF_3;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, flaps == FlapsPosition::CONF_FULL ? AirbusColors::GREEN : IM_COL32(60,60,60,255));
    if (ImGui::Button("FULL", ImVec2(45, 25))) flaps = FlapsPosition::CONF_FULL;
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "THRUST LEVERS:");

    // Color-code thrust slider
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

    ImGui::SliderFloat("Thrust", &pilot.thrust, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::TextColored(ImColor(thrust_color), "[%s]", thrust_label);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Manual sensor override toggle (for QF72-style scenarios)
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::AMBER);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, AirbusColors::RED);
    bool override_changed = ImGui::Checkbox("MANUAL SENSOR OVERRIDE (QF72 Mode)", &sim_settings.manual_sensor_override);
    ImGui::PopStyleColor(2);

    if (sim_settings.manual_sensor_override) {
        ImGui::TextColored(ImColor(AirbusColors::RED), "  WARNING: Physics disabled!");
        ImGui::TextColored(ImColor(IM_COL32(180,180,180,255)), "  You can inject false sensor data");
    } else {
        ImGui::TextColored(ImColor(AirbusColors::GREEN), "  Physics simulation active");
        ImGui::TextColored(ImColor(IM_COL32(180,180,180,255)), "  Sensors respond to control inputs");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Flight parameters
    ImGui::TextColored(ImColor(AirbusColors::CYAN), "FLIGHT PARAMETERS:");

    if (sim_settings.manual_sensor_override) {
        // Manual mode: Show editable sliders
        ImGui::SliderFloat("IAS (kt)", &sensors.ias_knots, 60.0f, 380.0f, "%.0f");
        ImGui::SliderFloat("Altitude (ft)", &sensors.altitude_ft, 0.0f, 45000.0f, "%.0f");
        ImGui::SliderFloat("V/S (fpm)", &sensors.vs_fpm, -6000.0f, 6000.0f, "%.0f");
        ImGui::SliderFloat("AoA (deg)", &sensors.aoa_deg, -5.0f, 25.0f, "%.1f");
        ImGui::SliderFloat("Heading (deg)", &sensors.heading_deg, 0.0f, 359.0f, "%.0f");
        ImGui::SliderFloat("Mach", &sensors.mach, 0.0f, 0.85f, "%.3f");
        ImGui::SliderFloat("Pitch (deg)", &sensors.pitch_deg, -30.0f, 30.0f, "%.1f");
        ImGui::SliderFloat("Roll (deg)", &sensors.roll_deg, -60.0f, 60.0f, "%.1f");
    } else {
        // Physics mode: Show read-only displays
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(100, 100, 100, 255));
        ImGui::BeginDisabled();

        ImGui::SliderFloat("IAS (kt)", &sensors.ias_knots, 60.0f, 380.0f, "%.0f");
        ImGui::SliderFloat("Altitude (ft)", &sensors.altitude_ft, 0.0f, 45000.0f, "%.0f");
        ImGui::SliderFloat("V/S (fpm)", &sensors.vs_fpm, -6000.0f, 6000.0f, "%.0f");
        ImGui::SliderFloat("AoA (deg)", &sensors.aoa_deg, -5.0f, 25.0f, "%.1f");
        ImGui::SliderFloat("Heading (deg)", &sensors.heading_deg, 0.0f, 359.0f, "%.0f");

        ImGui::EndDisabled();
        ImGui::PopStyleColor(2);

        ImGui::TextColored(ImColor(IM_COL32(150,150,150,255)), " (Computed from flight dynamics)");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Sensor faults
    ImGui::TextColored(ImColor(AirbusColors::RED), "SENSOR FAULTS:");
    ImGui::Checkbox("ADR 1 Failure", &faults.adr1_fail);
    ImGui::Checkbox("Overspeed Sensor Bad", &faults.overspeed_sensor_bad);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // System faults
    ImGui::TextColored(ImColor(AirbusColors::RED), "SYSTEM FAULTS:");
    ImGui::Checkbox("ELAC 1 Fault", &faults.elac1_fail);
    ImGui::Checkbox("ELAC 2 Fault", &faults.elac2_fail);
    ImGui::Checkbox("SEC 1 Fault", &faults.sec1_fail);
    ImGui::Checkbox("Elevator Jam", &faults.elevator_jam);
    ImGui::Checkbox("Aileron Jam", &faults.aileron_jam);
    ImGui::Checkbox("Alpha Floor Fail", &faults.alpha_floor_fail);

    ImGui::End();
    ImGui::PopStyleColor();
}

// ================================
// Autopilot Control Panel (FCU-style)
// ================================
void DrawAutopilotPanel(AutopilotState& ap, const Sensors& sensors) {
    ImGui::SetNextWindowPos(ImVec2(460, 730), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(820, 160), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, AirbusColors::DARK_BG);
    ImGui::Begin("AUTOPILOT / FCU", nullptr, ImGuiWindowFlags_NoResize);

    TextCentered("FLIGHT CONTROL UNIT", AirbusColors::CYAN);
    ImGui::Separator();
    ImGui::Spacing();

    // Layout: 4 columns for SPD, HDG, ALT, VS
    ImGui::Columns(4, nullptr, false);

    // ===== SPEED COLUMN =====
    ImGui::BeginChild("SPD_Section", ImVec2(0, 80), true);
    TextCentered("SPEED", AirbusColors::CYAN);
    ImGui::Separator();

    // Mode button
    ImU32 spd_btn_color = ap.spd_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, spd_btn_color);
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::WHITE);
    if (ImGui::Button("SPD", ImVec2(60, 25))) {
        ap.spd_mode = !ap.spd_mode;
        if (ap.spd_mode) {
            // Sync target to current speed when mode is enabled
            ap.target_spd_knots = sensors.ias_knots;
        }
    }
    ImGui::PopStyleColor(2);

    // Target value
    ImGui::PushItemWidth(120);
    if (ap.spd_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##spd_target", &ap.target_spd_knots, 1.0f, 100.0f, 350.0f, "%.0f kt");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 255));
        ImGui::BeginDisabled();
        ImGui::DragFloat("##spd_target", &ap.target_spd_knots, 1.0f, 100.0f, 350.0f, "%.0f kt");
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
    }
    ImGui::PopItemWidth();

    // Current value display
    ImGui::TextColored(ImColor(IM_COL32(150, 150, 150, 255)), "CUR: %.0f kt", sensors.ias_knots);

    ImGui::EndChild();
    ImGui::NextColumn();

    // ===== HEADING COLUMN =====
    ImGui::BeginChild("HDG_Section", ImVec2(0, 80), true);
    TextCentered("HEADING", AirbusColors::CYAN);
    ImGui::Separator();

    ImU32 hdg_btn_color = ap.hdg_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, hdg_btn_color);
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::WHITE);
    if (ImGui::Button("HDG", ImVec2(60, 25))) {
        ap.hdg_mode = !ap.hdg_mode;
        if (ap.hdg_mode) {
            // Sync target to current heading when mode is enabled
            ap.target_hdg_deg = sensors.heading_deg;
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::PushItemWidth(120);
    if (ap.hdg_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##hdg_target", &ap.target_hdg_deg, 1.0f, 0.0f, 359.0f, "%.0f°");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 255));
        ImGui::BeginDisabled();
        ImGui::DragFloat("##hdg_target", &ap.target_hdg_deg, 1.0f, 0.0f, 359.0f, "%.0f°");
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
    }
    ImGui::PopItemWidth();

    ImGui::TextColored(ImColor(IM_COL32(150, 150, 150, 255)), "CUR: %.0f°", sensors.heading_deg);

    ImGui::EndChild();
    ImGui::NextColumn();

    // ===== ALTITUDE COLUMN =====
    ImGui::BeginChild("ALT_Section", ImVec2(0, 80), true);
    TextCentered("ALTITUDE", AirbusColors::CYAN);
    ImGui::Separator();

    ImU32 alt_btn_color = ap.alt_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, alt_btn_color);
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::WHITE);
    if (ImGui::Button("ALT", ImVec2(60, 25))) {
        ap.alt_mode = !ap.alt_mode;
        if (ap.alt_mode) {
            // Sync target to current altitude when mode is enabled
            ap.target_alt_ft = sensors.altitude_ft;
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::PushItemWidth(120);
    if (ap.alt_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##alt_target", &ap.target_alt_ft, 100.0f, 0.0f, 45000.0f, "%.0f ft");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 255));
        ImGui::BeginDisabled();
        ImGui::DragFloat("##alt_target", &ap.target_alt_ft, 100.0f, 0.0f, 45000.0f, "%.0f ft");
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
    }
    ImGui::PopItemWidth();

    ImGui::TextColored(ImColor(IM_COL32(150, 150, 150, 255)), "CUR: %.0f ft", sensors.altitude_ft);

    ImGui::EndChild();
    ImGui::NextColumn();

    // ===== VERTICAL SPEED COLUMN =====
    ImGui::BeginChild("VS_Section", ImVec2(0, 80), true);
    TextCentered("VERT SPEED", AirbusColors::CYAN);
    ImGui::Separator();

    ImU32 vs_btn_color = ap.vs_mode ? AirbusColors::GREEN : IM_COL32(60, 60, 60, 255);
    ImGui::PushStyleColor(ImGuiCol_Button, vs_btn_color);
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::WHITE);
    if (ImGui::Button("V/S", ImVec2(60, 25))) {
        ap.vs_mode = !ap.vs_mode;
        if (ap.vs_mode) {
            // Sync target to current VS when mode is enabled
            ap.target_vs_fpm = sensors.vs_fpm;
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::PushItemWidth(120);
    if (ap.vs_mode) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 80, 0, 255));
        ImGui::DragFloat("##vs_target", &ap.target_vs_fpm, 100.0f, -6000.0f, 6000.0f, "%+.0f fpm");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(40, 40, 40, 255));
        ImGui::BeginDisabled();
        ImGui::DragFloat("##vs_target", &ap.target_vs_fpm, 100.0f, -6000.0f, 6000.0f, "%+.0f fpm");
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
    }
    ImGui::PopItemWidth();

    ImGui::TextColored(ImColor(IM_COL32(150, 150, 150, 255)), "CUR: %+.0f fpm", sensors.vs_fpm);

    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::Columns(1);

    // AP Disconnect Button (center bottom, like real Airbus)
    ImGui::Spacing();
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 180) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 50, 50, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(150, 0, 0, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, AirbusColors::WHITE);
    if (ImGui::Button("AP DISCONNECT", ImVec2(180, 35))) {
        // Disconnect all autopilot modes (triggers master warning)
        ap.spd_mode = false;
        ap.hdg_mode = false;
        ap.alt_mode = false;
        ap.vs_mode = false;
    }
    ImGui::PopStyleColor(4);

    ImGui::End();
    ImGui::PopStyleColor();
}
