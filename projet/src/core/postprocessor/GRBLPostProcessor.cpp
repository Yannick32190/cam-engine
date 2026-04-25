#include "GRBLPostProcessor.h"
#include <sstream>
#include <cmath>

namespace CamEngine {

std::string GRBLPostProcessor::generate(
    const std::vector<Toolpath>& paths,
    const Tool&                  tool,
    const MachineProfile&        machine,
    const std::string&           programName) const
{
    std::ostringstream out;
    double rpm    = std::min(tool.maxRPM * 0.75, machine.spindle.maxRPM);
    double feed   = std::min(tool.feedRate(rpm), machine.maxFeedRate);
    double plunge = std::min(tool.plungeFeedRate(rpm), machine.maxPlungeRate);
    double rapid  = machine.maxRapidRate;

    out << "; G-code généré par CAM-ENGINE\n";
    out << "; Post-processeur : " << name() << "\n";
    out << "; Outil  : T" << tool.id << " — " << tool.name << "\n";
    out << "; Diam.  : " << fmt(tool.diameter) << " mm\n";
    out << "; RPM    : " << fmt(rpm, 0) << "\n";
    out << "; Feed   : " << fmt(feed, 0) << " mm/min\n";
    out << "; Machine: " << machine.name << "\n\n";

    out << "G21        ; Unités mm\n";
    out << "G17        ; Plan XY\n";
    out << "G90        ; Absolu\n";
    out << "G94        ; Feed mm/min\n";
    out << "G54        ; Origine pièce\n";
    out << "M3 S" << static_cast<int>(rpm) << "  ; Broche\n";
    out << "G4 P1      ; Stabilisation\n\n";

    double lastX = 1e30, lastY = 1e30, lastZ = 1e30;
    double lastF = -1;
    bool   motionStarted = false;

    auto emitF = [&](double f) -> std::string {
        if (std::abs(f - lastF) > 0.1) { lastF = f; return " F" + fmt(f, 0); }
        return "";
    };

    for (const auto& tp : paths) {
        if (!tp.isComputed) continue;
        out << "; --- " << tp.name << "\n";

        for (const auto& mv : tp.moves) {
            if (!mv.comment.empty()) out << "; " << mv.comment << "\n";

            switch (mv.type) {
            case MoveType::Rapid: {
                out << "G0";
                if (std::abs(mv.x - lastX) > 1e-4) { out << " X" << fmt(mv.x); lastX = mv.x; }
                if (std::abs(mv.y - lastY) > 1e-4) { out << " Y" << fmt(mv.y); lastY = mv.y; }
                if (std::abs(mv.z - lastZ) > 1e-4) { out << " Z" << fmt(mv.z); lastZ = mv.z; }
                out << "\n";
                motionStarted = true;
                break;
            }
            case MoveType::Feed: {
                double f = (mv.feedRate > 0) ? mv.feedRate
                         : (motionStarted && lastZ == mv.z) ? feed : plunge;
                out << "G1";
                if (std::abs(mv.x - lastX) > 1e-4) { out << " X" << fmt(mv.x); lastX = mv.x; }
                if (std::abs(mv.y - lastY) > 1e-4) { out << " Y" << fmt(mv.y); lastY = mv.y; }
                if (std::abs(mv.z - lastZ) > 1e-4) { out << " Z" << fmt(mv.z); lastZ = mv.z; }
                out << emitF(f) << "\n";
                motionStarted = true;
                break;
            }
            case MoveType::ArcCW:
            case MoveType::ArcCCW: {
                out << (mv.type == MoveType::ArcCW ? "G2" : "G3");
                if (std::abs(mv.x - lastX) > 1e-4) { out << " X" << fmt(mv.x); lastX = mv.x; }
                if (std::abs(mv.y - lastY) > 1e-4) { out << " Y" << fmt(mv.y); lastY = mv.y; }
                if (std::abs(mv.z - lastZ) > 1e-4) { out << " Z" << fmt(mv.z); lastZ = mv.z; }
                out << " I" << fmt(mv.i) << " J" << fmt(mv.j);
                out << emitF(feed) << "\n";
                break;
            }
            case MoveType::Dwell:
                out << "G4 P" << fmt(mv.dwellSec, 1) << "\n";
                break;
            case MoveType::DrillCycle:
                out << "G81 X" << fmt(mv.x) << " Y" << fmt(mv.y)
                    << " Z" << fmt(mv.z) << " R" << fmt(mv.z + 2.0)
                    << emitF(plunge) << "\n";
                break;
            }
        }
    }

    out << "\n; --- Fin\n";
    out << "G0 Z" << fmt(machine.envelope.zTravel * 0.5) << " ; Dégagement\n";
    out << "M5         ; Arrêt broche\n";
    out << "M9         ; Arrêt arrosage\n";
    out << "G28        ; Retour origine\n";
    out << "M30        ; Fin programme\n";

    return out.str();
}

} // namespace CamEngine
