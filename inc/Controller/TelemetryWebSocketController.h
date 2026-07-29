#pragma once

#include "Services/TelemetryManager.h"
#include "Services/RecoveryService.h"
#include <boost/json.hpp>
#include <functional>
#include <string>

class TelemetryWebSocketController {
public:
    using SendTextCallback = std::function<void(const std::string&)>;

    TelemetryWebSocketController(TelemetryManager& telemetry, RecoveryService& recovery);

    void onMessage(const std::string& rawPayload, SendTextCallback sendReply);
    void broadcastTelemetry(SendTextCallback broadcast);

private:
    TelemetryManager& telemetryManager_;
    RecoveryService& recoveryService_;
};