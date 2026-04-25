#include "FanucPostProcessor.h"
#include <sstream>
#include <cmath>

namespace CamEngine {

std::string FanucPostProcessor::generate(
    const std::vector<Toolpath>& paths,
    const Tool&                  tool,
    const MachineProfile&        machine,
    const std::string&           programName) const
{
    std::ostringstream out;
    double rpm    = std::min(tool.maxRPM * 0.75, machine.spindle.maxRPM);
    double feed   = std::min(tool.feedRate(rpm), machine.maxFeedRate);
    double plunge = std::min(tool.plungeFeedRate(rpm), machine.maxPlungeRate);

    // Numéro de programme Fanuc (OXXXX)
    std::string progNum = programName;
    if (progNum.empty() || progNum[0] != 'O') progNum = "O" + programName;

    out << progNum << "\n";
    out << "(CAM-ENGINE — post Fanuc/Haas)\n";
    out << "(OUTIL T" << tool.id << " : " << tool.name << ")\n";
    out << "(DIAM : " << fmt(tool.diameter) << "MM)\n\n";

    out << "N10 G21 G17 G90 G94\n";
    out << "N20 T" << tool.id << " M6\n";
    out << "N30 G43 H" << tool.id << "\n";
    out << "N40 S" << static_cast<int>(rpm) << " M3\n";
    out << "N50 G4 P1.\n\n";

    int nLine = 60;
    double lastX = 1e30, lastY = 1e30, lastZ = 1e30, lastF = -1;
    bool motionStarted = false;

    for (const auto& tp : paths) {
        if (!tp.isComputed) continue;
        out << "(" << tp.name << ")\n";
        for (const auto& mv : tp.moves) {
            if (!mv.comment.empty()) out << "(" << mv.comment << ")\n";
            out << "N" << nLine << " ";
            nLine += 10;
            switch (mv.type) {
            case MoveType::Rapid:
                out << "G0";
                if (std::abs(mv.x - lastX) > 1e-4) { out << "X" << fmt(mv.x); lastX = mv.x; }
                if (std::abs(mv.y - lastY) > 1e-4) { out << "Y" << fmt(mv.y); lastY = mv.y; }
                if (std::abs(mv.z - lastZ) > 1e-4) { out << "Z" << fmt(mv.z); lastZ = mv.z; }
                out << "\n"; motionStarted = true; break;
            case MoveType::Feed: {
                double f = (mv.feedRate > 0) ? mv.feedRate
                         : (!motionStarted || std::abs(mv.z - lastZ) > 1e-4) ? plunge : feed;
                out << "G1";
                if (std::abs(mv.x - lastX) > 1e-4) { out << "X" << fmt(mv.x); lastX = mv.x; }
                if (std::abs(mv.y - lastY) > 1e-4) { out << "Y" << fmt(mv.y); lastY = mv.y; }
                if (std::abs(mv.z - lastZ) > 1e-4) { out << "Z" << fmt(mv.z); lastZ = mv.z; }
                if (std::abs(f - lastF) > 0.1) { out << "F" << fmt(f, 0); lastF = f; }
                out << "\n"; motionStarted = true; break;
            }
            case MoveType::ArcCW:
            case MoveType::ArcCCW:
                out << (mv.type == MoveType::ArcCW ? "G2" : "G3");
                out << "X" << fmt(mv.x) << "Y" << fmt(mv.y);
                out << "I" << fmt(mv.i) << "J" << fmt(mv.j);
                if (std::abs(feed - lastF) > 0.1) { out << "F" << fmt(feed, 0); lastF = feed; }
                out << "\n"; lastX = mv.x; lastY = mv.y; break;
            case MoveType::Dwell:
                out << "G4 P" << fmt(mv.dwellSec * 1000, 0) << "\n"; break;
            case MoveType::DrillCycle:
                out << "G81 X" << fmt(mv.x) << "Y" << fmt(mv.y)
                    << "Z" << fmt(mv.z) << "R" << fmt(mv.z + 2.0)
                    << "F" << fmt(plunge, 0) << "\n"; break;
            }
        }
    }

    out << "N" << nLine << " G80\n";
    nLine += 10;
    out << "N" << nLine << " M5\n";
    nLine += 10;
    out << "N" << nLine << " M9\n";
    nLine += 10;
    out << "N" << nLine << " G91 G28 Z0.\n";
    nLine += 10;
    out << "N" << nLine << " G28 X0. Y0.\n";
    nLine += 10;
    out << "N" << nLine << " G90\n";
    nLine += 10;
    out << "N" << nLine << " M30\n%\n";

    return out.str();
}

} // namespace CamEngine
