#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "../cam/Toolpath.h"
#include "../machine/MachineProfile.h"

namespace CamEngine {

// ─── Interface abstraite pour les post-processeurs ───────────────────────────
//
// Pour créer un nouveau post-processeur :
//  1. Hériter de PostProcessor
//  2. Implémenter les 5 méthodes virtuelles pures
//  3. L'enregistrer dans PostProcessorRegistry::registerAll()
//
class PostProcessor {
public:
    virtual ~PostProcessor() = default;

    virtual std::string id()            const = 0;  // identifiant unique ("grbl", "fanuc", …)
    virtual std::string name()          const = 0;  // nom affiché
    virtual std::string description()   const = 0;
    virtual std::string fileExtension() const = 0;  // "nc", "tap", "gcode", …

    virtual std::string generate(
        const std::vector<Toolpath>& paths,
        const Tool&                  tool,
        const MachineProfile&        machine,
        const std::string&           programName = "O0001") const = 0;

protected:
    // Utilitaires communs à tous les post-processeurs
    static std::string fmt(double v, int decimals = 3) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(decimals) << v;
        return ss.str();
    }
    static std::string coord(char axis, double v, int dec = 3) {
        return std::string(1, axis) + fmt(v, dec);
    }
    static std::string feedWord(double f, int dec = 0) {
        return "F" + fmt(f, dec);
    }
};

} // namespace CamEngine
