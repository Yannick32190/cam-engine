#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include "ui/mainwindow/MainWindow.h"
#include "ui/theme/ThemeManager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("cam-engine");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("cad-engine");

    // Lire le thème partagé avec cad-engine
    QSettings themeSettings("cad-engine", "theme");
    int themeIdx = themeSettings.value("theme_index", 1).toInt();
    switch (themeIdx) {
        case 0: ThemeManager::applyTheme(ThemeManager::Light,    &app); break;
        case 2: ThemeManager::applyTheme(ThemeManager::BlueDark, &app); break;
        case 3: ThemeManager::applyTheme(ThemeManager::Graphite, &app); break;
        default: ThemeManager::applyTheme(ThemeManager::Dark,    &app); break;
    }

    // Analyse des arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("CAM-ENGINE — Générateur de G-code pour CAD-ENGINE");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "Fichier .cadengine à charger (optionnel)");
    QCommandLineOption cadOpt({"f","cadfile"}, "Fichier .cadengine à ouvrir", "file");
    parser.addOption(cadOpt);
    parser.process(app);

    CamEngine::MainWindow win;
    win.show();

    // Charger le fichier passé en argument (depuis cad-engine ou ligne de commande)
    QString cadFile;
    if (parser.isSet(cadOpt))
        cadFile = parser.value(cadOpt);
    else if (!parser.positionalArguments().isEmpty())
        cadFile = parser.positionalArguments().first();

    if (!cadFile.isEmpty())
        win.loadCadFile(cadFile);

    return app.exec();
}
