#pragma once
#include <string>
#include <cmath>

namespace CamEngine {

enum class ToolType {
    FlatEndMill,
    BallEndMill,
    BullNoseEndMill,
    DrillBit,
    TurningInsert,
    ThreadingTool,
    LaserHead,
    PlasmaHead,
    VBit
};

inline std::string toolTypeName(ToolType t) {
    switch (t) {
        case ToolType::FlatEndMill:     return "Fraise 2 tailles (plate)";
        case ToolType::BallEndMill:     return "Fraise boule";
        case ToolType::BullNoseEndMill: return "Fraise nez de taureau";
        case ToolType::DrillBit:        return "Foret";
        case ToolType::TurningInsert:   return "Plaquette tournage";
        case ToolType::ThreadingTool:   return "Outil de filetage";
        case ToolType::LaserHead:       return "Tête laser";
        case ToolType::PlasmaHead:      return "Tête plasma";
        case ToolType::VBit:            return "Fraise en V";
        default:                        return "Inconnu";
    }
}

struct Tool {
    int         id            = 1;
    std::string name          = "Ø6 Fraise 2T";
    ToolType    type          = ToolType::FlatEndMill;

    double diameter           = 6.0;    // mm
    double cornerRadius       = 0.0;    // mm (pour bull nose / V-bit)
    double fluteLength        = 20.0;   // mm
    double totalLength        = 60.0;   // mm
    int    flutes             = 2;

    double maxRPM             = 15000;  // tr/min
    double feedPerTooth       = 0.02;   // mm/dent
    double plungeFeedFactor   = 0.3;    // fraction du feed radial

    // Calculs dérivés
    double feedRate(double rpm) const {
        return feedPerTooth * flutes * rpm;
    }
    double plungeFeedRate(double rpm) const {
        return feedRate(rpm) * plungeFeedFactor;
    }
    double recommendedRPM() const {
        return maxRPM * 0.75;
    }
};

} // namespace CamEngine
