//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.
#include "alerts.h"

AlertEdge AlertManager::set(int id, AlertLevel level, const std::string& text, bool condition_active, bool latch_when_active) {
    Alert* a = nullptr;
    for (auto& it : alerts_) {
        if (it.id == id) { a = &it; break; }
    }
    if (!a) {
        alerts_.push_back(Alert{ id, level, text, false, false, false });
        a = &alerts_.back();
    }

    a->level = level;
    a->text  = text;

    AlertEdge edge{};
    bool prevShown = (a->active || a->latched);

    a->active = condition_active;

    if (condition_active && latch_when_active) a->latched = true;

    bool nowShown = (a->active || a->latched);
    if (!prevShown && nowShown) {
        edge.became_active = true;
        a->acknowledged = false;
    }
    if (prevShown && !nowShown) edge.became_inactive = true;

    return edge;
}

void AlertManager::clearLatched(int id) {
    for (auto& a : alerts_) {
        if (a.id == id) {
            a.latched = false;
            if (!a.active) a.acknowledged = false;
            return;
        }
    }
}

void AlertManager::clearAllLatched() {
    for (auto& a : alerts_) a.latched = false;
}

bool AlertManager::masterWarningOn() const {
    for (const auto& a : alerts_) {
        bool shown = (a.active || a.latched);
        if (shown && a.level == AlertLevel::WARNING && !a.acknowledged) return true;
    }
    return false;
}

bool AlertManager::masterCautionOn() const {
    if (masterWarningOn()) return false;
    for (const auto& a : alerts_) {
        bool shown = (a.active || a.latched);
        if (shown && a.level == AlertLevel::CAUTION && !a.acknowledged) return true;
    }
    return false;
}

void AlertManager::acknowledgeAllVisible() {
    for (auto& a : alerts_) {
        bool shown = (a.active || a.latched);
        if (shown) a.acknowledged = true;
    }
}

std::vector<const Alert*> AlertManager::getShownSorted(AlertLevel lvl) const {
    std::vector<const Alert*> out;
    for (const auto& a : alerts_) {
        bool shown = (a.active || a.latched);
        if (shown && a.level == lvl) out.push_back(&a);
    }
    return out;
}
