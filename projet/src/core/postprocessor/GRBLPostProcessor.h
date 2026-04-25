#pragma once
#include "PostProcessor.h"

namespace CamEngine {

// Post-processeur GRBL / Arduino CNC Shield
// Compatible GRBL 1.1 et dérivés (Estlcam, Candle, UGS…)
class GRBLPostProcessor : public PostProcessor {
public:
    std::string id()            const override { return "grbl"; }
    std::string name()          const override { return "GRBL 1.1 (Arduino)"; }
    std::string description()   const override { return "GRBL 1.1 — fraiseuses/routeurs DIY Arduino"; }
    std::string fileExtension() const override { return "nc"; }

    std::string generate(
        const std::vector<Toolpath>& paths,
        const Tool&                  tool,
        const MachineProfile&        machine,
        const std::string&           programName) const override;
};

} // namespace CamEngine
