#pragma once
#include "PostProcessor.h"

namespace CamEngine {

class Mach4PostProcessor : public PostProcessor {
public:
    std::string id()            const override { return "mach4"; }
    std::string name()          const override { return "Mach3 / Mach4"; }
    std::string description()   const override { return "Mach3 / Mach4 — CNC Artsoft, format .tap/.nc"; }
    std::string fileExtension() const override { return "tap"; }

    std::string generate(
        const std::vector<Toolpath>& paths,
        const Tool&                  tool,
        const MachineProfile&        machine,
        const std::string&           programName) const override;
};

} // namespace CamEngine
