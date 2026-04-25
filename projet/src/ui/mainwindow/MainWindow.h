#pragma once
#include <QMainWindow>
#include <QTreeWidget>
#include <QDockWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSplitter>
#include <memory>
#include <vector>

#include "../../core/machine/MachineProfile.h"
#include "../../core/cam/CamOperation.h"
#include "../../core/cam/Toolpath.h"
#include "../viewport/Viewport3D.h"
#include <TopoDS_Shape.hxx>

namespace CamEngine {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Appelé depuis main() si --cadfile <path> est passé en argument
    void loadCadFile(const QString& path);

private slots:
    void onOpenCadFile();
    void onSaveGcode();
    void onAddOperation();
    void onRemoveOperation();
    void onComputeAll();
    void onComputeSelected();
    void onGenerateGcode();
    void onOperationSelected();
    void onFaceClicked(const TopoDS_Face& face);
    void onMachineChanged(int index);
    void onNewMachineProfile();
    void onEditMachineProfile();
    void onDeleteMachineProfile();
    void onPostProcessorChanged(int index);
    void onThemeChanged(int index);

private:
    void buildMenuBar();
    void buildToolBar();
    void buildDockOperations();
    void buildDockProperties();
    void buildDockMachine();
    void buildDockGcode();

    void refreshOperationTree();
    void refreshMachineList();
    void refreshToolList();
    void refreshPostProcessors();

    void applyParamsFromUI(std::shared_ptr<CamOperation>& op);
    void showOperationParams(const std::shared_ptr<CamOperation>& op);
    void computeOperation(std::shared_ptr<CamOperation>& op);

    void saveTheme(int idx);
    int  loadTheme();
    void applyTheme(int idx);

    // ─── Data ─────────────────────────────────────────────────────────────────
    TopoDS_Shape                         m_shape;
    QString                              m_cadFilePath;
    std::vector<std::shared_ptr<CamOperation>> m_operations;
    int                                  m_selectedOpIdx = -1;

    std::vector<MachineProfile>          m_profiles;
    int                                  m_currentProfileIdx = 0;

    // ─── Widgets ──────────────────────────────────────────────────────────────
    Viewport3D*    m_viewport    = nullptr;

    // Dock Opérations
    QTreeWidget*   m_opTree      = nullptr;
    QComboBox*     m_cbOpType    = nullptr;

    // Dock Paramètres opération
    QDoubleSpinBox* m_spDepth    = nullptr;
    QDoubleSpinBox* m_spTotalDepth = nullptr;
    QDoubleSpinBox* m_spStepover = nullptr;
    QDoubleSpinBox* m_spFinish   = nullptr;
    QDoubleSpinBox* m_spClearance= nullptr;
    QDoubleSpinBox* m_spSafe     = nullptr;
    QComboBox*     m_cbTool      = nullptr;
    QLabel*        m_lblFace     = nullptr;
    QCheckBox*     m_chkClimb    = nullptr;

    // Dock Machine
    QComboBox*     m_cbMachine   = nullptr;
    QComboBox*     m_cbPost      = nullptr;
    QLabel*        m_lblMachineInfo = nullptr;
    QListWidget*   m_toolList    = nullptr;

    // Dock G-code
    QTextEdit*     m_gcodeView   = nullptr;
    QComboBox*     m_cbPostFinal = nullptr;

    // Barre d'état
    QLabel*        m_statusLbl   = nullptr;

    void showStatus(const QString& msg);
};

} // namespace CamEngine
