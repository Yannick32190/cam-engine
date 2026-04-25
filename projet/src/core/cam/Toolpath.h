#pragma once
#include <vector>
#include <string>

namespace CamEngine {

enum class MoveType {
    Rapid,      // G0 — déplacement rapide
    Feed,       // G1 — interpolation linéaire
    ArcCW,      // G2 — arc horaire
    ArcCCW,     // G3 — arc anti-horaire
    Dwell,      // G4 — temporisation
    DrillCycle  // G81/G83 — cycle de perçage
};

struct ToolMove {
    MoveType type       = MoveType::Feed;
    double   x          = 0.0;
    double   y          = 0.0;
    double   z          = 0.0;
    double   i          = 0.0;   // offset centre arc (G2/G3)
    double   j          = 0.0;
    double   feedRate   = 0.0;   // mm/min  (0 = hérite du contexte)
    double   dwellSec   = 0.0;   // pour Dwell
    std::string comment;
};

struct Toolpath {
    std::string           name;
    std::vector<ToolMove> moves;
    bool                  isComputed = false;
};

} // namespace CamEngine
