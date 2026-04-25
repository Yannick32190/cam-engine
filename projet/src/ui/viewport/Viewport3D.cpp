#include "Viewport3D.h"
#include <QOpenGLFunctions>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

#include <cmath>
#include <algorithm>

namespace CamEngine {

Viewport3D::Viewport3D(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(400, 300);
}

void Viewport3D::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);

    GLfloat light0pos[] = { 1.f, 2.f, 3.f, 0.f };
    GLfloat ambient[]   = { 0.25f, 0.25f, 0.25f, 1.f };
    GLfloat diffuse[]   = { 0.85f, 0.85f, 0.85f, 1.f };
    glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
}

void Viewport3D::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (h > 0) ? (double)w / h : 1.0;
    gluPerspective(45.0, aspect, 0.1, 50000.0);
    glMatrixMode(GL_MODELVIEW);
    m_glValid = false;
}

void Viewport3D::setupCamera() {
    glLoadIdentity();

    // Calculer le centre du modèle
    double cx = 0, cy = 0, cz = 0, r = 100;
    if (!m_shape.IsNull()) {
        Bnd_Box bbox;
        BRepBndLib::Add(m_shape, bbox);
        if (!bbox.IsVoid()) {
            double xmin,ymin,zmin,xmax,ymax,zmax;
            bbox.Get(xmin,ymin,zmin,xmax,ymax,zmax);
            cx = (xmin+xmax)/2; cy = (ymin+ymax)/2; cz = (zmin+zmax)/2;
            r = std::max({xmax-xmin, ymax-ymin, zmax-zmin}) * 0.5;
        }
    }

    double dist = r * 3.0 / m_zoom;
    double azRad  = m_rotZ * M_PI / 180.0;
    double elevRad = m_rotX * M_PI / 180.0;
    double eyeX = cx + dist * cos(elevRad) * sin(azRad);
    double eyeY = cy + dist * cos(elevRad) * (-cos(azRad));
    double eyeZ = cz + dist * sin(elevRad);

    gluLookAt(eyeX, eyeY, eyeZ,
              cx + m_panX, cy + m_panY, cz,
              0, 0, 1);

    glGetDoublev(GL_MODELVIEW_MATRIX,  m_modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, m_projection);
    glGetIntegerv(GL_VIEWPORT, m_viewport);
    m_glValid = true;
}

void Viewport3D::paintGL() {
    // Couleur de fond selon thème (gris sombre par défaut)
    glClearColor(0.17f, 0.18f, 0.19f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    setupCamera();

    drawGrid();
    drawShape();
    if (m_toolpathsVisible) drawToolpaths();
}

void Viewport3D::drawGrid() {
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_LINES);
    for (int i = -100; i <= 100; i += 10) {
        glVertex3d(i, -100, 0); glVertex3d(i, 100, 0);
        glVertex3d(-100, i, 0); glVertex3d(100, i, 0);
    }
    glEnd();
    // Axes
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(0.9f, 0.2f, 0.2f); glVertex3d(0,0,0); glVertex3d(50,0,0);
    glColor3f(0.2f, 0.9f, 0.2f); glVertex3d(0,0,0); glVertex3d(0,50,0);
    glColor3f(0.2f, 0.5f, 0.9f); glVertex3d(0,0,0); glVertex3d(0,0,50);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

void Viewport3D::tessellate() {
    if (m_shape.IsNull()) return;
    try {
        BRepMesh_IncrementalMesh mesher(m_shape, 0.1, false, 0.3, true);
        mesher.Perform();
    } catch (...) {}
    m_tessellated = true;
}

void Viewport3D::drawShape() {
    if (m_shape.IsNull()) return;
    if (!m_tessellated) tessellate();

    glEnable(GL_LIGHTING);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_POLYGON_OFFSET_FILL);

    TopExp_Explorer faceExp(m_shape, TopAbs_FACE);
    for (; faceExp.More(); faceExp.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExp.Current());
        bool picked = (!m_pickedFace.IsNull() && face.IsEqual(m_pickedFace));

        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;

        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
        bool rev = (face.Orientation() == TopAbs_REVERSED);

        if (picked)
            glColor3f(0.2f, 0.7f, 1.0f);
        else
            glColor3f(0.55f, 0.65f, 0.75f);

        glBegin(GL_TRIANGLES);
        for (int t = 1; t <= tri->NbTriangles(); ++t) {
            int n1, n2, n3;
            tri->Triangle(t).Get(n1, n2, n3);
            if (rev) std::swap(n2, n3);

            auto pt = [&](int n) {
                gp_Pnt p = tri->Node(n);
                if (!loc.IsIdentity()) p.Transform(trsf);
                return p;
            };
            gp_Pnt p1 = pt(n1), p2 = pt(n2), p3 = pt(n3);
            gp_Vec normal = gp_Vec(p1, p2).Crossed(gp_Vec(p1, p3));
            if (normal.Magnitude() > 1e-10) {
                normal.Normalize();
                glNormal3d(normal.X(), normal.Y(), normal.Z());
            }
            glVertex3d(p1.X(), p1.Y(), p1.Z());
            glVertex3d(p2.X(), p2.Y(), p2.Z());
            glVertex3d(p3.X(), p3.Y(), p3.Z());
        }
        glEnd();
    }

    // Wireframe par-dessus
    glDisable(GL_LIGHTING);
    glPolygonOffset(0, 0);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glColor3f(0.1f, 0.15f, 0.2f);
    glLineWidth(0.8f);

    TopExp_Explorer edgeExp(m_shape, TopAbs_EDGE);
    for (; edgeExp.More(); edgeExp.Next()) {
        // Dessiner arêtes (via tessellation)
    }

    glEnable(GL_LIGHTING);
}

void Viewport3D::drawToolpaths() {
    glDisable(GL_LIGHTING);
    glLineWidth(1.5f);

    for (const auto& tpd : m_toolpaths) {
        if (!tpd.visible) continue;
        glColor3f(tpd.r, tpd.g, tpd.b);
        for (const auto& tp : tpd.paths) {
            if (!tp.isComputed) continue;
            bool inLine = false;
            for (const auto& mv : tp.moves) {
                if (mv.type == MoveType::Rapid) {
                    if (inLine) { glEnd(); inLine = false; }
                    glLineWidth(0.6f);
                    glColor3f(tpd.r * 0.5f, tpd.g * 0.5f, tpd.b * 0.5f);
                } else if (mv.type == MoveType::Feed) {
                    if (!inLine) {
                        glLineWidth(1.5f);
                        glColor3f(tpd.r, tpd.g, tpd.b);
                        glBegin(GL_LINE_STRIP);
                        inLine = true;
                    }
                    glVertex3d(mv.x, mv.y, mv.z);
                } else {
                    if (inLine) { glEnd(); inLine = false; }
                }
            }
            if (inLine) glEnd();
        }
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// ─── Événements souris ────────────────────────────────────────────────────────

void Viewport3D::mousePressEvent(QMouseEvent* e) {
    m_lastPos = e->pos();
    if (e->button() == Qt::LeftButton) {
        // Sélection de face
        makeCurrent();
        TopoDS_Face face = pickFace(e->pos().x(), e->pos().y());
        if (!face.IsNull()) {
            m_pickedFace = face;
            emit faceClicked(face);
            update();
        }
        m_rotating = false;
    } else if (e->button() == Qt::RightButton) {
        m_rotating = true;
    } else if (e->button() == Qt::MiddleButton) {
        m_panning = true;
    }
}

void Viewport3D::mouseMoveEvent(QMouseEvent* e) {
    int dx = e->pos().x() - m_lastPos.x();
    int dy = e->pos().y() - m_lastPos.y();
    m_lastPos = e->pos();
    if (m_rotating) {
        m_rotZ += dx * 0.5;
        m_rotX -= dy * 0.5;
        m_rotX = std::max(-89.0, std::min(89.0, m_rotX));
        update();
    } else if (m_panning) {
        m_panX -= dx * 0.3 / m_zoom;
        m_panY += dy * 0.3 / m_zoom;
        update();
    }
}

void Viewport3D::mouseReleaseEvent(QMouseEvent*) {
    m_rotating = m_panning = false;
}

void Viewport3D::wheelEvent(QWheelEvent* e) {
    double delta = e->angleDelta().y() / 120.0;
    m_zoom *= std::pow(1.15, delta);
    m_zoom = std::max(0.01, std::min(m_zoom, 200.0));
    update();
}

// ─── Sélection de face ────────────────────────────────────────────────────────

TopoDS_Face Viewport3D::pickFace(int sx, int sy) {
    if (m_shape.IsNull() || !m_glValid) return TopoDS_Face();

    gp_Pnt rayOrigin;
    gp_Dir rayDir;

    // Unproject deux points (near et far)
    double wx1, wy1, wz1, wx2, wy2, wz2;
    int h = height();
    gluUnProject(sx, h - sy, 0.0, m_modelview, m_projection, m_viewport, &wx1, &wy1, &wz1);
    gluUnProject(sx, h - sy, 1.0, m_modelview, m_projection, m_viewport, &wx2, &wy2, &wz2);
    rayOrigin = gp_Pnt(wx1, wy1, wz1);
    gp_Vec rd(wx2-wx1, wy2-wy1, wz2-wz1);
    if (rd.Magnitude() < 1e-10) return TopoDS_Face();
    rd.Normalize();
    rayDir = gp_Dir(rd);

    TopoDS_Face bestFace;
    double bestDist = 1e30;

    TopExp_Explorer faceExp(m_shape, TopAbs_FACE);
    for (; faceExp.More(); faceExp.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;
        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();

        for (int t = 1; t <= tri->NbTriangles(); ++t) {
            int n1, n2, n3;
            tri->Triangle(t).Get(n1, n2, n3);
            gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
            if (!loc.IsIdentity()) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }

            // Möller–Trumbore
            gp_Vec e1(p1,p2), e2(p1,p3);
            gp_Vec h = gp_Vec(rayDir).Crossed(e2);
            double a = e1.Dot(h);
            if (std::abs(a) < 1e-10) continue;
            double f = 1.0/a;
            gp_Vec s(p1, rayOrigin);
            double u = f * s.Dot(h);
            if (u < 0 || u > 1) continue;
            gp_Vec q = s.Crossed(e1);
            double v = f * gp_Vec(rayDir).Dot(q);
            if (v < 0 || u+v > 1) continue;
            double t_val = f * e2.Dot(q);
            if (t_val > 1e-6 && t_val < bestDist) {
                bestDist = t_val;
                bestFace = face;
            }
        }
    }
    return bestFace;
}

// ─── API publique ─────────────────────────────────────────────────────────────

void Viewport3D::setShape(const TopoDS_Shape& shape) {
    m_shape = shape;
    m_tessellated = false;
    m_pickedFace = TopoDS_Face();
    update();
}

void Viewport3D::clearShape() {
    m_shape = TopoDS_Shape();
    m_tessellated = false;
    m_pickedFace = TopoDS_Face();
    update();
}

void Viewport3D::addToolpaths(const ToolpathDisplay& tpd) {
    m_toolpaths.push_back(tpd);
    update();
}

void Viewport3D::clearToolpaths() {
    m_toolpaths.clear();
    update();
}

void Viewport3D::setToolpathsVisible(bool v) {
    m_toolpathsVisible = v;
    update();
}

} // namespace CamEngine
