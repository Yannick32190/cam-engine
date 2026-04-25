#pragma once
#include "PostProcessor.h"

namespace CamEngine {

class LinuxCNCPostProcessor : public PostProcessor {
public:
    std::string id()            const override { return "linuxcnc"; }
    std::string name()          const override { return "LinuxCNC / EMC2"; }
    std::string description()   const override { return "LinuxCNC (EMC2) — RS274/NGC avec changeur d'outil"; }
    std::string fileExtension() const override { return "ngc"; }

    std::string generate(
        const std::vector<Toolpath>& paths,
        const Tool&                  tool,
        const MachineProfile&        machine,
        const std::string&           programName) const override;
};

} // namespace CamEngine
