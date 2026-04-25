#pragma once
#include <string>
#include <vector>
#include "Tool.h"

namespace CamEngine {

enum class MachineType {
    Mill3Axis,
    Mill4Axis,
    Mill5Axis,
    Lathe,
    LaserCutter,
    PlasmaCutter,
    Router3Axis
};

inline std::string machineTypeName(MachineType t) {
    switch (t) {
        case MachineType::Mill3Axis:    return "Fraiseuse 3 axes";
        case MachineType::Mill4Axis:    return "Fraiseuse 4 axes";
        case MachineType::Mill5Axis:    return "Fraiseuse 5 axes";
        case MachineType::Lathe:        return "Tour";
        case MachineType::LaserCutter:  return "Découpe laser";
        case MachineType::PlasmaCutter: return "Découpe plasma";
        case MachineType::Router3Axis:  return "Défonceuse 3 axes";
        default:                        return "Inconnu";
    }
}

struct WorkEnvelope {
    double xTravel = 300.0;
    double yTravel = 200.0;
    double zTravel = 150.0;
};

struct SpindleConfig {
    double minRPM   = 100.0;
    double maxRPM   = 20000.0;
    double maxPower = 1000.0;   // W
    bool   hasCoolant = true;
    bool   hasThroughCoolant = false;
};

struct MachineProfile {
    std::string id;
    std::string name            = "Ma CNC 3 axes";
    MachineType type            = MachineType::Mill3Axis;
    std::string postProcessorId = "grbl";
    std::string description;

    WorkEnvelope  envelope;
    SpindleConfig spindle;

    double maxFeedRate    = 3000.0;  // mm/min
    double maxRapidRate   = 8000.0;  // mm/min
    double maxPlungeRate  = 500.0;   // mm/min

    std::vector<Tool> toolLibrary;

    // === Persistence JSON ===
    void saveToFile(const std::string& path) const;
    static MachineProfile loadFromFile(const std::string& path);

    // === Gestion de la collection de profils ===
    static std::vector<MachineProfile> loadAll();
    static void saveAll(const std::vector<MachineProfile>& profiles);
    static std::string profilesDir();     // ~/.config/cam-engine/profiles/
    static MachineProfile defaultProfile();
};

} // namespace CamEngine
