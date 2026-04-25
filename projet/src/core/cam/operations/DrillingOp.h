#pragma once
#include "../CamOperation.h"
#include <vector>

namespace CamEngine {

// Perçage : cycle G81/G83 sur une liste de points
class DrillingOp : public CamOperation {
public:
    DrillingOp() : CamOperation(OperationType::Drilling) {}

    void setDrillPoints(const std::vector<std::pair<double,double>>& pts) {
        m_drillPoints = pts;
    }

    std::vector<Toolpath> compute(const MachineProfile& machine) override;

private:
    std::vector<std::pair<double,double>> m_drillPoints;
};

} // namespace CamEngine
