#include "MachineProfile.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <iostream>

namespace CamEngine {

// ─── Sérialisation Tool ───────────────────────────────────────────────────────

static QJsonObject toolToJson(const Tool& t) {
    QJsonObject o;
    o["id"]              = t.id;
    o["name"]            = QString::fromStdString(t.name);
    o["type"]            = static_cast<int>(t.type);
    o["diameter"]        = t.diameter;
    o["cornerRadius"]    = t.cornerRadius;
    o["fluteLength"]     = t.fluteLength;
    o["totalLength"]     = t.totalLength;
    o["flutes"]          = t.flutes;
    o["maxRPM"]          = t.maxRPM;
    o["feedPerTooth"]    = t.feedPerTooth;
    o["plungeFeedFactor"]= t.plungeFeedFactor;
    return o;
}

static Tool toolFromJson(const QJsonObject& o) {
    Tool t;
    t.id              = o["id"].toInt(1);
    t.name            = o["name"].toString("Outil").toStdString();
    t.type            = static_cast<ToolType>(o["type"].toInt(0));
    t.diameter        = o["diameter"].toDouble(6.0);
    t.cornerRadius    = o["cornerRadius"].toDouble(0.0);
    t.fluteLength     = o["fluteLength"].toDouble(20.0);
    t.totalLength     = o["totalLength"].toDouble(60.0);
    t.flutes          = o["flutes"].toInt(2);
    t.maxRPM          = o["maxRPM"].toDouble(15000.0);
    t.feedPerTooth    = o["feedPerTooth"].toDouble(0.02);
    t.plungeFeedFactor= o["plungeFeedFactor"].toDouble(0.3);
    return t;
}

// ─── saveToFile ───────────────────────────────────────────────────────────────

void MachineProfile::saveToFile(const std::string& path) const {
    QJsonObject root;
    root["id"]              = QString::fromStdString(id);
    root["name"]            = QString::fromStdString(name);
    root["type"]            = static_cast<int>(type);
    root["postProcessorId"] = QString::fromStdString(postProcessorId);
    root["description"]     = QString::fromStdString(description);

    QJsonObject env;
    env["xTravel"] = envelope.xTravel;
    env["yTravel"] = envelope.yTravel;
    env["zTravel"] = envelope.zTravel;
    root["envelope"] = env;

    QJsonObject sp;
    sp["minRPM"]            = spindle.minRPM;
    sp["maxRPM"]            = spindle.maxRPM;
    sp["maxPower"]          = spindle.maxPower;
    sp["hasCoolant"]        = spindle.hasCoolant;
    sp["hasThroughCoolant"] = spindle.hasThroughCoolant;
    root["spindle"] = sp;

    root["maxFeedRate"]   = maxFeedRate;
    root["maxRapidRate"]  = maxRapidRate;
    root["maxPlungeRate"] = maxPlungeRate;

    QJsonArray tools;
    for (const auto& t : toolLibrary)
        tools.append(toolToJson(t));
    root["toolLibrary"] = tools;

    QFile f(QString::fromStdString(path));
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

// ─── loadFromFile ─────────────────────────────────────────────────────────────

MachineProfile MachineProfile::loadFromFile(const std::string& path) {
    MachineProfile p;
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::ReadOnly)) return p;

    QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    p.id              = root["id"].toString().toStdString();
    p.name            = root["name"].toString("Ma CNC").toStdString();
    p.type            = static_cast<MachineType>(root["type"].toInt(0));
    p.postProcessorId = root["postProcessorId"].toString("grbl").toStdString();
    p.description     = root["description"].toString().toStdString();

    auto env = root["envelope"].toObject();
    p.envelope.xTravel = env["xTravel"].toDouble(300);
    p.envelope.yTravel = env["yTravel"].toDouble(200);
    p.envelope.zTravel = env["zTravel"].toDouble(150);

    auto sp = root["spindle"].toObject();
    p.spindle.minRPM            = sp["minRPM"].toDouble(100);
    p.spindle.maxRPM            = sp["maxRPM"].toDouble(20000);
    p.spindle.maxPower          = sp["maxPower"].toDouble(1000);
    p.spindle.hasCoolant        = sp["hasCoolant"].toBool(true);
    p.spindle.hasThroughCoolant = sp["hasThroughCoolant"].toBool(false);

    p.maxFeedRate   = root["maxFeedRate"].toDouble(3000);
    p.maxRapidRate  = root["maxRapidRate"].toDouble(8000);
    p.maxPlungeRate = root["maxPlungeRate"].toDouble(500);

    for (const auto& tv : root["toolLibrary"].toArray())
        p.toolLibrary.push_back(toolFromJson(tv.toObject()));

    return p;
}

// ─── profilesDir ─────────────────────────────────────────────────────────────

std::string MachineProfile::profilesDir() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                  + "/cam-engine/profiles/";
    QDir().mkpath(dir);
    return dir.toStdString();
}

// ─── loadAll ─────────────────────────────────────────────────────────────────

std::vector<MachineProfile> MachineProfile::loadAll() {
    std::vector<MachineProfile> result;
    QDir dir(QString::fromStdString(profilesDir()));
    for (const QString& fn : dir.entryList({"*.json"}, QDir::Files)) {
        result.push_back(loadFromFile((dir.absoluteFilePath(fn)).toStdString()));
    }
    if (result.empty())
        result.push_back(defaultProfile());
    return result;
}

// ─── saveAll ─────────────────────────────────────────────────────────────────

void MachineProfile::saveAll(const std::vector<MachineProfile>& profiles) {
    QString base = QString::fromStdString(profilesDir());
    for (const auto& p : profiles) {
        QString fn = QString::fromStdString(p.id.empty() ? p.name : p.id) + ".json";
        fn.replace(' ', '_');
        p.saveToFile((base + fn).toStdString());
    }
}

// ─── defaultProfile ──────────────────────────────────────────────────────────

MachineProfile MachineProfile::defaultProfile() {
    MachineProfile p;
    p.id              = "default_mill3";
    p.name            = "Fraiseuse 3 axes (défaut)";
    p.type            = MachineType::Mill3Axis;
    p.postProcessorId = "grbl";

    Tool t1; t1.id=1; t1.name="Ø6 Fraise 2T"; t1.type=ToolType::FlatEndMill;
    t1.diameter=6; t1.flutes=2; t1.maxRPM=15000; t1.feedPerTooth=0.02;
    p.toolLibrary.push_back(t1);

    Tool t2; t2.id=2; t2.name="Ø3 Fraise 2T"; t2.type=ToolType::FlatEndMill;
    t2.diameter=3; t2.flutes=2; t2.maxRPM=24000; t2.feedPerTooth=0.01;
    p.toolLibrary.push_back(t2);

    Tool t3; t3.id=3; t3.name="Ø5 Foret HSS"; t3.type=ToolType::DrillBit;
    t3.diameter=5; t3.flutes=2; t3.maxRPM=3000; t3.feedPerTooth=0.05;
    p.toolLibrary.push_back(t3);

    return p;
}

} // namespace CamEngine
