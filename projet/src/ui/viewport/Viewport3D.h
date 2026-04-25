#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <vector>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include "core/cam/Toolpath.h"

namespace CamEngine {

struct ToolpathDisplay {
    std::string name;
    std::vector<Toolpath> paths;
    float r = 1.f, g = 0.5f, b = 0.f;  // couleur RGB
    bool visible = true;
};

class Viewport3D : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit Viewport3D(QWidget* parent = nullptr);

    void setShape(const TopoDS_Shape& shape);
    void clearShape();

    void addToolpaths(const ToolpathDisplay& tpd);
    void clearToolpaths();
    void setToolpathsVisible(bool v);

    TopoDS_Face pickFace(int sx, int sy);
    bool hasPickedFace() const { return !m_pickedFace.IsNull(); }
    const TopoDS_Face& pickedFace() const { return m_pickedFace; }

signals:
    void faceClicked(const TopoDS_Face& face);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    void setupCamera();
    void drawShape();
    void drawToolpaths();
    void drawGrid();
    void tessellate();
    bool m_tessellated = false;

    TopoDS_Shape m_shape;
    TopoDS_Face  m_pickedFace;
    std::vector<ToolpathDisplay> m_toolpaths;
    bool m_toolpathsVisible = true;

    // Caméra orbitale
    double m_rotX = -30.0, m_rotZ = 30.0;
    double m_zoom = 1.0;
    double m_panX = 0.0, m_panY = 0.0;
    QPoint m_lastPos;
    bool   m_rotating = false, m_panning = false;

    // Cache matrices GL
    double m_modelview[16], m_projection[16];
    int    m_viewport[4];
    bool   m_glValid = false;
};

} // namespace CamEngine
