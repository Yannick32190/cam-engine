#include "MainWindow.h"
#include "../../core/importer/CadImporter.h"
#include "../../core/postprocessor/PostProcessorRegistry.h"
#include "../../core/cam/operations/FacingOp.h"
#include "../../core/cam/operations/ContourOp.h"
#include "../../core/cam/operations/PocketOp.h"
#include "../../core/cam/operations/DrillingOp.h"
#include "../theme/ThemeManager.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QScrollArea>
#include <QDateTime>

#include <iostream>
#include <fstream>

namespace CamEngine {

// ─── Constructeur ─────────────────────────────��───────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("CAM-ENGINE v0.1");
    setMinimumSize(1280, 800);

    m_viewport = new Viewport3D(this);
    setCentralWidget(m_viewport);
    connect(m_viewport, &Viewport3D::faceClicked, this, &MainWindow::onFaceClicked);

    buildMenuBar();
    buildToolBar();
    buildDockOperations();
    buildDockProperties();
    buildDockMachine();
    buildDockGcode();

    m_profiles = MachineProfile::loadAll();
    PostProcessorRegistry::registerAll();
    refreshMachineList();
    refreshPostProcessors();

    // Restaurer le thème
    applyTheme(loadTheme());

    statusBar()->showMessage("Prêt — Ouvrir un fichier .cadengine pour commencer");
}

// ─── Build UI ─────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar() {
    auto* mb = menuBar();

    auto* mf = mb->addMenu("&Fichier");
    mf->addAction("Ouvrir .cadengine…", this, &MainWindow::onOpenCadFile, QKeySequence::Open);
    mf->addSeparator();
    mf->addAction("Enregistrer G-code…", this, &MainWindow::onSaveGcode, QKeySequence::Save);
    mf->addSeparator();
    mf->addAction("Quitter", this, &QWidget::close, QKeySequence::Quit);

    auto* mo = mb->addMenu("&Opérations");
    mo->addAction("Ajouter",            this, &MainWindow::onAddOperation);
    mo->addAction("Calculer sélection", this, &MainWindow::onComputeSelected);
    mo->addAction("Calculer tout",      this, &MainWindow::onComputeAll);

    auto* mg = mb->addMenu("&G-code");
    mg->addAction("Générer", this, &MainWindow::onGenerateGcode, QKeySequence(Qt::CTRL | Qt::Key_G));
    mg->addAction("Enregistrer…", this, &MainWindow::onSaveGcode);

    auto* mth = mb->addMenu("&Thème");
    auto addTheme = [&](const QString& name, int idx) {
        auto* act = mth->addAction(name, [this, idx]{ applyTheme(idx); saveTheme(idx); });
        act->setCheckable(true);
    };
    addTheme("Clair",    0);
    addTheme("Sombre",   1);
    addTheme("Bleu nuit",2);
    addTheme("Graphite", 3);
}

void MainWindow::buildToolBar() {
    auto* tb = addToolBar("Principal");
    tb->setMovable(false);
    tb->addAction("Ouvrir", this, &MainWindow::onOpenCadFile);
    tb->addSeparator();
    tb->addAction("+ Op.",   this, &MainWindow::onAddOperation);
    tb->addAction("Calcul.", this, &MainWindow::onComputeAll);
    tb->addAction("G-code",  this, &MainWindow::onGenerateGcode);
    tb->addAction("Export",  this, &MainWindow::onSaveGcode);
    tb->addSeparator();
    auto* chkTp = new QCheckBox("Parcours", tb);
    chkTp->setChecked(true);
    connect(chkTp, &QCheckBox::toggled, m_viewport, &Viewport3D::setToolpathsVisible);
    tb->addWidget(chkTp);
}

void MainWindow::buildDockOperations() {
    auto* dock = new QDockWidget("Opérations", this);
    auto* w = new QWidget;
    auto* vbl = new QVBoxLayout(w);

    m_opTree = new QTreeWidget;
    m_opTree->setHeaderLabels({"#", "Type", "Outil", "État"});
    m_opTree->setColumnWidth(0, 30);
    m_opTree->setColumnWidth(1, 90);
    m_opTree->setColumnWidth(2, 80);
    connect(m_opTree, &QTreeWidget::itemClicked, this, [this]{ onOperationSelected(); });
    vbl->addWidget(m_opTree);

    auto* row = new QHBoxLayout;
    m_cbOpType = new QComboBox;
    m_cbOpType->addItem("Surfaçage");
    m_cbOpType->addItem("Contournage");
    m_cbOpType->addItem("Poche");
    m_cbOpType->addItem("Perçage");
    row->addWidget(m_cbOpType, 1);
    auto* btnAdd = new QPushButton("+");
    auto* btnDel = new QPushButton("−");
    btnAdd->setFixedWidth(28); btnDel->setFixedWidth(28);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onAddOperation);
    connect(btnDel, &QPushButton::clicked, this, &MainWindow::onRemoveOperation);
    row->addWidget(btnAdd); row->addWidget(btnDel);
    vbl->addLayout(row);

    auto* btnRow2 = new QHBoxLayout;
    auto* btnCalcSel = new QPushButton("Calculer");
    auto* btnCalcAll = new QPushButton("Tout");
    connect(btnCalcSel, &QPushButton::clicked, this, &MainWindow::onComputeSelected);
    connect(btnCalcAll, &QPushButton::clicked, this, &MainWindow::onComputeAll);
    btnRow2->addWidget(btnCalcSel); btnRow2->addWidget(btnCalcAll);
    vbl->addLayout(btnRow2);

    dock->setWidget(w);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void MainWindow::buildDockProperties() {
    auto* dock = new QDockWidget("Paramètres opération", this);
    auto* scroll = new QScrollArea;
    auto* w = new QWidget;
    auto* vbl = new QVBoxLayout(w);
    scroll->setWidget(w); scroll->setWidgetResizable(true);

    auto sp = [](double mn, double mx, double v, double st, const QString& suf, int d=2) {
        auto* x = new QDoubleSpinBox;
        x->setRange(mn,mx); x->setValue(v); x->setSingleStep(st);
        x->setSuffix(suf); x->setDecimals(d); return x;
    };

    // Passes
    auto* g1 = new QGroupBox("Passes"); auto* f1 = new QFormLayout(g1);
    m_spDepth      = sp(0.01, 50, 1.0,  0.1, " mm");
    m_spTotalDepth = sp(0.01,500, 5.0,  0.5, " mm");
    m_spStepover   = sp(0.05,  1, 0.5, 0.05, "×D");
    m_spFinish     = sp(0.0,   5, 0.2, 0.05, " mm");
    f1->addRow("Prof. passe :",    m_spDepth);
    f1->addRow("Prof. totale :",   m_spTotalDepth);
    f1->addRow("Stepover :",       m_spStepover);
    f1->addRow("Surépaisseur :",   m_spFinish);
    m_chkClimb = new QCheckBox("Fraisage en opposition");
    m_chkClimb->setChecked(true);
    f1->addRow(m_chkClimb);
    vbl->addWidget(g1);

    // Hauteurs
    auto* g2 = new QGroupBox("Hauteurs"); auto* f2 = new QFormLayout(g2);
    m_spClearance = sp(1, 200, 10, 1, " mm");
    m_spSafe      = sp(.5,  50,  5, .5," mm");
    f2->addRow("Dégagement :", m_spClearance);
    f2->addRow("Sécurité :",   m_spSafe);
    vbl->addWidget(g2);

    // Outil
    auto* g3 = new QGroupBox("Outil"); auto* f3 = new QFormLayout(g3);
    m_cbTool = new QComboBox;
    f3->addRow("Outil :", m_cbTool);
    vbl->addWidget(g3);

    // Face cible
    auto* g4 = new QGroupBox("Face cible"); auto* f4 = new QFormLayout(g4);
    m_lblFace = new QLabel("Aucune — cliquez une face dans la vue 3D");
    m_lblFace->setWordWrap(true);
    m_lblFace->setStyleSheet("color: #888; font-size: 11px;");
    f4->addRow(m_lblFace);
    vbl->addWidget(g4);

    vbl->addStretch();
    dock->setWidget(scroll);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::buildDockMachine() {
    auto* dock = new QDockWidget("Machine", this);
    auto* w = new QWidget;
    auto* vbl = new QVBoxLayout(w);

    // Sélection profil
    auto* g1 = new QGroupBox("Profil machine"); auto* f1 = new QFormLayout(g1);
    m_cbMachine = new QComboBox;
    connect(m_cbMachine, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMachineChanged);
    f1->addRow("Profil :", m_cbMachine);

    auto* btnRow = new QHBoxLayout;
    auto* btnNew  = new QPushButton("Nouveau");
    auto* btnEdit = new QPushButton("Éditer");
    auto* btnDel  = new QPushButton("Suppr.");
    connect(btnNew,  &QPushButton::clicked, this, &MainWindow::onNewMachineProfile);
    connect(btnEdit, &QPushButton::clicked, this, &MainWindow::onEditMachineProfile);
    connect(btnDel,  &QPushButton::clicked, this, &MainWindow::onDeleteMachineProfile);
    btnRow->addWidget(btnNew); btnRow->addWidget(btnEdit); btnRow->addWidget(btnDel);
    f1->addRow(btnRow);

    m_lblMachineInfo = new QLabel;
    m_lblMachineInfo->setWordWrap(true);
    m_lblMachineInfo->setStyleSheet("font-size: 11px; color: #888;");
    f1->addRow(m_lblMachineInfo);
    vbl->addWidget(g1);

    // Post-processeur
    auto* g2 = new QGroupBox("Post-processeur"); auto* f2 = new QFormLayout(g2);
    m_cbPost = new QComboBox;
    connect(m_cbPost, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPostProcessorChanged);
    f2->addRow("Post-proc. :", m_cbPost);
    vbl->addWidget(g2);

    // Bibliothèque d'outils
    auto* g3 = new QGroupBox("Bibliothèque d'outils");
    auto* vbl3 = new QVBoxLayout(g3);
    m_toolList = new QListWidget;
    m_toolList->setMaximumHeight(140);
    vbl3->addWidget(m_toolList);
    vbl->addWidget(g3);

    vbl->addStretch();
    dock->setWidget(w);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::buildDockGcode() {
    auto* dock = new QDockWidget("G-code", this);
    auto* w = new QWidget;
    auto* vbl = new QVBoxLayout(w);

    m_gcodeView = new QTextEdit;
    m_gcodeView->setReadOnly(true);
    m_gcodeView->setFont(QFont("Consolas,Courier New,monospace", 10));
    m_gcodeView->setStyleSheet("QTextEdit { background:#1e1e1e; color:#d4d4d4; }");
    vbl->addWidget(m_gcodeView, 1);

    auto* row = new QHBoxLayout;
    auto* btnGen  = new QPushButton("Générer");
    auto* btnSave = new QPushButton("Enregistrer…");
    connect(btnGen,  &QPushButton::clicked, this, &MainWindow::onGenerateGcode);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::onSaveGcode);
    row->addWidget(btnGen); row->addWidget(btnSave);
    vbl->addLayout(row);

    dock->setWidget(w);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

// ─── Slots ───────────────��────────────────────────────��──────────────────────

void MainWindow::onOpenCadFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "Ouvrir fichier CAD-ENGINE", {},
        "Fichiers CAD-ENGINE (*.cadengine);;Tous (*)");
    if (path.isEmpty()) return;
    loadCadFile(path);
}

void MainWindow::loadCadFile(const QString& path) {
    showStatus("Chargement : " + path + " …");
    QApplication::processEvents();

    m_shape = CadImporter::import(path);
    if (m_shape.IsNull()) {
        QMessageBox::warning(this, "CAM-ENGINE",
            "Impossible de reconstruire le solide.\n"
            "Vérifiez que le fichier contient des features Extrude ou Revolve.");
        showStatus("Échec du chargement");
        return;
    }

    m_cadFilePath = path;
    m_viewport->setShape(m_shape);
    setWindowTitle("CAM-ENGINE — " + QFileInfo(path).baseName());
    showStatus("Chargé : " + QFileInfo(path).baseName());
    refreshToolList();
}

void MainWindow::onAddOperation() {
    int typeIdx = m_cbOpType->currentIndex();
    std::shared_ptr<CamOperation> op;
    switch (typeIdx) {
        case 0: op = std::make_shared<FacingOp>();   break;
        case 1: op = std::make_shared<ContourOp>();  break;
        case 2: op = std::make_shared<PocketOp>();   break;
        case 3: op = std::make_shared<DrillingOp>(); break;
        default: return;
    }

    // Outil par défaut depuis le profil courant
    if (m_currentProfileIdx < (int)m_profiles.size() &&
        !m_profiles[m_currentProfileIdx].toolLibrary.empty()) {
        op->setTool(m_profiles[m_currentProfileIdx].toolLibrary[0]);
    }

    // Face sélectionnée
    if (m_viewport->hasPickedFace())
        op->setTargetFace(m_viewport->pickedFace());
    if (!m_shape.IsNull())
        op->setTargetShape(m_shape);

    m_operations.push_back(op);
    m_selectedOpIdx = (int)m_operations.size() - 1;
    refreshOperationTree();
    m_opTree->setCurrentItem(m_opTree->topLevelItem(m_selectedOpIdx));
    showOperationParams(op);
}

void MainWindow::onRemoveOperation() {
    if (m_selectedOpIdx < 0 || m_selectedOpIdx >= (int)m_operations.size()) return;
    m_operations.erase(m_operations.begin() + m_selectedOpIdx);
    m_selectedOpIdx = -1;
    refreshOperationTree();
    m_viewport->clearToolpaths();
    for (auto& op : m_operations)
        if (op->isComputed())
            for (auto& tp : op->toolpaths()) {
                ToolpathDisplay tpd;
                tpd.name = tp.name;
                tpd.paths.push_back(tp);
                tpd.r = 1.f; tpd.g = 0.6f; tpd.b = 0.f;
                m_viewport->addToolpaths(tpd);
            }
}

void MainWindow::onComputeSelected() {
    if (m_selectedOpIdx < 0 || m_selectedOpIdx >= (int)m_operations.size()) {
        QMessageBox::information(this, "CAM-ENGINE", "Sélectionnez une opération.");
        return;
    }
    computeOperation(m_operations[m_selectedOpIdx]);
    refreshOperationTree();
}

void MainWindow::onComputeAll() {
    for (auto& op : m_operations)
        computeOperation(op);
    refreshOperationTree();
}

void MainWindow::computeOperation(std::shared_ptr<CamOperation>& op) {
    if (!op) return;
    applyParamsFromUI(op);
    if (!m_shape.IsNull()) op->setTargetShape(m_shape);

    MachineProfile profile;
    if (m_currentProfileIdx < (int)m_profiles.size())
        profile = m_profiles[m_currentProfileIdx];

    showStatus("Calcul : " + QString::fromStdString(op->name()) + " …");
    QApplication::processEvents();

    auto tps = op->compute(profile);
    if (tps.empty()) {
        showStatus("Aucun parcours — vérifiez la face cible et les paramètres");
        return;
    }

    // Afficher dans le viewport
    for (const auto& tp : tps) {
        ToolpathDisplay tpd;
        tpd.name = tp.name;
        tpd.paths.push_back(tp);
        // Couleurs par type d'opération
        switch (op->type()) {
            case OperationType::Facing:   tpd.r=0.2f; tpd.g=0.8f; tpd.b=0.4f; break;
            case OperationType::Contour:  tpd.r=1.0f; tpd.g=0.6f; tpd.b=0.0f; break;
            case OperationType::Pocket:   tpd.r=0.3f; tpd.g=0.6f; tpd.b=1.0f; break;
            case OperationType::Drilling: tpd.r=1.0f; tpd.g=0.2f; tpd.b=0.2f; break;
        }
        m_viewport->addToolpaths(tpd);
    }

    size_t total = 0;
    for (const auto& tp : tps) total += tp.moves.size();
    showStatus(QString("Calculé : %1 passes, %2 mouvements")
               .arg(tps.size()).arg(total));
}

void MainWindow::onGenerateGcode() {
    if (m_profiles.empty()) {
        QMessageBox::warning(this, "CAM-ENGINE", "Aucun profil machine disponible.");
        return;
    }
    const MachineProfile& profile = m_profiles[m_currentProfileIdx];
    auto& reg = PostProcessorRegistry::instance();
    std::string ppId = profile.postProcessorId;

    // Utiliser la sélection du dock G-code si disponible
    int postIdx = m_cbPost ? m_cbPost->currentIndex() : 0;
    auto allPPs = reg.all();
    if (postIdx >= 0 && postIdx < (int)allPPs.size())
        ppId = allPPs[postIdx]->id();

    auto pp = reg.get(ppId);
    if (!pp) pp = reg.get("grbl");
    if (!pp) { QMessageBox::warning(this, "CAM-ENGINE", "Post-processeur introuvable."); return; }

    // Rassembler toutes les toolpaths calculées
    std::vector<Toolpath> all;
    Tool tool;
    bool hasTool = false;
    for (const auto& op : m_operations) {
        if (!op->isComputed()) continue;
        if (!hasTool) { tool = op->tool(); hasTool = true; }
        for (const auto& tp : op->toolpaths())
            all.push_back(tp);
    }

    if (all.empty()) {
        QMessageBox::information(this, "CAM-ENGINE",
            "Aucune opération calculée.\nLancez 'Calculer tout' d'abord.");
        return;
    }

    if (!hasTool && !profile.toolLibrary.empty())
        tool = profile.toolLibrary[0];

    std::string programName = "O0001";
    if (!m_cadFilePath.isEmpty())
        programName = "O" + QFileInfo(m_cadFilePath).baseName().left(4).toUpper().toStdString();

    std::string gcode = pp->generate(all, tool, profile, programName);
    m_gcodeView->setPlainText(QString::fromStdString(gcode));
    showStatus(QString("G-code généré : %1 lignes — post-proc : %2")
               .arg(m_gcodeView->document()->blockCount())
               .arg(QString::fromStdString(pp->name())));
}

void MainWindow::onSaveGcode() {
    QString gcode = m_gcodeView->toPlainText();
    if (gcode.isEmpty()) { onGenerateGcode(); gcode = m_gcodeView->toPlainText(); }
    if (gcode.isEmpty()) return;

    const MachineProfile& profile = m_profiles.empty() ? MachineProfile::defaultProfile()
                                                        : m_profiles[m_currentProfileIdx];
    auto pp = PostProcessorRegistry::instance().get(profile.postProcessorId);
    QString ext = pp ? QString::fromStdString(pp->fileExtension()) : "nc";

    QString path = QFileDialog::getSaveFileName(
        this, "Enregistrer G-code",
        QFileInfo(m_cadFilePath).baseName() + "." + ext,
        QString("G-code (*.%1 *.nc *.tap *.ngc);;Tous (*)").arg(ext));
    if (path.isEmpty()) return;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text))
        f.write(gcode.toUtf8());
    showStatus("G-code enregistré : " + path);
}

void MainWindow::onOperationSelected() {
    int idx = m_opTree->indexOfTopLevelItem(m_opTree->currentItem());
    if (idx < 0 || idx >= (int)m_operations.size()) return;
    m_selectedOpIdx = idx;
    showOperationParams(m_operations[idx]);
}

void MainWindow::onFaceClicked(const TopoDS_Face& face) {
    if (m_selectedOpIdx >= 0 && m_selectedOpIdx < (int)m_operations.size())
        m_operations[m_selectedOpIdx]->setTargetFace(face);
    m_lblFace->setText("Face sélectionnée");
    m_lblFace->setStyleSheet("color: #4CAF50; font-weight: bold; font-size: 11px;");
    showStatus("Face assignée à l'opération " +
               QString::fromStdString(m_operations.empty() ? "" : m_operations[m_selectedOpIdx]->name()));
}

void MainWindow::onMachineChanged(int index) {
    if (index < 0 || index >= (int)m_profiles.size()) return;
    m_currentProfileIdx = index;
    const auto& p = m_profiles[index];
    m_lblMachineInfo->setText(
        QString("%1\nCourse: X%2 Y%3 Z%4 mm\nBroche: %5–%6 tr/min")
        .arg(QString::fromStdString(machineTypeName(p.type)))
        .arg(p.envelope.xTravel).arg(p.envelope.yTravel).arg(p.envelope.zTravel)
        .arg(p.spindle.minRPM).arg(p.spindle.maxRPM));

    // Sélectionner automatiquement le post-processeur du profil
    auto& reg = PostProcessorRegistry::instance();
    auto allPPs = reg.all();
    for (int i = 0; i < (int)allPPs.size(); ++i) {
        if (allPPs[i]->id() == p.postProcessorId) {
            m_cbPost->setCurrentIndex(i);
            break;
        }
    }
    refreshToolList();
}

void MainWindow::onNewMachineProfile() {
    bool ok;
    QString name = QInputDialog::getText(this, "Nouveau profil",
        "Nom du profil :", QLineEdit::Normal, "Ma machine", &ok);
    if (!ok || name.isEmpty()) return;
    MachineProfile p;
    p.id = name.toLower().replace(' ','_').toStdString();
    p.name = name.toStdString();
    p.toolLibrary = MachineProfile::defaultProfile().toolLibrary;
    m_profiles.push_back(p);
    MachineProfile::saveAll(m_profiles);
    refreshMachineList();
    m_cbMachine->setCurrentIndex((int)m_profiles.size() - 1);
}

void MainWindow::onEditMachineProfile() {
    if (m_currentProfileIdx < 0 || m_currentProfileIdx >= (int)m_profiles.size()) return;
    // Dialogue d'édition simplifié
    auto& p = m_profiles[m_currentProfileIdx];
    bool ok;
    QString name = QInputDialog::getText(this, "Renommer profil", "Nom :",
        QLineEdit::Normal, QString::fromStdString(p.name), &ok);
    if (!ok || name.isEmpty()) return;
    p.name = name.toStdString();
    MachineProfile::saveAll(m_profiles);
    refreshMachineList();
}

void MainWindow::onDeleteMachineProfile() {
    if (m_profiles.size() <= 1) {
        QMessageBox::warning(this, "CAM-ENGINE", "Impossible de supprimer le dernier profil.");
        return;
    }
    if (m_currentProfileIdx < 0 || m_currentProfileIdx >= (int)m_profiles.size()) return;
    auto r = QMessageBox::question(this, "Supprimer profil",
        "Supprimer le profil « " +
        QString::fromStdString(m_profiles[m_currentProfileIdx].name) + " » ?",
        QMessageBox::Yes | QMessageBox::No);
    if (r != QMessageBox::Yes) return;
    m_profiles.erase(m_profiles.begin() + m_currentProfileIdx);
    MachineProfile::saveAll(m_profiles);
    m_currentProfileIdx = 0;
    refreshMachineList();
}

void MainWindow::onPostProcessorChanged(int index) {
    if (m_currentProfileIdx >= (int)m_profiles.size()) return;
    auto allPPs = PostProcessorRegistry::instance().all();
    if (index >= 0 && index < (int)allPPs.size()) {
        m_profiles[m_currentProfileIdx].postProcessorId = allPPs[index]->id();
        MachineProfile::saveAll(m_profiles);
    }
}

void MainWindow::onThemeChanged(int index) {
    applyTheme(index);
    saveTheme(index);
}

// ─── Helpers ──────────��────────────────────────────────────���─────────────────

void MainWindow::refreshOperationTree() {
    m_opTree->clear();
    for (int i = 0; i < (int)m_operations.size(); ++i) {
        const auto& op = m_operations[i];
        auto* item = new QTreeWidgetItem(m_opTree);
        item->setText(0, QString::number(i+1));
        item->setText(1, QString::fromStdString(opTypeName(op->type())));
        item->setText(2, QString("T%1 Ø%2")
            .arg(op->tool().id).arg(op->tool().diameter, 0, 'f', 1));
        item->setText(3, op->isComputed() ? "✓" : "—");
        if (op->isComputed())
            item->setForeground(3, QBrush(QColor(0x4c,0xaf,0x50)));
    }
}

void MainWindow::refreshMachineList() {
    m_cbMachine->blockSignals(true);
    m_cbMachine->clear();
    for (const auto& p : m_profiles)
        m_cbMachine->addItem(QString::fromStdString(p.name));
    if (m_currentProfileIdx < m_cbMachine->count())
        m_cbMachine->setCurrentIndex(m_currentProfileIdx);
    m_cbMachine->blockSignals(false);
    if (!m_profiles.empty()) onMachineChanged(m_currentProfileIdx);
}

void MainWindow::refreshToolList() {
    m_toolList->clear();
    m_cbTool->clear();
    if (m_currentProfileIdx >= (int)m_profiles.size()) return;
    for (const auto& t : m_profiles[m_currentProfileIdx].toolLibrary) {
        QString s = QString("T%1 — %2 (Ø%3)")
            .arg(t.id).arg(QString::fromStdString(t.name)).arg(t.diameter, 0,'f',1);
        m_toolList->addItem(s);
        m_cbTool->addItem(s);
    }
}

void MainWindow::refreshPostProcessors() {
    m_cbPost->blockSignals(true);
    m_cbPost->clear();
    for (const auto& pp : PostProcessorRegistry::instance().all())
        m_cbPost->addItem(QString::fromStdString(pp->name()));
    m_cbPost->blockSignals(false);
}

void MainWindow::applyParamsFromUI(std::shared_ptr<CamOperation>& op) {
    CamParams p;
    p.depthOfCut      = m_spDepth->value();
    p.totalDepth      = m_spTotalDepth->value();
    p.stepover        = m_spStepover->value();
    p.finishAllowance = m_spFinish->value();
    p.clearanceHeight = m_spClearance->value();
    p.safeHeight      = m_spSafe->value();
    p.climb           = m_chkClimb->isChecked();
    op->setParams(p);

    // Outil
    int toolIdx = m_cbTool->currentIndex();
    if (m_currentProfileIdx < (int)m_profiles.size() &&
        toolIdx >= 0 &&
        toolIdx < (int)m_profiles[m_currentProfileIdx].toolLibrary.size()) {
        op->setTool(m_profiles[m_currentProfileIdx].toolLibrary[toolIdx]);
    }
}

void MainWindow::showOperationParams(const std::shared_ptr<CamOperation>& op) {
    if (!op) return;
    const CamParams& p = op->params();
    m_spDepth->setValue(p.depthOfCut);
    m_spTotalDepth->setValue(p.totalDepth);
    m_spStepover->setValue(p.stepover);
    m_spFinish->setValue(p.finishAllowance);
    m_spClearance->setValue(p.clearanceHeight);
    m_spSafe->setValue(p.safeHeight);
    m_chkClimb->setChecked(p.climb);
}

void MainWindow::saveTheme(int idx) {
    QSettings s("cad-engine", "theme");
    s.setValue("theme_index", idx);
}

int MainWindow::loadTheme() {
    QSettings s("cad-engine", "theme");
    return s.value("theme_index", 1).toInt();
}

void MainWindow::applyTheme(int idx) {
    auto* app = qobject_cast<QApplication*>(QApplication::instance());
    if (!app) return;
    switch (idx) {
        case 0: ThemeManager::applyTheme(ThemeManager::Light,    app); break;
        case 1: ThemeManager::applyTheme(ThemeManager::Dark,     app); break;
        case 2: ThemeManager::applyTheme(ThemeManager::BlueDark, app); break;
        case 3: ThemeManager::applyTheme(ThemeManager::Graphite, app); break;
        default: ThemeManager::applyTheme(ThemeManager::Dark,    app); break;
    }
    // Mettre à jour la couleur de fond du viewport selon le thème
    bool dark = (idx != 0);
    if (m_gcodeView) {
        if (dark) m_gcodeView->setStyleSheet("QTextEdit{background:#1e1e1e;color:#d4d4d4;}");
        else      m_gcodeView->setStyleSheet("QTextEdit{background:#f8f8f2;color:#202020;}");
    }
}

void MainWindow::showStatus(const QString& msg) {
    statusBar()->showMessage(msg, 6000);
}

} // namespace CamEngine
