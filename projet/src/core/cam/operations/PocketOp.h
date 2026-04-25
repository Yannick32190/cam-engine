#pragma once
#include "../CamOperation.h"

namespace CamEngine {

// Vidage de poche : offsets progressifs vers l'intérieur
class PocketOp : public CamOperation {
public:
    PocketOp() : CamOperation(OperationType::Pocket) {}
    std::vector<Toolpath> compute(const MachineProfile& machine) override;
};

} // namespace CamEngine
