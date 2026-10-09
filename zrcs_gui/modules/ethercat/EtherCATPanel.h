#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QToolBar>
#include <QLabel>
#include <QVector>
#include <QString>
#include <memory>

namespace Ui { class EtherCATPanelUi; }

/**
 * @brief EtherCAT 主站(master)配置 —— 对应 OpenPLC Editor "Advanced / Master Configuration"
 */
struct EtherCATMasterConfig {
    bool enabled = true;                 // Enable Bus
    QString networkInterface = "eth0";   // 网络接口
    int cycleTimeUs = 1000;              // Cycle Time (microseconds)
    int taskPriority = 90;               // Task Priority
    int watchdogTimeoutCycles = 3;       // Watchdog Timeout (cycles)
};

/**
 * @brief 从站通道映射条目 —— Channel Mapping (# / Dir / IEC Type / Address / Alias)
 */
struct EtherCATChannelMapping {
    int channelId = 0;
    QString direction = "input";   // input / output
    QString iecType;               // IEC 数据类型
    QString iecLocation;           // IEC 地址 (%IX / %QW)，只读自动分配
    QString alias;                 // 可编辑别名
};

/**
 * @brief SDO 启动参数条目 (Startup Parameters / SDO)
 */
struct SDOConfigEntry {
    quint16 index = 0;
    quint8  subIndex = 0;
    QString value;
    QString defaultValue;
    QString dataType;
    int    bitLength = 0;
    QString name;
};

/**
 * @brief 从站配置 EtherCATSlaveConfig —— 对应 esi-types 的 slave config
 */
struct EtherCATSlaveConfig {
    // Startup Checks
    bool checkVendorId = true;
    bool checkProductCode = true;
    // Addressing
    int ethercatAddress = 0;       // EtherCAT Address, 0 = auto
    // Timeouts (ms)
    int sdoTimeoutMs = 1000;
    int initToPreOpTimeoutMs = 3000;
    int safeOpToOpTimeoutMs = 10000;
    // Watchdog (ms)
    bool smWatchdogEnabled = true;
    int  smWatchdogMs = 100;
    bool pdiWatchdogEnabled = false;
    int  pdiWatchdogMs = 100;
    // Distributed Clocks (DC)
    bool dcEnabled = false;
    int  dcSyncUnitCycleUs = 0;    // 0 = master cycle
    bool dcSync0Enabled = false;
    int  dcSync0CycleUs = 0;
    int  dcSync0ShiftUs = 0;
    bool dcSync1Enabled = false;
    int  dcSync1CycleUs = 0;
    int  dcSync1ShiftUs = 0;
};

/**
 * @brief 已配置从站 EtherCAT 设备 —— 对应 ConfiguredEtherCATDevice
 */
struct EtherCATDevice {
    QString name;                 // 从站名称
    QString type;                 // 设备类型 category (SERVO_DRIVE / COUPLER / ...)
    QString vendorName;           // 厂商名称
    QString groupName;            // ESI 组名
    QString source;               // "Scan" / "Manual" / "Repository"
    int position = 0;             // 拓扑位置
    quint32 vendorId = 0;
    quint32 productCode = 0;
    quint32 revisionNo = 0;
    int inputChannels = 0;        // Channels In
    int outputChannels = 0;       // Channels Out

    EtherCATSlaveConfig config;
    QVector<EtherCATChannelMapping> channelMappings;
    QVector<SDOConfigEntry> sdoConfigurations;
};

/**
 * @class EtherCATPanel
 * @brief EtherCAT 主站组态界面(移植自 OpenPLC Editor 的 EtherCAT Bus Editor)
 *
 * 对应界面结构：
 *   - Bus 标签页：从站网络/Configured Devices 表(Name/Type/Position/Channels/In/Out/Source)
 *     + 从站详情(Device Info + Configuration + Channel Mapping + Startup Parameters/SDO)
 *   - Repository 标签页：ESI 从站设备库
 *   - Advanced 标签页：主站配置(Enable Bus/周期/优先级/看门狗)
 *   - "Add Device from Repository" 按厂商分组的添加对话框
 */
class EtherCATPanel : public QWidget {
    Q_OBJECT

public:
    explicit EtherCATPanel(QWidget *parent = nullptr);
    ~EtherCATPanel();

    /// 从站网络列表
    const QVector<EtherCATDevice> &devices() const { return devices_; }
    const EtherCATMasterConfig &masterConfig() const { return masterConfig_; }

    /// 序列化当前 EtherCAT 组态为 XML
    QString saveToXML() const;
    /// 从 XML 载入 EtherCAT 组态
    bool loadFromXML(const QString &xmlText, QString *error = nullptr);

    /// 取项目 program 目录(与行为树一致的约定)
    static QString defaultProgramDirectory();

public slots:
    // Configured Devices
    void onAddDevice();
    void onRemoveDevice();
    void onMoveUp();
    void onMoveDown();
    void onScanDevice();
    void onAddSelectedDevices();
    // 从站详情
    void onDeviceSelected(int row);
    void onApplyDeviceConfig();
    void onEnableDcToggled(bool checked);
    // 网络 / ESI
    void onImportEsi();
    void onLoadNetwork();
    void onSaveNetwork();
    void onSendToController();
    // Master (Advanced)
    void onApplyMasterConfig();

    void onAddChannelMapping();
    void onRemoveChannelMapping();
    void onAddSdoEntry();
    void onRemoveSdoEntry();

signals:
    /// 由 MainWindow 桥接到 ZMQClient
    void applyEthercatRequested(const QString &xml);

protected:
    /// 左右两栏表头行高对齐。左栏是纯 QLabel，右栏是 QLabel + 按钮，
    /// 按钮高度由样式表决定，故在样式变化/显示时重新对齐。
    bool event(QEvent *e) override;

private:
    void setupUI();
    void setupConnections();
    void rebuildDeviceTable();
    void rebuildChannelTable();
    void rebuildSdoTable();
    void populateRepository();
    void refreshStatus();

    void setCurrentDevice(const EtherCATDevice &dev);
    void clearDeviceDetail();
    int  currentDeviceIndex() const;
    int  scanNextPosition() const;

    QString dialogStartDirectory(const QString &settingsKey) const;

    Ui::EtherCATPanelUi *ui;

    QVector<EtherCATDevice> devices_;
    EtherCATMasterConfig masterConfig_;

    // 选中设备下标(相对于 configured devices 表)
    int currentDeviceRow_ = -1;

    // 界面子控件
    QToolBar       *toolBar_ = nullptr;
    QTabWidget     *masterTabs_ = nullptr;
    QTableWidget   *deviceTable_ = nullptr;      // Configured Devices
    QTreeWidget    *scannedTree_ = nullptr;      // Scanned Devices
    QComboBox      *interfaceCombo_ = nullptr;
    QLabel         *ecatStatusLabel_ = nullptr;

    // Repository 标签页
    QTableWidget   *repositoryTable_ = nullptr;

    // 从站详情(Device Info)
    QLabel         *detailName_ = nullptr;
    QLabel         *detailType_ = nullptr;
    QLabel         *detailVendor_ = nullptr;
    QLabel         *detailVendorId_ = nullptr;
    QLabel         *detailProductCode_ = nullptr;
    QLabel         *detailRevision_ = nullptr;
    QLabel         *detailGroup_ = nullptr;
    QLabel         *detailChannels_ = nullptr;

    // 从站配置表单(Configuration)
    QCheckBox      *chkCheckVendor_ = nullptr;
    QCheckBox      *chkCheckProduct_ = nullptr;
    QSpinBox       *spinEcatAddress_ = nullptr;
    QSpinBox       *spinSdoTimeout_ = nullptr;
    QSpinBox       *spinInitPreOp_ = nullptr;
    QSpinBox       *spinSafeOpOp_ = nullptr;
    QCheckBox      *chkSmWatchdog_ = nullptr;
    QSpinBox       *spinSmWatchdogMs_ = nullptr;
    QCheckBox      *chkPdiWatchdog_ = nullptr;
    QSpinBox       *spinPdiWatchdogMs_ = nullptr;
    QCheckBox      *chkDc_ = nullptr;
    QSpinBox       *spinDcSyncUnit_ = nullptr;
    QCheckBox      *chkSync0_ = nullptr;
    QSpinBox       *spinSync0Cycle_ = nullptr;
    QSpinBox       *spinSync0Shift_ = nullptr;
    QCheckBox      *chkSync1_ = nullptr;
    QSpinBox       *spinSync1Cycle_ = nullptr;
    QSpinBox       *spinSync1Shift_ = nullptr;
    QPushButton    *btnApplyDevice_ = nullptr;

    // Channel Mapping 表
    QTableWidget   *channelTable_ = nullptr;
    // SDO 表
    QTableWidget   *sdoTable_ = nullptr;

    // Master config (Advanced)
    QCheckBox      *chkMasterEnable_ = nullptr;
    QComboBox      *comboInterface_ = nullptr;
    QSpinBox       *spinCycleUs_ = nullptr;
    QSpinBox       *spinTaskPriority_ = nullptr;
    QSpinBox       *spinWatchdogCycles_ = nullptr;
    QPushButton    *btnApplyMaster_ = nullptr;
};
