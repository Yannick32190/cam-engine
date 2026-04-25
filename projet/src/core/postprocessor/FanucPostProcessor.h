#pragma once
#include "PostProcessor.h"

namespace CamEngine {

class FanucPostProcessor : public PostProcessor {
public:
    std::string id()            const override { return "fanuc"; }
    std::string name()          const override { return "Fanuc / Haas (ISO)"; }
    std::string description()   const override { return "Fanuc 0i/18i/21i et Haas — format ISO standard"; }
    std::string fileExtension() const override { return "tap"; }

    std::string generate(
        const std::vector<Toolpath>& paths,
        const Tool&                  tool,
        const MachineProfile&        machine,
        const std::string&           programName) const override;
};

} // namespace CamEngine
