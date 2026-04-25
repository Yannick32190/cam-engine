#include "FacingOp.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <cmath>

namespace CamEngine {

std::vector<Toolpath> FacingOp::compute(const MachineProfile& machine) {
    m_computed = false;
    m_toolpaths.clear();

    if (m_face.IsNull() && m_shape.IsNull())
        return {};

    // Boîte englobante
    Bnd_Box bbox;
    if (!m_face.IsNull())
        BRepBndLib::Add(m_face, bbox);
    else
        BRepBndLib::Add(m_shape, bbox);

    if (bbox.IsVoid()) return {};

    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double toolR   = m_tool.diameter / 2.0;
    double stepover = m_params.stepover * m_tool.diameter;
    double clearZ  = zmax + m_params.clearanceHeight;
    double safeZ   = zmax + m_params.safeHeight;

    // Passes de profondeur : de (zmax - depthOfCut) vers le bas
    std::vector<double> zLevels;
    double zTarget = zmax - m_params.totalDepth;
    for (double z = zmax - m_params.depthOfCut; z >= zTarget - 1e-6; z -= m_params.depthOfCut)
        zLevels.push_back(std::max(z, zTarget));
    if (zLevels.empty() || std::abs(zLevels.back() - zTarget) > 1e-6)
        zLevels.push_back(zTarget);

    Toolpath tp;
    tp.name = "Surfaçage";
    tp.moves.push_back(rapidTo(xmin - toolR, ymin - toolR, clearZ));

    for (int pass = 0; pass < (int)zLevels.size(); ++pass) {
        double z = zLevels[pass];
        tp.moves.push_back(rapidTo(xmin - toolR, ymin - toolR, safeZ));
        tp.moves.push_back(feedTo(xmin - toolR, ymin - toolR, z, 0)); // plonge feed

        double y = ymin - toolR;
        int row = 0;
        while (y <= ymax + toolR) {
            double xStart = (row % 2 == 0) ? xmin - toolR : xmax + toolR;
            double xEnd   = (row % 2 == 0) ? xmax + toolR : xmin - toolR;
            tp.moves.push_back(feedTo(xStart, y, z));
            tp.moves.push_back(feedTo(xEnd,   y, z));
            y += stepover;
            ++row;
        }
        tp.moves.push_back(rapidTo(tp.moves.back().x, tp.moves.back().y, safeZ));
    }

    tp.moves.push_back(rapidTo(tp.moves.back().x, tp.moves.back().y, clearZ));
    tp.isComputed = true;
    m_toolpaths.push_back(std::move(tp));
    m_computed = true;
    return m_toolpaths;
}

} // namespace CamEngine
