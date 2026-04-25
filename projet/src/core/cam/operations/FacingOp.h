#pragma once
#include "../CamOperation.h"

namespace CamEngine {

// Surfaçage : balayage raster zig-zag sur la face à l'altitude Z
class FacingOp : public CamOperation {
public:
    FacingOp() : CamOperation(OperationType::Facing) {}
    std::vector<Toolpath> compute(const MachineProfile& machine) override;
};

} // namespace CamEngine
