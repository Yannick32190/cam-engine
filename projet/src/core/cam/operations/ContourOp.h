#pragma once
#include "../CamOperation.h"

namespace CamEngine {

// Contournage 2.5D : suit le profil extérieur de la face
class ContourOp : public CamOperation {
public:
    ContourOp() : CamOperation(OperationType::Contour) {}
    std::vector<Toolpath> compute(const MachineProfile& machine) override;
};

} // namespace CamEngine
