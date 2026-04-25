#include "DrillingOp.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

namespace CamEngine {

std::vector<Toolpath> DrillingOp::compute(const MachineProfile& machine) {
    m_computed = false;
    m_toolpaths.clear();

    // Déterminer la surface de référence Z
    double zTop    = 0.0;
    double clearZ  = m_params.clearanceHeight;
    double safeZ   = m_params.safeHeight;
    double drillZ  = -m_params.totalDepth;  // relatif à zTop

    if (!m_face.IsNull()) {
        Bnd_Box bbox;
        BRepBndLib::Add(m_face, bbox);
        if (!bbox.IsVoid()) {
            double xmin,ymin,zmin,xmax,ymax,zmax;
            bbox.Get(xmin,ymin,zmin,xmax,ymax,zmax);
            zTop   = zmax;
            drillZ = zmax - m_params.totalDepth;
            clearZ = zmax + m_params.clearanceHeight;
            safeZ  = zmax + m_params.safeHeight;
        }
    }

    // Si pas de points spécifiés, extraire les cercles de la face
    auto points = m_drillPoints;
    if (points.empty() && !m_face.IsNull()) {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(m_face, props);
        gp_Pnt c = props.CentreOfMass();
        points.emplace_back(c.X(), c.Y());
    }
    if (points.empty()) return {};

    Toolpath tp;
    tp.name = "Perçage";
    tp.moves.push_back(rapidTo(points[0].first, points[0].second, clearZ));

    for (const auto& [px, py] : points) {
        tp.moves.push_back(rapidTo(px, py, safeZ));
        // Cycle de perçage
        ToolMove drill;
        drill.type = MoveType::DrillCycle;
        drill.x = px; drill.y = py;
        drill.z = drillZ;
        tp.moves.push_back(drill);
    }

    tp.moves.push_back(rapidTo(points.back().first, points.back().second, clearZ));
    tp.isComputed = true;
    m_toolpaths.push_back(std::move(tp));
    m_computed = true;
    return m_toolpaths;
}

} // namespace CamEngine
