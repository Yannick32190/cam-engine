#include "PocketOp.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <cmath>
#include <iostream>

namespace CamEngine {

static std::vector<std::pair<double,double>> wirePoints(
    const TopoDS_Wire& wire, double tol)
{
    std::vector<std::pair<double,double>> pts;
    TopExp_Explorer exp(wire, TopAbs_EDGE);
    for (; exp.More(); exp.Next()) {
        BRepAdaptor_Curve curve(TopoDS::Edge(exp.Current()));
        GCPnts_UniformAbscissa disc;
        disc.Initialize(curve, tol);
        if (!disc.IsDone()) continue;
        for (int i = 1; i <= disc.NbPoints(); ++i) {
            gp_Pnt p = curve.Value(disc.Parameter(i));
            pts.emplace_back(p.X(), p.Y());
        }
    }
    return pts;
}

std::vector<Toolpath> PocketOp::compute(const MachineProfile& machine) {
    m_computed = false;
    m_toolpaths.clear();

    if (m_face.IsNull()) return {};

    Bnd_Box bbox;
    BRepBndLib::Add(m_face, bbox);
    if (bbox.IsVoid()) return {};
    double xmin,ymin,zmin,xmax,ymax,zmax;
    bbox.Get(xmin,ymin,zmin,xmax,ymax,zmax);

    double toolD    = m_tool.diameter;
    double stepover = m_params.stepover * toolD;
    double clearZ   = zmax + m_params.clearanceHeight;
    double safeZ    = zmax + m_params.safeHeight;
    double tol      = m_params.tolerance;

    std::vector<double> zLevels;
    double zTarget = zmax - m_params.totalDepth;
    for (double z = zmax - m_params.depthOfCut; z >= zTarget - 1e-6; z -= m_params.depthOfCut)
        zLevels.push_back(std::max(z, zTarget));
    if (zLevels.empty() || std::abs(zLevels.back() - zTarget) > 1e-6)
        zLevels.push_back(zTarget);

    TopExp_Explorer wExp(m_face, TopAbs_WIRE);
    if (!wExp.More()) return {};
    TopoDS_Wire outerWire = TopoDS::Wire(wExp.Current());

    Toolpath tp;
    tp.name = "Poche";

    // Centroïde de la face → point de départ/plongée
    GProp_GProps props;
    BRepGProp::SurfaceProperties(m_face, props);
    gp_Pnt centroid = props.CentreOfMass();
    tp.moves.push_back(rapidTo(centroid.X(), centroid.Y(), clearZ));

    for (double z : zLevels) {
        // Offsets progressifs de -stepover/2 jusqu'à -grand nombre (inward)
        tp.moves.push_back(rapidTo(centroid.X(), centroid.Y(), safeZ));
        tp.moves.push_back(feedTo(centroid.X(), centroid.Y(), z, 0));

        // Construire les passes concentriques de l'extérieur vers l'intérieur
        std::vector<std::vector<std::pair<double,double>>> passes;
        for (double off = -(toolD / 2.0); off > -(xmax - xmin); off -= stepover) {
            try {
                BRepOffsetAPI_MakeOffset offsetMaker(outerWire, GeomAbs_Arc);
                offsetMaker.Perform(off);
                if (!offsetMaker.IsDone()) break;
                auto offsetShape = offsetMaker.Shape();
                TopExp_Explorer owExp(offsetShape, TopAbs_WIRE);
                if (!owExp.More()) break;
                auto pts = wirePoints(TopoDS::Wire(owExp.Current()), tol);
                if (pts.empty()) break;
                passes.push_back(pts);
            } catch (...) { break; }
        }

        // Parcourir de l'intérieur vers l'extérieur (ou l'inverse selon climb)
        if (!m_params.climb)
            std::reverse(passes.begin(), passes.end());

        for (const auto& pts : passes) {
            if (pts.empty()) continue;
            tp.moves.push_back(rapidTo(pts[0].first, pts[0].second, safeZ));
            tp.moves.push_back(feedTo(pts[0].first, pts[0].second, z, 0));
            for (size_t i = 1; i < pts.size(); ++i)
                tp.moves.push_back(feedTo(pts[i].first, pts[i].second, z));
            tp.moves.push_back(feedTo(pts[0].first, pts[0].second, z));
        }
        tp.moves.push_back(rapidTo(tp.moves.back().x, tp.moves.back().y, safeZ));
    }

    tp.moves.push_back(rapidTo(centroid.X(), centroid.Y(), clearZ));
    tp.isComputed = true;
    m_toolpaths.push_back(std::move(tp));
    m_computed = true;
    return m_toolpaths;
}

} // namespace CamEngine
