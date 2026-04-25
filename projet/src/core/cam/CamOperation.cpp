#include "CamOperation.h"

namespace CamEngine {

CamOperation::CamOperation(OperationType type)
    : m_type(type)
    , m_name(opTypeName(type))
{}

ToolMove CamOperation::rapidTo(double x, double y, double z) const {
    ToolMove mv;
    mv.type = MoveType::Rapid;
    mv.x = x; mv.y = y; mv.z = z;
    return mv;
}

ToolMove CamOperation::feedTo(double x, double y, double z, double f) const {
    ToolMove mv;
    mv.type = MoveType::Feed;
    mv.x = x; mv.y = y; mv.z = z;
    mv.feedRate = f;
    return mv;
}

ToolMove CamOperation::plungeTo(double z, double f) const {
    ToolMove mv;
    mv.type = MoveType::Feed;
    mv.x = 0; mv.y = 0; mv.z = z;  // x/y inchangés — le caller positionne avant
    mv.feedRate = f;
    return mv;
}

} // namespace CamEngine
