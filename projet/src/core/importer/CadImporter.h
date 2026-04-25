#pragma once
#include <QString>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>

namespace CamEngine {

// Lit un fichier .cadengine (JSON) et reconstruit le solide OCCT final
// en rejouant la chaîne de features (Sketch → Extrude/Revolve → …)
class CadImporter {
public:
    // Retourne le solide final ou une shape nulle si l'import échoue
    static TopoDS_Shape import(const QString& filePath);
};

} // namespace CamEngine
