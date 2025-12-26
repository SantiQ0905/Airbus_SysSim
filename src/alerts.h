//
//
// Santiago Quintana Moreno A01571222
// Created on: 24/12/2025.
#pragma once
#include <string>
#include <vector>

enum class AlertLevel { MEMO, CAUTION, WARNING };

struct Alert {
    int id = 0;
    AlertLevel level = AlertLevel::MEMO;
    std::string text;
    bool active = false;
    bool latched = false;
    bool acknowledged = false;
    std::vector<std::string> ecam_actions; // Procedural steps for pilot to follow
};

struct AlertEdge {
    bool became_active = false;
    bool became_inactive = false;
};

class AlertManager {
public:
    AlertEdge set(int id, AlertLevel level, const std::string& text, bool condition_active, bool latch_when_active);
    AlertEdge set(int id, AlertLevel level, const std::string& text, bool condition_active, bool latch_when_active, const std::vector<std::string>& ecam_actions);

    void clearLatched(int id);
    void clearAllLatched();

    bool masterWarningOn() const;
    bool masterCautionOn() const;

    void acknowledgeAllVisible();

    std::vector<const Alert*> getShownSorted(AlertLevel lvl) const;

    const std::vector<Alert>& all() const { return alerts_; }

private:
    std::vector<Alert> alerts_;
};
