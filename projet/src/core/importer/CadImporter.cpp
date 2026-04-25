#include "CadImporter.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <iostream>

// OCCT
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Pln.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <gp_Circ.hxx>
#include <GC_MakeCircle.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <Precision.hxx>
#include <cmath>

namespace CamEngine {

// ─── Utilitaires ─────────────────────────────────────────────────────────────

static gp_Pln makePlane(const QJsonObject& params) {
    double nx = params["plane_normal_x"].toObject()["value"].toDouble(0);
    double ny = params["plane_normal_y"].toObject()["value"].toDouble(0);
    double nz = params["plane_normal_z"].toObject()["value"].toDouble(1);
    double ox = params["plane_origin_x"].toObject()["value"].toDouble(0);
    double oy = params["plane_origin_y"].toObject()["value"].toDouble(0);
    double oz = params["plane_origin_z"].toObject()["value"].toDouble(0);
    double xx = params["plane_xdir_x"].toObject()["value"].toDouble(1);
    double xy = params["plane_xdir_y"].toObject()["value"].toDouble(0);
    double xz = params["plane_xdir_z"].toObject()["value"].toDouble(0);
    gp_Dir normal(nx, ny, nz);
    gp_Dir xDir(xx, xy, xz);
    gp_Pnt origin(ox, oy, oz);
    return gp_Pln(gp_Ax3(origin, normal, xDir));
}

// Reconstruit une face plane depuis les entités d'un sketch
static TopoDS_Face buildFaceFromSketch(const QJsonObject& sketchObj) {
    const QJsonArray entities = sketchObj["entities"].toArray();
    if (entities.isEmpty()) return TopoDS_Face();

    gp_Pln plane = makePlane(sketchObj["parameters"].toObject());

    Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();

    for (const auto& ev : entities) {
        QJsonObject e = ev.toObject();
        QString type = e["type"].toString();

        try {
            if (type == "Circle") {
                double cx = e["cx"].toDouble(), cy = e["cy"].toDouble();
                double r  = e["radius"].toDouble();
                gp_Pnt center3D;
                gp_Vec du(plane.XAxis().Direction());
                gp_Vec dv(plane.YAxis().Direction());
                center3D = plane.Location().Translated(du * cx + dv * cy);
                gp_Circ circ(gp_Ax2(center3D, plane.Axis().Direction()), r);
                BRepBuilderAPI_MakeEdge em(circ);
                if (em.IsDone()) edgeSeq->Append(em.Edge());
            }
            else if (type == "Rectangle") {
                double cx = e["cx"].toDouble(), cy = e["cy"].toDouble();
                double w  = e["width"].toDouble(), h = e["height"].toDouble();
                gp_Vec du(plane.XAxis().Direction());
                gp_Vec dv(plane.YAxis().Direction());
                gp_Pnt p00 = plane.Location().Translated(du * cx       + dv * cy);
                gp_Pnt p10 = plane.Location().Translated(du * (cx + w) + dv * cy);
                gp_Pnt p11 = plane.Location().Translated(du * (cx + w) + dv * (cy + h));
                gp_Pnt p01 = plane.Location().Translated(du * cx       + dv * (cy + h));
                auto addSeg = [&](const gp_Pnt& a, const gp_Pnt& b) {
                    BRepBuilderAPI_MakeEdge em(a, b);
                    if (em.IsDone()) edgeSeq->Append(em.Edge());
                };
                addSeg(p00, p10); addSeg(p10, p11);
                addSeg(p11, p01); addSeg(p01, p00);
            }
            else if (type == "Line") {
                double x1 = e["x1"].toDouble(), y1 = e["y1"].toDouble();
                double x2 = e["x2"].toDouble(), y2 = e["y2"].toDouble();
                gp_Vec du(plane.XAxis().Direction());
                gp_Vec dv(plane.YAxis().Direction());
                gp_Pnt p1 = plane.Location().Translated(du * x1 + dv * y1);
                gp_Pnt p2 = plane.Location().Translated(du * x2 + dv * y2);
                if (p1.Distance(p2) > Precision::Confusion()) {
                    BRepBuilderAPI_MakeEdge em(p1, p2);
                    if (em.IsDone()) edgeSeq->Append(em.Edge());
                }
            }
        } catch (...) {}
    }

    if (edgeSeq->IsEmpty()) return TopoDS_Face();

    // Connecter les arêtes en wire(s)
    Handle(TopTools_HSequenceOfShape) wires = new TopTools_HSequenceOfShape();
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 0.5, Standard_False, wires);

    for (int i = 1; i <= wires->Length(); ++i) {
        TopoDS_Wire w = TopoDS::Wire(wires->Value(i));
        if (w.IsNull()) continue;
        try {
            BRepBuilderAPI_MakeFace fm(plane, w, Standard_True);
            if (fm.IsDone()) {
                TopoDS_Face face = fm.Face();
                if (face.Orientation() == TopAbs_REVERSED)
                    face = TopoDS::Face(face.Reversed());
                return face;
            }
        } catch (...) {}
    }
    return TopoDS_Face();
}

// ─── Appliquer une opération booléenne ───────────────────────────────────────

static TopoDS_Shape applyBool(
    const TopoDS_Shape& body,
    const TopoDS_Shape& tool,
    int opCode)  // 0=NewBody, 1=Join, 2=Cut, 3=Intersect
{
    if (body.IsNull() || opCode == 0) return tool;
    try {
        if (opCode == 1) {
            BRepAlgoAPI_Fuse fuse(body, tool);
            fuse.Build();
            if (fuse.IsDone()) return fuse.Shape();
        } else if (opCode == 2) {
            BRepAlgoAPI_Cut cut(body, tool);
            cut.Build();
            if (cut.IsDone()) return cut.Shape();
            return body;
        } else if (opCode == 3) {
            BRepAlgoAPI_Common common(body, tool);
            common.Build();
            if (common.IsDone()) return common.Shape();
        }
    } catch (...) {}
    return tool;
}

// ─── CadImporter::import ─────────────────────────────────────────────────────

TopoDS_Shape CadImporter::import(const QString& filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        std::cerr << "[CadImporter] Impossible d'ouvrir : "
                  << filePath.toStdString() << "\n";
        return TopoDS_Shape();
    }

    QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    QJsonArray features = root["features"].toArray();

    // Table sketch_id → sketchObj (pour résolution des dépendances)
    QMap<int, QJsonObject> sketchMap;
    TopoDS_Shape currentBody;

    for (const auto& fv : features) {
        QJsonObject feat = fv.toObject();
        QString ftype = feat["type"].toString();
        int fid = feat["id"].toInt();

        if (ftype == "Sketch") {
            sketchMap[fid] = feat;
            continue;
        }

        if (ftype == "Extrude") {
            QJsonObject params = feat["parameters"].toObject();
            int sketchId = params["sketch_id"].toObject()["value"].toInt(-1);
            double dist1 = params["distance"].toObject()["value"].toDouble(10);
            double dist2 = params["distance2"].toObject()["value"].toDouble(10);
            int direction = params["direction"].toObject()["value"].toInt(0); // 0=OneSide,1=Symmetric,2=TwoSides
            int operation = params["operation"].toObject()["value"].toInt(0); // 0=New,1=Join,2=Cut,3=Intersect

            if (!sketchMap.contains(sketchId)) continue;
            TopoDS_Face face = buildFaceFromSketch(sketchMap[sketchId]);
            if (face.IsNull()) continue;

            gp_Pln plane = makePlane(sketchMap[sketchId]["parameters"].toObject());
            gp_Dir normal = plane.Axis().Direction();

            try {
                TopoDS_Shape extruded;
                if (direction == 0) { // OneSide
                    gp_Vec vec(normal); vec.Scale(dist1);
                    BRepPrimAPI_MakePrism prism(face, vec);
                    prism.Build();
                    if (prism.IsDone()) extruded = prism.Shape();
                } else if (direction == 1) { // Symmetric
                    gp_Vec vecNeg(normal); vecNeg.Scale(-dist1 / 2.0);
                    gp_Trsf trsf; trsf.SetTranslation(vecNeg);
                    TopoDS_Face moved = TopoDS::Face(face.Moved(TopLoc_Location(trsf)));
                    gp_Vec vecFull(normal); vecFull.Scale(dist1);
                    BRepPrimAPI_MakePrism prism(moved, vecFull);
                    prism.Build();
                    if (prism.IsDone()) extruded = prism.Shape();
                } else { // TwoSides
                    gp_Vec v1(normal); v1.Scale(dist1);
                    gp_Vec v2(normal); v2.Scale(-dist2);
                    BRepPrimAPI_MakePrism p1(face, v1), p2(face, v2);
                    p1.Build(); p2.Build();
                    if (p1.IsDone() && p2.IsDone()) {
                        BRepAlgoAPI_Fuse fuse(p1.Shape(), p2.Shape());
                        fuse.Build();
                        if (fuse.IsDone()) extruded = fuse.Shape();
                    } else if (p1.IsDone()) extruded = p1.Shape();
                }
                if (!extruded.IsNull())
                    currentBody = applyBool(currentBody, extruded, operation);
            } catch (...) {}
            continue;
        }

        if (ftype == "Revolve") {
            QJsonObject params = feat["parameters"].toObject();
            int sketchId = params["sketch_id"].toObject()["value"].toInt(-1);
            double angleDeg = params["angle"].toObject()["value"].toDouble(360);
            int axisType = params["axis_type"].toObject()["value"].toInt(0);
            int operation = params["operation"].toObject()["value"].toInt(0);

            if (!sketchMap.contains(sketchId)) continue;
            TopoDS_Face face = buildFaceFromSketch(sketchMap[sketchId]);
            if (face.IsNull()) continue;

            gp_Pln plane = makePlane(sketchMap[sketchId]["parameters"].toObject());
            gp_Ax1 axis;
            if (axisType == 0)      axis = gp_Ax1(plane.Location(), plane.XAxis().Direction());
            else if (axisType == 1) axis = gp_Ax1(plane.Location(), plane.YAxis().Direction());
            else if (axisType == 2) axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(1,0,0));
            else                    axis = gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,1,0));

            try {
                double angleRad = angleDeg * M_PI / 180.0;
                BRepPrimAPI_MakeRevol revol(face, axis,
                    std::abs(angleDeg - 360.0) < 0.01 ? 2.0 * M_PI : angleRad);
                revol.Build();
                if (revol.IsDone())
                    currentBody = applyBool(currentBody, revol.Shape(), operation);
            } catch (...) {}
            continue;
        }

        if (ftype == "Fillet3D") {
            QJsonObject params = feat["parameters"].toObject();
            double radius = params["radius"].toObject()["value"].toDouble(1.0);
            if (currentBody.IsNull()) continue;
            try {
                BRepFilletAPI_MakeFillet fillet(currentBody);
                TopExp_Explorer edgeExp(currentBody, TopAbs_EDGE);
                for (; edgeExp.More(); edgeExp.Next())
                    fillet.Add(radius, TopoDS::Edge(edgeExp.Current()));
                fillet.Build();
                if (fillet.IsDone()) currentBody = fillet.Shape();
            } catch (...) {}
            continue;
        }

        if (ftype == "Chamfer3D") {
            QJsonObject params = feat["parameters"].toObject();
            double dist = params["distance"].toObject()["value"].toDouble(1.0);
            if (currentBody.IsNull()) continue;
            try {
                BRepFilletAPI_MakeChamfer chamfer(currentBody);
                TopExp_Explorer edgeExp(currentBody, TopAbs_EDGE);
                for (; edgeExp.More(); edgeExp.Next())
                    chamfer.Add(dist, TopoDS::Edge(edgeExp.Current()));
                chamfer.Build();
                if (chamfer.IsDone()) currentBody = chamfer.Shape();
            } catch (...) {}
            continue;
        }

        // Features non gérées (Shell, PushPull, Pattern, …) : passer silencieusement
        std::cout << "[CadImporter] Feature ignorée : " << ftype.toStdString() << "\n";
    }

    return currentBody;
}

} // namespace CamEngine
