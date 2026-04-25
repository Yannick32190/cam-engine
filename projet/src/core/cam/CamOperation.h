#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Toolpath.h"
#include "../machine/Tool.h"
#include "../machine/MachineProfile.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>

namespace CamEngine {

enum class OperationType {
    Facing,     // Surfaçage
    Contour,    // Contournage 2.5D
    Pocket,     // Vidage de poche
    Drilling    // Perçage
};

inline std::string opTypeName(OperationType t) {
    switch (t) {
        case OperationType::Facing:   return "Surfaçage";
        case OperationType::Contour:  return "Contournage";
        case OperationType::Pocket:   return "Poche";
        case OperationType::Drilling: return "Perçage";
        default:                      return "?";
    }
}

struct CamParams {
    double depthOfCut       = 1.0;   // mm — profondeur par passe
    double totalDepth       = 5.0;   // mm — profondeur totale
    double stepover         = 0.5;   // fraction du diamètre outil
    double finishAllowance  = 0.2;   // mm — surépaisseur finition
    double clearanceHeight  = 10.0;  // mm — hauteur de dégagement
    double safeHeight       = 5.0;   // mm — hauteur de sécurité
    double tolerance        = 0.01;  // mm — tolérance calcul
    bool   climb            = true;  // fraisage en opposition (true) ou conventionnel
};

// ─── Classe de base abstraite ─────────────────────────────────────────────────

class CamOperation {
public:
    explicit CamOperation(OperationType type);
    virtual ~CamOperation() = default;

    OperationType type() const { return m_type; }
    const std::string& name() const { return m_name; }
    void setName(const std::string& n) { m_name = n; }

    void setTool(const Tool& tool)               { m_tool = tool; }
    void setParams(const CamParams& p)           { m_params = p; }
    void setTargetFace(const TopoDS_Face& face)  { m_face = face; }
    void setTargetShape(const TopoDS_Shape& s)   { m_shape = s; }

    const Tool&       tool()   const { return m_tool; }
    const CamParams&  params() const { return m_params; }
    const TopoDS_Face&  face() const { return m_face; }

    // Point d'entrée principal
    virtual std::vector<Toolpath> compute(
        const MachineProfile& machine) = 0;

    bool         isComputed() const  { return m_computed; }
    const std::vector<Toolpath>& toolpaths() const { return m_toolpaths; }

protected:
    OperationType          m_type;
    std::string            m_name;
    Tool                   m_tool;
    CamParams              m_params;
    TopoDS_Face            m_face;
    TopoDS_Shape           m_shape;
    bool                   m_computed = false;
    std::vector<Toolpath>  m_toolpaths;

    // Utilitaires communs
    ToolMove rapidTo(double x, double y, double z) const;
    ToolMove feedTo(double x, double y, double z, double f = 0) const;
    ToolMove plungeTo(double z, double f = 0) const;
};

} // namespace CamEngine
