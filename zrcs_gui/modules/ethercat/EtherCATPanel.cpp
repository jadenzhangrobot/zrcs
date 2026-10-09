#include "ethercat/EtherCATPanel.h"
#include "ui_ethercat_panel.h"

#include <QDebug>
#include <QSettings>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QHeaderView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QToolBar>
#include <QMap>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <algorithm>

namespace {

// OpenPLC Editor 中常用的 EtherCAT 从站设备类别目录 —— 模拟 ESI Repository
struct RepositoryDevice {
    const char *vendorName;
    quint32     vendorId;
    quint32     productCode;
    quint32     revision;
    const char *name;
    const char *group;
    int         inputChannels;
    int         outputChannels;
};
const RepositoryDevice kRepository[] = {
    {"ZRCS Drives",  0x000003A8, 0x00001000, 0x00000001, "ZRCS-AC Servo Drive",       "SERVO_DRIVE",       0, 2},
    {"ZRCS Drives",  0x000003A8, 0x00001001, 0x00000001, "ZRCS-Servo2 Drive",         "SERVO_DRIVE",       0, 2},
    {"ZRCS Drives",  0x000003A8, 0x00002000, 0x00000001, "ZRCS-EC Servo Motor",       "MOTOR",             0, 0},
    {"ZRCS IO",      0x000003A8, 0x00003000, 0x00000001, "ZRCS-EC Coupler",           "COUPLER",           8, 8},
    {"ZRCS IO",      0x000003A8, 0x00004001, 0x00000001, "ZRCS DI8",                  "DIGITAL_INPUT",     8, 0},
    {"ZRCS IO",      0x000003A8, 0x00004002, 0x00000001, "ZRCS DO8",                  "DIGITAL_OUTPUT",    0, 8},
    {"ZRCS IO",      0x000003A8, 0x00004003, 0x00000001, "ZRCS AI4",                  "ANALOG_INPUT",      4, 0},
    {"ZRCS IO",      0x000003A8, 0x00004004, 0x00000001, "ZRCS AO4",                  "ANALOG_OUTPUT",     0, 4},
    {"ZRCS IO",      0x000003A8, 0x00004005, 0x00000001, "ZRCS DO16",                 "DIGITAL_OUTPUT",    0, 16},
    {"ZRCS IO",      0x000003A8, 0x00004006, 0x00000001, "ZRCS DI16",                 "DIGITAL_INPUT",     16, 0},
    {"ZRCS Enc",     0x000003A8, 0x00005000, 0x00000001, "ZRCS Encoder",              "ENCODER",           1, 0},
    {"ZRCS Network", 0x000003A8, 0x00006000, 0x00000001, "ZRCS Fiber Coupler",        "COUPLER",           0, 0},
};

QString hex4(quint32 v)  { return QStringLiteral("0x%1").arg(v, 4, 16, QLatin1Char('0')).toUpper(); }
QString hex8(quint32 v)  { return QStringLiteral("0x%1").arg(v, 8, 16, QLatin1Char('0')).toUpper(); }
quint32 parseHex(const QString &text, quint32 def = 0)
{
    bool ok = false;
    quint32 v = text.toUInt(&ok, 0);
    return ok ? v : def;
}

QString slaveType(const RepositoryDevice &d)
{
    return QString::fromUtf8(d.group);
}

/**
 * @brief "Add Device from Repository" 对话框
 *        按厂商(可折叠)分组设备列表，带搜索框，与 openplc-editor 的 device-browser-modal 对应。
 */
class DeviceBrowserDialog : public QDialog {
    Q_OBJECT
public:
    DeviceBrowserDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Add Device from Repository"));
        resize(560, 460);

        auto *main = new QVBoxLayout(this);

        search_ = new QLineEdit(this);
        search_->setPlaceholderText(tr("Search devices by name, product code, or vendor..."));
        main->addWidget(search_);

        countLabel_ = new QLabel(this);
        main->addWidget(countLabel_);

        tree_ = new QTreeWidget(this);
        tree_->setHeaderLabels({tr("设备"), tr("Product Code"), tr("Rev"), tr("来源")});
        tree_->setColumnWidth(0, 240);
        main->addWidget(tree_);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
        applyBtn_ = buttons->button(QDialogButtonBox::Apply);
        applyBtn_->setText(tr("Add Device"));
        applyBtn_->setEnabled(false);
        main->addWidget(buttons);

        connect(search_, &QLineEdit::textChanged, this, &DeviceBrowserDialog::applyFilter);
        connect(tree_, &QTreeWidget::itemSelectionChanged, this, &DeviceBrowserDialog::onSelectionChanged);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

        rebuild();
    }

    int selectedDevice() const { return selectedDevice_; }

private slots:
    void rebuild()
    {
        tree_->clear();
        int totalDevices = 0;
        int vendorCount = 0;
        QMap<QString, QTreeWidgetItem*> vendorItems;
        const QList<QString> order = {QStringLiteral("ZRCS Drives"),
                                      QStringLiteral("ZRCS IO"),
                                      QStringLiteral("ZRCS Enc"),
                                      QStringLiteral("ZRCS Network")};
        for (const QString &vendor : order) {
            vendorItems[vendor] = nullptr;
        }
        for (int i = 0; i < int(sizeof(kRepository) / sizeof(kRepository[0])); ++i) {
            const RepositoryDevice &d = kRepository[i];
            QString vendor = QString::fromUtf8(d.vendorName);
            QTreeWidgetItem *vendorItem = vendorItems.value(vendor);
            if (!vendorItem) {
                vendorItem = new QTreeWidgetItem(tree_,
                    {QStringLiteral("%1 (%2)").arg(vendor, hex4(d.vendorId))});
                vendorItem->setExpanded(true);
                vendorItems[vendor] = vendorItem;
                ++vendorCount;
            }
            auto *devItem = new QTreeWidgetItem(vendorItem,
                {QString::fromUtf8(d.name),
                 hex8(d.productCode),
                 hex8(d.revision),
                 tr("repository")});
            devItem->setData(0, Qt::UserRole, i);
            ++totalDevices;
        }
        countLabel_->setText(tr("%1 device(s) in %2 vendor(s)").arg(totalDevices).arg(vendorCount));
        applyFilter();
    }

    void applyFilter()
    {
        const QString q = search_->text().trimmed().toLower();
        int total = 0;
        for (int v = 0; v < tree_->topLevelItemCount(); ++v) {
            QTreeWidgetItem *vendor = tree_->topLevelItem(v);
            int shown = 0;
            for (int c = 0; c < vendor->childCount(); ++c) {
                QTreeWidgetItem *dev = vendor->child(c);
                bool match = q.isEmpty();
                if (!match) {
                    const int idx = dev->data(0, Qt::UserRole).toInt();
                    const RepositoryDevice &d = kRepository[idx];
                    match = QString::fromUtf8(d.name).toLower().contains(q)
                         || hex8(d.productCode).toLower().contains(q)
                         || hex8(d.vendorId).toLower().contains(q)
                         || QString::fromUtf8(d.vendorName).toLower().contains(q);
                }
                dev->setHidden(!match);
                if (match) ++shown;
            }
            vendor->setHidden(shown == 0);
            total += shown;
        }
        countLabel_->setText(tr("%1 device(s)").arg(total));
    }

    void onSelectionChanged()
    {
        auto *item = tree_->currentItem();
        selectedDevice_ = -1;
        if (item && item->parent()) {
            selectedDevice_ = item->data(0, Qt::UserRole).toInt();
        }
        applyBtn_->setEnabled(selectedDevice_ >= 0);
    }

private:
    QLineEdit *search_ = nullptr;
    QLabel *countLabel_ = nullptr;
    QTreeWidget *tree_ = nullptr;
    QPushButton *applyBtn_ = nullptr;
    int selectedDevice_ = -1;
};

QString dialogProjectProgramDir()
{
    // 复用与行为树一致的工程 program 目录约定
    QStringList seeds;
    seeds << QCoreApplication::applicationDirPath() << QDir::currentPath();
    QString projectRoot;
    for (const QString &seed : seeds) {
        QDir dir(seed);
        for (int i = 0; i < 8; ++i) {
            const QString candidate = dir.filePath(QStringLiteral("config/project.txt"));
            if (QFileInfo::exists(candidate)) { projectRoot = dir.absolutePath(); break; }
            if (!dir.cdUp()) break;
        }
        if (!projectRoot.isEmpty()) break;
    }
    if (projectRoot.isEmpty()) return QDir::currentPath();
    QFile pf(QDir(projectRoot).filePath(QStringLiteral("config/project.txt")));
    QString projectName;
    if (pf.open(QIODevice::ReadOnly | QIODevice::Text)) projectName = QString::fromUtf8(pf.readLine()).trimmed();
    if (projectName.isEmpty()) return QDir(projectRoot).filePath(QStringLiteral("config"));
    const QString programDir = QDir(projectRoot).filePath(QStringLiteral("config/%1/program").arg(projectName));
    if (QDir(programDir).exists()) return QDir(programDir).absolutePath();
    const QString projectDir = QDir(projectRoot).filePath(QStringLiteral("config/%1").arg(projectName));
    if (QDir(projectDir).exists()) return QDir(projectDir).absolutePath();
    return QDir(projectRoot).filePath(QStringLiteral("config"));
}

} // namespace

EtherCATPanel::EtherCATPanel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EtherCATPanelUi)
{
    ui->setupUi(this);
    setupUI();
    setupConnections();
    rebuildDeviceTable();
    populateRepository();
    clearDeviceDetail();
    refreshStatus();
}

EtherCATPanel::~EtherCATPanel()
{
    delete ui;
}

QString EtherCATPanel::defaultProgramDirectory()
{
    return dialogProjectProgramDir();
}

QString EtherCATPanel::dialogStartDirectory(const QString &settingsKey) const
{
    QSettings settings;
    const QString remembered = settings.value(settingsKey).toString();
    const QString programDir = defaultProgramDirectory();
    if (remembered.isEmpty() || !QDir(remembered).exists()) return programDir;
    return QDir::cleanPath(remembered);
}

bool EtherCATPanel::event(QEvent *e)
{
    // 左栏表头是纯 QLabel（约 14px），右栏表头是 QLabel + 按钮（约 31px），
    // 两栏表格顶边因此错位。按钮高度随样式表变化（本控件构造时样式尚未应用），
    // 所以在样式变化和首次显示时重新对齐。
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::Show) {
        if (auto *label = findChild<QLabel *>("lblScanned")) {
            if (auto *button = findChild<QPushButton *>("btnAddConfigured")) {
                label->setMinimumHeight(button->sizeHint().height());
            }
        }
    }
    return QWidget::event(e);
}

void EtherCATPanel::setupUI()
{
    toolBar_        = findChild<QToolBar*>("toolBar");
    masterTabs_     = findChild<QTabWidget*>("masterTabs");
    deviceTable_    = findChild<QTableWidget*>("deviceTable");
    scannedTree_    = findChild<QTreeWidget*>("scannedTree");
    interfaceCombo_ = findChild<QComboBox*>("interfaceCombo");
    ecatStatusLabel_ = findChild<QLabel*>("ecatStatusLabel");
    repositoryTable_ = findChild<QTableWidget*>("repositoryTable");

    detailName_        = findChild<QLabel*>("detailName");
    detailType_        = findChild<QLabel*>("detailType");
    detailVendor_      = findChild<QLabel*>("detailVendor");
    detailVendorId_    = findChild<QLabel*>("detailVendorId");
    detailProductCode_ = findChild<QLabel*>("detailProductCode");
    detailRevision_    = findChild<QLabel*>("detailRevision");
    detailGroup_       = findChild<QLabel*>("detailGroup");
    detailChannels_    = findChild<QLabel*>("detailChannels");

    chkCheckVendor_   = findChild<QCheckBox*>("chkCheckVendor");
    chkCheckProduct_  = findChild<QCheckBox*>("chkCheckProduct");
    spinEcatAddress_  = findChild<QSpinBox*>("spinEcatAddress");
    spinSdoTimeout_   = findChild<QSpinBox*>("spinSdoTimeout");
    spinInitPreOp_    = findChild<QSpinBox*>("spinInitPreOp");
    spinSafeOpOp_     = findChild<QSpinBox*>("spinSafeOpOp");
    chkSmWatchdog_    = findChild<QCheckBox*>("chkSmWatchdog");
    spinSmWatchdogMs_ = findChild<QSpinBox*>("spinSmMs");
    chkPdiWatchdog_   = findChild<QCheckBox*>("chkPdiWatchdog");
    spinPdiWatchdogMs_= findChild<QSpinBox*>("spinPdiMs");
    chkDc_            = findChild<QCheckBox*>("chkDc");
    spinDcSyncUnit_   = findChild<QSpinBox*>("spinDcSyncUnit");
    chkSync0_         = findChild<QCheckBox*>("chkSync0");
    spinSync0Cycle_   = findChild<QSpinBox*>("spinSync0Cycle");
    spinSync0Shift_   = findChild<QSpinBox*>("spinSync0Shift");
    chkSync1_         = findChild<QCheckBox*>("chkSync1");
    spinSync1Cycle_   = findChild<QSpinBox*>("spinSync1Cycle");
    spinSync1Shift_   = findChild<QSpinBox*>("spinSync1Shift");
    btnApplyDevice_   = findChild<QPushButton*>("btnApplyDevice");

    channelTable_ = findChild<QTableWidget*>("channelTable");
    sdoTable_     = findChild<QTableWidget*>("sdoTable");

    chkMasterEnable_  = findChild<QCheckBox*>("chkMasterEnable");
    comboInterface_   = findChild<QComboBox*>("comboInterface");
    spinCycleUs_      = findChild<QSpinBox*>("spinCycleUs");
    spinTaskPriority_ = findChild<QSpinBox*>("spinTaskPriority");
    spinWatchdogCycles_ = findChild<QSpinBox*>("spinWatchdogCycles");
    btnApplyMaster_   = findChild<QPushButton*>("btnApplyMaster");

    for (QTableWidget *t : {deviceTable_, channelTable_, sdoTable_, repositoryTable_}) {
        if (t) {
            t->horizontalHeader()->setStretchLastSection(true);
            t->verticalHeader()->setVisible(false);
            t->setSelectionBehavior(QAbstractItemView::SelectRows);
            t->setSelectionMode(QAbstractItemView::SingleSelection);
        }
    }
    if (deviceTable_) deviceTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    if (channelTable_) channelTable_->setEditTriggers(QAbstractItemView::AllEditTriggers);
    if (toolBar_) toolBar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 根据从站类型自动填充通道
    if (scannedTree_) {
        scannedTree_->setRootIsDecorated(false);
        scannedTree_->header()->setStretchLastSection(true);
    }
}

void EtherCATPanel::setupConnections()
{
    auto a = [this](const char *name, void (EtherCATPanel::*slot)()) {
        auto *act = findChild<QAction*>(QString::fromLatin1(name));
        if (act) connect(act, &QAction::triggered, this, slot);
    };
    a("actionAddDevice", &EtherCATPanel::onAddDevice);
    a("actionRemoveDevice", &EtherCATPanel::onRemoveDevice);
    a("actionMoveUp", &EtherCATPanel::onMoveUp);
    a("actionMoveDown", &EtherCATPanel::onMoveDown);
    a("actionImportEsi", &EtherCATPanel::onImportEsi);
    a("actionLoadNetwork", &EtherCATPanel::onLoadNetwork);
    a("actionSaveNetwork", &EtherCATPanel::onSaveNetwork);
    a("actionSendToController", &EtherCATPanel::onSendToController);

    if (auto *b = findChild<QPushButton*>("btnAddConfigured")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onAddDevice);
    if (auto *b = findChild<QPushButton*>("btnRemoveConfigured")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onRemoveDevice);
    if (auto *b = findChild<QPushButton*>("btnScan")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onScanDevice);
    if (auto *b = findChild<QPushButton*>("btnImportEsi")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onImportEsi);
    if (auto *b = findChild<QPushButton*>("btnClearRepo")) connect(b, &QPushButton::clicked, this, [this]() { repositoryTable_->setRowCount(0); refreshStatus(); });
    if (auto *b = findChild<QPushButton*>("btnApplyDevice")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onApplyDeviceConfig);
    if (auto *b = findChild<QPushButton*>("btnApplyMaster")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onApplyMasterConfig);
    if (auto *b = findChild<QPushButton*>("btnAddChannel")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onAddChannelMapping);
    if (auto *b = findChild<QPushButton*>("btnRemoveChannel")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onRemoveChannelMapping);
    if (auto *b = findChild<QPushButton*>("btnAddSdo")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onAddSdoEntry);
    if (auto *b = findChild<QPushButton*>("btnRemoveSdo")) connect(b, &QPushButton::clicked, this, &EtherCATPanel::onRemoveSdoEntry);

    if (deviceTable_) {
        connect(deviceTable_, &QTableWidget::itemSelectionChanged, this, [this]() {
            onDeviceSelected(deviceTable_->currentRow());
        });
    }
    if (chkDc_) connect(chkDc_, &QCheckBox::toggled, this, &EtherCATPanel::onEnableDcToggled);

    // 主站默认值装载
    if (chkMasterEnable_) chkMasterEnable_->setChecked(masterConfig_.enabled);
    if (comboInterface_) { comboInterface_->setCurrentText(masterConfig_.networkInterface); }
    if (spinCycleUs_) spinCycleUs_->setValue(masterConfig_.cycleTimeUs);
    if (spinTaskPriority_) spinTaskPriority_->setValue(masterConfig_.taskPriority);
    if (spinWatchdogCycles_) spinWatchdogCycles_->setValue(masterConfig_.watchdogTimeoutCycles);
}

void EtherCATPanel::populateRepository()
{
    if (!repositoryTable_) return;
    repositoryTable_->setRowCount(0);
    QMap<QString, QPair<QString, int>> files; // vendor -> (vendorIdHex, count)
    for (const auto &d : kRepository) {
        QString vendor = QString::fromUtf8(d.vendorName);
        auto &e = files[vendor];
        e.first = hex4(d.vendorId);
        e.second += 1;
    }
    int row = 0;
    for (auto it = files.constBegin(); it != files.constEnd(); ++it, ++row) {
        repositoryTable_->insertRow(row);
        repositoryTable_->setItem(row, 0, new QTableWidgetItem(QStringLiteral("%1.esi").arg(it.key().toLower().replace(QLatin1Char(' '), QLatin1Char('-')))));
        repositoryTable_->setItem(row, 1, new QTableWidgetItem(QStringLiteral("%1 (%2)").arg(it.key(), it.value().first)));
        repositoryTable_->setItem(row, 2, new QTableWidgetItem(QString::number(it.value().second)));
    }
    if (auto *lbl = findChild<QLabel*>("lblRepo")) {
        lbl->setText(tr("已加载文件 (%1) · ESI 设备库").arg(files.size()));
    }
}

void EtherCATPanel::rebuildDeviceTable()
{
    if (!deviceTable_) return;
    deviceTable_->setRowCount(static_cast<int>(devices_.size()));
    for (int i = 0; i < devices_.size(); ++i) {
        const EtherCATDevice &d = devices_[i];
        auto mk = [](const QString &t) {
            auto *it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignCenter);
            return it;
        };
        deviceTable_->setItem(i, 0, new QTableWidgetItem(d.name));
        deviceTable_->setItem(i, 1, new QTableWidgetItem(d.type));
        deviceTable_->setItem(i, 2, mk(QString::number(d.position)));
        deviceTable_->setItem(i, 3, mk(QStringLiteral("%1 / %2").arg(d.inputChannels).arg(d.outputChannels)));
        deviceTable_->setItem(i, 4, mk(d.source));
    }
}

void EtherCATPanel::rebuildChannelTable()
{
    if (!channelTable_) return;
    channelTable_->setRowCount(0);
    if (currentDeviceRow_ < 0 || currentDeviceRow_ >= devices_.size()) return;
    const auto &mappings = devices_[currentDeviceRow_].channelMappings;
    int inIdx = 0, outIdx = 0;
    for (const auto &m : mappings) {
        const int row = channelTable_->rowCount();
        channelTable_->insertRow(row);
        const QString num = (m.direction == QLatin1String("output")) ? QString::number(++outIdx) : QString::number(++inIdx);
        auto mk = [](const QString &t) { auto *it = new QTableWidgetItem(t); it->setTextAlignment(Qt::AlignCenter); return it; };
        channelTable_->setItem(row, 0, mk(num));
        channelTable_->setItem(row, 1, mk(m.direction == QLatin1String("output") ? tr("Output") : tr("Input")));
        channelTable_->setItem(row, 2, new QTableWidgetItem(m.iecType));
        channelTable_->setItem(row, 3, new QTableWidgetItem(m.iecLocation));
        channelTable_->setItem(row, 4, new QTableWidgetItem(m.alias));
    }
}

void EtherCATPanel::rebuildSdoTable()
{
    if (!sdoTable_) return;
    sdoTable_->setRowCount(0);
    if (currentDeviceRow_ < 0 || currentDeviceRow_ >= devices_.size()) return;
    const auto &sdo = devices_[currentDeviceRow_].sdoConfigurations;
    for (const auto &s : sdo) {
        const int row = sdoTable_->rowCount();
        sdoTable_->insertRow(row);
        auto mk = [](const QString &t) { auto *it = new QTableWidgetItem(t); it->setTextAlignment(Qt::AlignCenter); return it; };
        sdoTable_->setItem(row, 0, mk(QStringLiteral("0x%1").arg(s.index, 4, 16, QLatin1Char('0')).toUpper()));
        sdoTable_->setItem(row, 1, mk(QString::number(s.subIndex)));
        sdoTable_->setItem(row, 2, new QTableWidgetItem(s.value));
        sdoTable_->setItem(row, 3, mk(s.dataType));
        sdoTable_->setItem(row, 4, new QTableWidgetItem(s.name));
    }
}

void EtherCATPanel::setCurrentDevice(const EtherCATDevice &d)
{
    if (detailName_)        detailName_->setText(d.name);
    if (detailType_)        detailType_->setText(d.type);
    if (detailVendor_)      detailVendor_->setText(d.vendorName);
    if (detailVendorId_)    detailVendorId_->setText(hex4(d.vendorId));
    if (detailProductCode_) detailProductCode_->setText(hex8(d.productCode));
    if (detailRevision_)    detailRevision_->setText(hex8(d.revisionNo));
    if (detailGroup_)       detailGroup_->setText(d.groupName);
    if (detailChannels_)    detailChannels_->setText(QStringLiteral("%1 / %2").arg(d.inputChannels).arg(d.outputChannels));

    // Configuration 表单
    if (chkCheckVendor_)   chkCheckVendor_->setChecked(d.config.checkVendorId);
    if (chkCheckProduct_)  chkCheckProduct_->setChecked(d.config.checkProductCode);
    if (spinEcatAddress_)  spinEcatAddress_->setValue(d.config.ethercatAddress);
    if (spinSdoTimeout_)   spinSdoTimeout_->setValue(d.config.sdoTimeoutMs);
    if (spinInitPreOp_)    spinInitPreOp_->setValue(d.config.initToPreOpTimeoutMs);
    if (spinSafeOpOp_)     spinSafeOpOp_->setValue(d.config.safeOpToOpTimeoutMs);
    if (chkSmWatchdog_)    chkSmWatchdog_->setChecked(d.config.smWatchdogEnabled);
    if (spinSmWatchdogMs_) spinSmWatchdogMs_->setValue(d.config.smWatchdogMs);
    if (chkPdiWatchdog_)   chkPdiWatchdog_->setChecked(d.config.pdiWatchdogEnabled);
    if (spinPdiWatchdogMs_) spinPdiWatchdogMs_->setValue(d.config.pdiWatchdogMs);
    if (chkDc_)            chkDc_->setChecked(d.config.dcEnabled);
    if (spinDcSyncUnit_)   spinDcSyncUnit_->setValue(d.config.dcSyncUnitCycleUs);
    if (chkSync0_)         chkSync0_->setChecked(d.config.dcSync0Enabled);
    if (spinSync0Cycle_)   spinSync0Cycle_->setValue(d.config.dcSync0CycleUs);
    if (spinSync0Shift_)   spinSync0Shift_->setValue(d.config.dcSync0ShiftUs);
    if (chkSync1_)         chkSync1_->setChecked(d.config.dcSync1Enabled);
    if (spinSync1Cycle_)   spinSync1Cycle_->setValue(d.config.dcSync1CycleUs);
    if (spinSync1Shift_)   spinSync1Shift_->setValue(d.config.dcSync1ShiftUs);
    onEnableDcToggled(d.config.dcEnabled);

    rebuildChannelTable();
    rebuildSdoTable();
}

void EtherCATPanel::clearDeviceDetail()
{
    currentDeviceRow_ = -1;
    for (QLabel *l : {detailName_, detailType_, detailVendor_, detailVendorId_,
                       detailProductCode_, detailRevision_, detailGroup_, detailChannels_}) {
        if (l) l->setText(QStringLiteral("--"));
    }
    if (channelTable_) channelTable_->setRowCount(0);
    if (sdoTable_) sdoTable_->setRowCount(0);
}

int EtherCATPanel::currentDeviceIndex() const
{
    return currentDeviceRow_;
}

void EtherCATPanel::onDeviceSelected(int row)
{
    if (row < 0 || row >= devices_.size()) { clearDeviceDetail(); return; }
    currentDeviceRow_ = row;
    setCurrentDevice(devices_[row]);
}

void EtherCATPanel::onAddDevice()
{
    DeviceBrowserDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    const int idx = dlg.selectedDevice();
    if (idx < 0 || idx >= int(sizeof(kRepository) / sizeof(kRepository[0]))) return;
    const RepositoryDevice &d = kRepository[idx];

    EtherCATDevice dev;
    dev.name = QString::fromUtf8(d.name);
    dev.type = slaveType(d);
    dev.vendorName = QString::fromUtf8(d.vendorName);
    dev.groupName = QString::fromUtf8(d.group);
    dev.source = QStringLiteral("Manual");
    int maxPos = 0;
    for (const auto &e : devices_) maxPos = qMax(maxPos, e.position);
    dev.position = maxPos + 1;
    dev.vendorId = d.vendorId;
    dev.productCode = d.productCode;
    dev.revisionNo = d.revision;
    dev.inputChannels = d.inputChannels;
    dev.outputChannels = d.outputChannels;

    // 默认从站配置 (DEFAULT_SLAVE_CONFIG)
    dev.config.checkVendorId = true;
    dev.config.checkProductCode = true;
    dev.config.ethercatAddress = 0;
    dev.config.sdoTimeoutMs = 1000;
    dev.config.initToPreOpTimeoutMs = 3000;
    dev.config.safeOpToOpTimeoutMs = 10000;
    dev.config.smWatchdogEnabled = true;
    dev.config.smWatchdogMs = 100;
    dev.config.pdiWatchdogEnabled = false;
    dev.config.pdiWatchdogMs = 100;

    devices_.append(dev);
    currentDeviceRow_ = devices_.size() - 1;
    rebuildDeviceTable();
    if (deviceTable_) deviceTable_->selectRow(currentDeviceRow_);
    setCurrentDevice(dev);
    refreshStatus();
}

void EtherCATPanel::onRemoveDevice()
{
    if (currentDeviceRow_ < 0 || currentDeviceRow_ >= devices_.size()) return;
    devices_.remove(currentDeviceRow_);
    rebuildDeviceTable();
    clearDeviceDetail();
    refreshStatus();
}

void EtherCATPanel::onMoveUp()
{
    if (currentDeviceRow_ <= 0) return;
    devices_.move(currentDeviceRow_, currentDeviceRow_ - 1);
    --currentDeviceRow_;
    rebuildDeviceTable();
    if (deviceTable_) deviceTable_->selectRow(currentDeviceRow_);
}

void EtherCATPanel::onMoveDown()
{
    if (currentDeviceRow_ < 0 || currentDeviceRow_ >= devices_.size() - 1) return;
    devices_.move(currentDeviceRow_, currentDeviceRow_ + 1);
    ++currentDeviceRow_;
    rebuildDeviceTable();
    if (deviceTable_) deviceTable_->selectRow(currentDeviceRow_);
}

void EtherCATPanel::onScanDevice()
{
    if (!scannedTree_) return;
    scannedTree_->clear();
    // 模拟在线扫描：将设备库设备按顺序列出为扫描结果
    int pos = 1;
    for (const auto &d : kRepository) {
        auto *it = new QTreeWidgetItem(scannedTree_,
            {QString::number(pos++),
             QString::fromUtf8(d.name),
             hex4(d.vendorId),
             hex8(d.productCode)});
        it->setCheckState(0, Qt::Unchecked);
    }
    scannedTree_->expandAll();
    refreshStatus();
}

void EtherCATPanel::onAddSelectedDevices()
{
    if (!scannedTree_) return;
    for (int i = 0; i < scannedTree_->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = scannedTree_->topLevelItem(i);
        if (!it) continue;
        if (it->checkState(0) != Qt::Checked) continue;
        const QString name = it->text(1);
        for (const auto &d : kRepository) {
            if (QString::fromUtf8(d.name) == name) {
                bool exists = false;
                for (const auto &e : devices_) {
                    if (e.productCode == d.productCode && e.vendorId == d.vendorId) { exists = true; break; }
                }
                if (exists) break;
                EtherCATDevice dev;
                dev.name = QString::fromUtf8(d.name);
                dev.type = slaveType(d);
                dev.vendorName = QString::fromUtf8(d.vendorName);
                dev.groupName = QString::fromUtf8(d.group);
                dev.source = QStringLiteral("Scan");
                dev.position = scanNextPosition();
                dev.vendorId = d.vendorId;
                dev.productCode = d.productCode;
                dev.revisionNo = d.revision;
                dev.inputChannels = d.inputChannels;
                dev.outputChannels = d.outputChannels;
                devices_.append(dev);
                break;
            }
        }
    }
    rebuildDeviceTable();
    refreshStatus();
}

void EtherCATPanel::onApplyDeviceConfig()
{
    if (currentDeviceRow_ < 0 || currentDeviceRow_ >= devices_.size()) return;
    EtherCATDevice &d = devices_[currentDeviceRow_];
    if (chkCheckVendor_)   d.config.checkVendorId = chkCheckVendor_->isChecked();
    if (chkCheckProduct_)  d.config.checkProductCode = chkCheckProduct_->isChecked();
    if (spinEcatAddress_)  d.config.ethercatAddress = spinEcatAddress_->value();
    if (spinSdoTimeout_)   d.config.sdoTimeoutMs = spinSdoTimeout_->value();
    if (spinInitPreOp_)    d.config.initToPreOpTimeoutMs = spinInitPreOp_->value();
    if (spinSafeOpOp_)     d.config.safeOpToOpTimeoutMs = spinSafeOpOp_->value();
    if (chkSmWatchdog_)    d.config.smWatchdogEnabled = chkSmWatchdog_->isChecked();
    if (spinSmWatchdogMs_) d.config.smWatchdogMs = spinSmWatchdogMs_->value();
    if (chkPdiWatchdog_)   d.config.pdiWatchdogEnabled = chkPdiWatchdog_->isChecked();
    if (spinPdiWatchdogMs_) d.config.pdiWatchdogMs = spinPdiWatchdogMs_->value();
    if (chkDc_)            d.config.dcEnabled = chkDc_->isChecked();
    if (spinDcSyncUnit_)   d.config.dcSyncUnitCycleUs = spinDcSyncUnit_->value();
    if (chkSync0_)         d.config.dcSync0Enabled = chkSync0_->isChecked();
    if (spinSync0Cycle_)   d.config.dcSync0CycleUs = spinSync0Cycle_->value();
    if (spinSync0Shift_)   d.config.dcSync0ShiftUs = spinSync0Shift_->value();
    if (chkSync1_)         d.config.dcSync1Enabled = chkSync1_->isChecked();
    if (spinSync1Cycle_)   d.config.dcSync1CycleUs = spinSync1Cycle_->value();
    if (spinSync1Shift_)   d.config.dcSync1ShiftUs = spinSync1Shift_->value();

    // 从 channelTable 写回别名
    if (channelTable_) {
        int inIdx = 0, outIdx = 0;
        for (int i = 0; i < d.channelMappings.size(); ++i) {
            const bool out = d.channelMappings[i].direction == QLatin1String("output");
            const int tblRow = out ? (outIdx) : (inIdx);
            if (tblRow < channelTable_->rowCount()) {
                if (auto *alias = channelTable_->item(tblRow, 4)) {
                    d.channelMappings[i].alias = alias->text();
                }
            }
            if (out) ++outIdx; else ++inIdx;
        }
    }
    rebuildDeviceTable();
    rebuildChannelTable();
    rebuildSdoTable();
    refreshStatus();
}

void EtherCATPanel::onEnableDcToggled(bool checked)
{
    const bool enable = checked;
    for (QWidget *w : {spinDcSyncUnit_}) if (w) w->setEnabled(enable);
    for (QWidget *w : std::initializer_list<QWidget *>{
             chkSync0_, chkSync1_, spinSync0Cycle_, spinSync0Shift_,
             spinSync1Cycle_, spinSync1Shift_}) if (w) w->setEnabled(enable);
}

void EtherCATPanel::onApplyMasterConfig()
{
    if (chkMasterEnable_)  masterConfig_.enabled = chkMasterEnable_->isChecked();
    if (comboInterface_)   masterConfig_.networkInterface = comboInterface_->currentText();
    if (spinCycleUs_)      masterConfig_.cycleTimeUs = spinCycleUs_->value();
    if (spinTaskPriority_) masterConfig_.taskPriority = spinTaskPriority_->value();
    if (spinWatchdogCycles_) masterConfig_.watchdogTimeoutCycles = spinWatchdogCycles_->value();
    if (interfaceCombo_)   interfaceCombo_->setCurrentText(masterConfig_.networkInterface);
    refreshStatus();
}

void EtherCATPanel::onImportEsi()
{
    // 从设备库导入：直接打开添加对话框；也可扩展为解析 .esi XML (ETG.2000)
    onAddDevice();
}

void EtherCATPanel::onAddChannelMapping()
{
    if (currentDeviceRow_ < 0) return;
    EtherCATDevice &d = devices_[currentDeviceRow_];
    EtherCATChannelMapping m;
    const int inputCount = std::count_if(d.channelMappings.constBegin(), d.channelMappings.constEnd(),
        [](const EtherCATChannelMapping &c) { return c.direction == QLatin1String("input"); });
    const int outputCount = d.channelMappings.size() - inputCount;
    if (inputCount <= outputCount) {
        m.direction = "input";
        m.channelId = inputCount;
        m.iecType = QStringLiteral("BOOL");
        m.iecLocation = QStringLiteral("%IX%1.%2").arg(d.position).arg(inputCount);
    } else {
        m.direction = "output";
        m.channelId = outputCount;
        m.iecType = QStringLiteral("BOOL");
        m.iecLocation = QStringLiteral("%QX%1.%2").arg(d.position).arg(outputCount);
    }
    d.channelMappings.append(m);
    rebuildChannelTable();
}

void EtherCATPanel::onRemoveChannelMapping()
{
    if (currentDeviceRow_ < 0 || !channelTable_ || channelTable_->currentRow() < 0) return;
    EtherCATDevice &d = devices_[currentDeviceRow_];
    const int target = channelTable_->currentRow();
    int inIdx = 0, outIdx = 0;
    for (int i = 0; i < d.channelMappings.size(); ++i) {
        const bool out = d.channelMappings[i].direction == QLatin1String("output");
        const int row = out ? outIdx : inIdx;
        if (row == target) { d.channelMappings.remove(i); break; }
        if (out) ++outIdx; else ++inIdx;
    }
    rebuildChannelTable();
}

void EtherCATPanel::onAddSdoEntry()
{
    if (currentDeviceRow_ < 0) return;
    SDOConfigEntry e;
    e.index = 0x6060;
    e.subIndex = 0;
    e.value = QStringLiteral("0");
    e.dataType = QStringLiteral("UINT8");
    e.name = QStringLiteral("Modes of operation");
    devices_[currentDeviceRow_].sdoConfigurations.append(e);
    rebuildSdoTable();
}

void EtherCATPanel::onRemoveSdoEntry()
{
    if (currentDeviceRow_ < 0 || !sdoTable_ || sdoTable_->currentRow() < 0) return;
    devices_[currentDeviceRow_].sdoConfigurations.remove(sdoTable_->currentRow());
    rebuildSdoTable();
}

void EtherCATPanel::onLoadNetwork()
{
    const QString start = dialogStartDirectory(QStringLiteral("EtherCATPanel.lastLoadDirectory"));
    const QString fileName = QFileDialog::getOpenFileName(this, tr("加载 EtherCAT 组态"),
        start, tr("EtherCAT 组态文件 (*.xml)"));
    if (!QFileInfo::exists(fileName)) return;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    const QString xml = QString::fromUtf8(file.readAll());
    QSettings s2; s2.setValue("EtherCATPanel.lastLoadDirectory", QFileInfo(fileName).absolutePath());
    QString error;
    if (!loadFromXML(xml, &error)) {
        QMessageBox::warning(this, tr("加载失败"), tr("无法解析 EtherCAT 组态：\n%1").arg(error));
    }
}

void EtherCATPanel::onSaveNetwork()
{
    QString directory = dialogStartDirectory(QStringLiteral("EtherCATPanel.lastSaveDirectory"));
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存 EtherCAT 组态"),
        directory, tr("EtherCAT 组态文件 (*.xml)"));
    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(QStringLiteral(".xml"))) fileName += QStringLiteral(".xml");
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << saveToXML();
    }
    QSettings s2; s2.setValue("EtherCATPanel.lastSaveDirectory", QFileInfo(fileName).absolutePath());
}

void EtherCATPanel::onSendToController()
{
    const QString xml = saveToXML();
    if (xml.isEmpty()) return;
    emit applyEthercatRequested(xml);
    qDebug() << "[EtherCATPanel] Sent EtherCAT config to controller";
}

QString EtherCATPanel::saveToXML() const
{
    QDomDocument doc;
    QDomElement root = doc.createElement("EtherCATConfiguration");
    root.setAttribute("version", "1.0");
    doc.appendChild(root);

    // Master
    QDomElement master = doc.createElement("Master");
    master.setAttribute("Enabled", masterConfig_.enabled ? "true" : "false");
    master.appendChild(doc.createElement("NetworkInterface"))
          .appendChild(doc.createTextNode(masterConfig_.networkInterface));
    master.appendChild(doc.createElement("CycleTimeUs"))
          .appendChild(doc.createTextNode(QString::number(masterConfig_.cycleTimeUs)));
    master.appendChild(doc.createElement("TaskPriority"))
          .appendChild(doc.createTextNode(QString::number(masterConfig_.taskPriority)));
    master.appendChild(doc.createElement("WatchdogTimeoutCycles"))
          .appendChild(doc.createTextNode(QString::number(masterConfig_.watchdogTimeoutCycles)));
    root.appendChild(master);

    for (const auto &s : devices_) {
        QDomElement slave = doc.createElement("Slave");
        slave.setAttribute("Position", s.position);
        slave.setAttribute("Name", s.name);
        slave.setAttribute("Type", s.type);
        slave.setAttribute("Vendor", s.vendorName);
        slave.setAttribute("Source", s.source);
        slave.appendChild(doc.createElement("VendorId")).appendChild(doc.createTextNode(hex8(s.vendorId)));
        slave.appendChild(doc.createElement("ProductCode")).appendChild(doc.createTextNode(hex8(s.productCode)));
        slave.appendChild(doc.createElement("RevisionNo")).appendChild(doc.createTextNode(hex8(s.revisionNo)));

        QDomElement cfg = doc.createElement("Config");
        QDomElement sc = doc.createElement("StartupChecks");
        sc.setAttribute("CheckVendorId", s.config.checkVendorId ? "true" : "false");
        sc.setAttribute("CheckProductCode", s.config.checkProductCode ? "true" : "false");
        cfg.appendChild(sc);
        QDomElement ad = doc.createElement("Addressing");
        ad.setAttribute("EthercatAddress", s.config.ethercatAddress);
        cfg.appendChild(ad);
        QDomElement tm = doc.createElement("Timeouts");
        tm.setAttribute("SdoMs", s.config.sdoTimeoutMs);
        tm.setAttribute("InitToPreOpMs", s.config.initToPreOpTimeoutMs);
        tm.setAttribute("SafeOpToOpMs", s.config.safeOpToOpTimeoutMs);
        cfg.appendChild(tm);
        QDomElement wd = doc.createElement("Watchdog");
        wd.setAttribute("SmEnabled", s.config.smWatchdogEnabled ? "true" : "false");
        wd.setAttribute("SmMs", s.config.smWatchdogMs);
        wd.setAttribute("PdiEnabled", s.config.pdiWatchdogEnabled ? "true" : "false");
        wd.setAttribute("PdiMs", s.config.pdiWatchdogMs);
        cfg.appendChild(wd);
        QDomElement dc = doc.createElement("DistributedClocks");
        dc.setAttribute("Enabled", s.config.dcEnabled ? "true" : "false");
        dc.setAttribute("SyncUnitCycleUs", s.config.dcSyncUnitCycleUs);
        dc.setAttribute("Sync0Enabled", s.config.dcSync0Enabled ? "true" : "false");
        dc.setAttribute("Sync0CycleUs", s.config.dcSync0CycleUs);
        dc.setAttribute("Sync0ShiftUs", s.config.dcSync0ShiftUs);
        dc.setAttribute("Sync1Enabled", s.config.dcSync1Enabled ? "true" : "false");
        dc.setAttribute("Sync1CycleUs", s.config.dcSync1CycleUs);
        dc.setAttribute("Sync1ShiftUs", s.config.dcSync1ShiftUs);
        cfg.appendChild(dc);
        slave.appendChild(cfg);

        // Channel mappings
        QDomElement chans = doc.createElement("Channels");
        for (const auto &m : s.channelMappings) {
            QDomElement ch = doc.createElement("Channel");
            ch.setAttribute("Id", m.channelId);
            ch.setAttribute("Direction", m.direction);
            ch.setAttribute("IecType", m.iecType);
            ch.setAttribute("IecLocation", m.iecLocation);
            ch.setAttribute("Alias", m.alias);
            chans.appendChild(ch);
        }
        slave.appendChild(chans);

        // SDO
        QDomElement sdoEl = doc.createElement("SdoConfigurations");
        for (const auto &sdo : s.sdoConfigurations) {
            QDomElement e = doc.createElement("Sdo");
            e.setAttribute("Index", QStringLiteral("0x%1").arg(sdo.index, 4, 16, QLatin1Char('0')));
            e.setAttribute("SubIndex", sdo.subIndex);
            e.setAttribute("Value", sdo.value);
            e.setAttribute("DataType", sdo.dataType);
            e.setAttribute("Name", sdo.name);
            sdoEl.appendChild(e);
        }
        slave.appendChild(sdoEl);

        root.appendChild(slave);
    }

    QString out;
    QTextStream stream(&out);
    doc.save(stream, 4);
    return out;
}

bool EtherCATPanel::loadFromXML(const QString &xmlText, QString *error)
{
    QDomDocument doc;
    if (!doc.setContent(xmlText, error)) return false;
    const QDomElement root = doc.documentElement();

    // Master
    EtherCATMasterConfig mc;
    const QDomElement master = root.firstChildElement("Master");
    if (!master.isNull()) {
        mc.enabled = master.attribute("Enabled") == QLatin1String("true");
        if (!master.firstChildElement("NetworkInterface").isNull())
            mc.networkInterface = master.firstChildElement("NetworkInterface").text();
        if (!master.firstChildElement("CycleTimeUs").isNull())
            mc.cycleTimeUs = master.firstChildElement("CycleTimeUs").text().toInt();
        if (!master.firstChildElement("TaskPriority").isNull())
            mc.taskPriority = master.firstChildElement("TaskPriority").text().toInt();
        if (!master.firstChildElement("WatchdogTimeoutCycles").isNull())
            mc.watchdogTimeoutCycles = master.firstChildElement("WatchdogTimeoutCycles").text().toInt();
    }
    masterConfig_ = mc;
    if (chkMasterEnable_) chkMasterEnable_->setChecked(mc.enabled);
    if (comboInterface_) comboInterface_->setCurrentText(mc.networkInterface);
    if (spinCycleUs_) spinCycleUs_->setValue(mc.cycleTimeUs);
    if (spinTaskPriority_) spinTaskPriority_->setValue(mc.taskPriority);
    if (spinWatchdogCycles_) spinWatchdogCycles_->setValue(mc.watchdogTimeoutCycles);
    if (interfaceCombo_) interfaceCombo_->setCurrentText(mc.networkInterface);

    QVector<EtherCATDevice> loaded;
    for (QDomElement sEl = root.firstChildElement("Slave"); !sEl.isNull();
         sEl = sEl.nextSiblingElement("Slave"))
    {
        EtherCATDevice d;
        d.position = sEl.attribute("Position").toInt();
        d.name = sEl.attribute("Name");
        d.type = sEl.attribute("Type");
        d.vendorName = sEl.attribute("Vendor");
        d.source = sEl.attribute("Source", QStringLiteral("Manual"));
        d.vendorId = parseHex(sEl.firstChildElement("VendorId").text(), 0);
        d.productCode = parseHex(sEl.firstChildElement("ProductCode").text(), 0);
        d.revisionNo = parseHex(sEl.firstChildElement("RevisionNo").text(), 0);

        const QDomElement cfg = sEl.firstChildElement("Config");
        if (!cfg.isNull()) {
            const QDomElement sc = cfg.firstChildElement("StartupChecks");
            d.config.checkVendorId = sc.attribute("CheckVendorId") != QLatin1String("false");
            d.config.checkProductCode = sc.attribute("CheckProductCode") != QLatin1String("false");
            const QDomElement ad = cfg.firstChildElement("Addressing");
            d.config.ethercatAddress = ad.attribute("EthercatAddress").toInt();
            const QDomElement tm = cfg.firstChildElement("Timeouts");
            d.config.sdoTimeoutMs = tm.attribute("SdoMs").toInt();
            d.config.initToPreOpTimeoutMs = tm.attribute("InitToPreOpMs").toInt();
            d.config.safeOpToOpTimeoutMs = tm.attribute("SafeOpToOpMs").toInt();
            const QDomElement wd = cfg.firstChildElement("Watchdog");
            d.config.smWatchdogEnabled = wd.attribute("SmEnabled") != QLatin1String("false");
            d.config.smWatchdogMs = wd.attribute("SmMs").toInt();
            d.config.pdiWatchdogEnabled = wd.attribute("PdiEnabled") == QLatin1String("true");
            d.config.pdiWatchdogMs = wd.attribute("PdiMs").toInt();
            const QDomElement dc = cfg.firstChildElement("DistributedClocks");
            d.config.dcEnabled = dc.attribute("Enabled") == QLatin1String("true");
            d.config.dcSyncUnitCycleUs = dc.attribute("SyncUnitCycleUs").toInt();
            d.config.dcSync0Enabled = dc.attribute("Sync0Enabled") == QLatin1String("true");
            d.config.dcSync0CycleUs = dc.attribute("Sync0CycleUs").toInt();
            d.config.dcSync0ShiftUs = dc.attribute("Sync0ShiftUs").toInt();
            d.config.dcSync1Enabled = dc.attribute("Sync1Enabled") == QLatin1String("true");
            d.config.dcSync1CycleUs = dc.attribute("Sync1CycleUs").toInt();
            d.config.dcSync1ShiftUs = dc.attribute("Sync1ShiftUs").toInt();
        }

        const QDomElement chans = sEl.firstChildElement("Channels");
        for (QDomElement ch = chans.firstChildElement("Channel"); !ch.isNull();
             ch = ch.nextSiblingElement("Channel"))
        {
            EtherCATChannelMapping m;
            m.channelId = ch.attribute("Id").toInt();
            m.direction = ch.attribute("Direction");
            m.iecType = ch.attribute("IecType");
            m.iecLocation = ch.attribute("IecLocation");
            m.alias = ch.attribute("Alias");
            d.channelMappings.append(m);
            if (m.direction == QLatin1String("output")) ++d.outputChannels; else ++d.inputChannels;
        }

        const QDomElement sdoEl = sEl.firstChildElement("SdoConfigurations");
        for (QDomElement e = sdoEl.firstChildElement("Sdo"); !e.isNull();
             e = e.nextSiblingElement("Sdo"))
        {
            SDOConfigEntry s;
            s.index = static_cast<quint16>(parseHex(e.attribute("Index"), 0));
            s.subIndex = static_cast<quint8>(e.attribute("SubIndex").toInt());
            s.value = e.attribute("Value");
            s.dataType = e.attribute("DataType");
            s.name = e.attribute("Name");
            d.sdoConfigurations.append(s);
        }

        loaded.append(d);
    }

    devices_ = loaded;
    rebuildDeviceTable();
    clearDeviceDetail();
    refreshStatus();
    return true;
}

void EtherCATPanel::refreshStatus()
{
    if (!ecatStatusLabel_) return;
    const int pos = scanNextPosition() - 1;
    ecatStatusLabel_->setText(
        tr("EtherCAT 组态：%1 个从站 · 主站 %2")
            .arg(devices_.size())
            .arg(masterConfig_.enabled ? tr("已启用") : tr("已禁用")));
    Q_UNUSED(pos);
}

int EtherCATPanel::scanNextPosition() const
{
    int maxPos = 0;
    for (const auto &e : devices_) maxPos = qMax(maxPos, e.position);
    return maxPos + 1;
}

// MOC for the local Q_OBJECT classes (DeviceBrowserDialog) defined in this TU.
#include "EtherCATPanel.moc"
