#include "ContourOp.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <cmath>
#include <iostream>

namespace CamEngine {

static std::vector<std::pair<double,double>> discretizeWire(
    const TopoDS_Wire& wire, double tolerance)
{
    std::vector<std::pair<double,double>> pts;
    TopExp_Explorer exp(wire, TopAbs_EDGE);
    for (; exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        try {
            BRepAdaptor_Curve curve(edge);
            GCPnts_UniformAbscissa disc;
            disc.Initialize(curve, tolerance);
            if (!disc.IsDone()) continue;
            for (int i = 1; i <= disc.NbPoints(); ++i) {
                gp_Pnt p = curve.Value(disc.Parameter(i));
                pts.emplace_back(p.X(), p.Y());
            }
        } catch (...) {}
    }
    return pts;
}

std::vector<Toolpath> ContourOp::compute(const MachineProfile& machine) {
    m_computed = false;
    m_toolpaths.clear();

    if (m_face.IsNull()) return {};

    Bnd_Box bbox;
    BRepBndLib::Add(m_face, bbox);
    if (bbox.IsVoid()) return {};
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double toolR    = m_tool.diameter / 2.0;
    double offset   = toolR + m_params.finishAllowance;
    double clearZ   = zmax + m_params.clearanceHeight;
    double safeZ    = zmax + m_params.safeHeight;
    double tolerance= m_params.tolerance;

    std::vector<double> zLevels;
    double zTarget = zmax - m_params.totalDepth;
    for (double z = zmax - m_params.depthOfCut; z >= zTarget - 1e-6; z -= m_params.depthOfCut)
        zLevels.push_back(std::max(z, zTarget));
    if (zLevels.empty() || std::abs(zLevels.back() - zTarget) > 1e-6)
        zLevels.push_back(zTarget);

    // Obtenir le wire extérieur de la face
    TopExp_Explorer wExp(m_face, TopAbs_WIRE);
    if (!wExp.More()) return {};
    TopoDS_Wire outerWire = TopoDS::Wire(wExp.Current());

    Toolpath tp;
    tp.name = "Contournage";
    tp.moves.push_back(rapidTo(xmin, ymin, clearZ));

    for (double z : zLevels) {
        // Offset du wire par le rayon outil (côté extérieur)
        try {
            BRepOffsetAPI_MakeOffset offsetMaker(outerWire, GeomAbs_Arc);
            offsetMaker.Perform(offset);
            if (!offsetMaker.IsDone()) {
                // Fallback: utiliser le wire original
                auto pts = discretizeWire(outerWire, tolerance);
                if (pts.empty()) continue;
                tp.moves.push_back(rapidTo(pts[0].first, pts[0].second, safeZ));
                tp.moves.push_back(feedTo(pts[0].first, pts[0].second, z, 0));
                for (size_t i = 1; i < pts.size(); ++i)
                    tp.moves.push_back(feedTo(pts[i].first, pts[i].second, z));
                tp.moves.push_back(feedTo(pts[0].first, pts[0].second, z));
                tp.moves.push_back(rapidTo(pts[0].first, pts[0].second, safeZ));
                continue;
            }
            auto offsetShape = offsetMaker.Shape();
            TopExp_Explorer owExp(offsetShape, TopAbs_WIRE);
            if (!owExp.More()) continue;
            TopoDS_Wire offsetWire = TopoDS::Wire(owExp.Current());
            auto pts = discretizeWire(offsetWire, tolerance);
            if (pts.empty()) continue;
            tp.moves.push_back(rapidTo(pts[0].first, pts[0].second, safeZ));
            tp.moves.push_back(feedTo(pts[0].first, pts[0].second, z, 0));
            for (size_t i = 1; i < pts.size(); ++i)
                tp.moves.push_back(feedTo(pts[i].first, pts[i].second, z));
            tp.moves.push_back(feedTo(pts[0].first, pts[0].second, z));
            tp.moves.push_back(rapidTo(pts[0].first, pts[0].second, safeZ));
        } catch (const std::exception& e) {
            std::cerr << "[ContourOp] " << e.what() << "\n";
        } catch (...) {}
    }

    tp.moves.push_back(rapidTo(tp.moves.back().x, tp.moves.back().y, clearZ));
    tp.isComputed = true;
    m_toolpaths.push_back(std::move(tp));
    m_computed = true;
    return m_toolpaths;
}

} // namespace CamEngine
