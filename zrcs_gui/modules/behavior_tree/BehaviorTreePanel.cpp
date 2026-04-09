#include "behavior_tree/BehaviorTreePanel.h"

#include <QDebug>
#include <QSettings>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QTabBar>
#include <QXmlStreamWriter>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/NodeStyle>
#include <nodes/FlowView>

#include "bt_editor/editor_flowscene.h"
#include "bt_editor/utils.h"
#include "bt_editor/XML_utilities.hpp"
#include "bt_editor/models/RootNodeModel.hpp"
#include "bt_editor/models/SubtreeNodeModel.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowView;
using QtNodes::FlowScene;

BehaviorTreePanel::BehaviorTreePanel(QWidget *parent)
    : QWidget(parent)
    , _currentLayout(QtNodes::PortLayout::Vertical)
{
    initializeNodeModels();
    setupUI();
    setupConnections();

    createTab(QString::fromUtf8("行为树"));
    _mainTree = QString::fromUtf8("行为树");
    onSceneChanged();
    _currentState = saveCurrentState();
}

BehaviorTreePanel::~BehaviorTreePanel()
{
    for (auto &it : _tabInfo) {
        it.second->clearScene();
    }
}

void BehaviorTreePanel::initializeNodeModels()
{
    _modelRegistry = std::make_shared<QtNodes::DataModelRegistry>();

    auto registerModel = [this](const QString &ID, const NodeModel &model) {
        QString category = QString::fromStdString(BT::toStr(model.type));
        if (ID == "Root") {
            category = "Root";
        }
        DataModelRegistry::RegistryItemCreator creator;
        creator = [model]() -> DataModelRegistry::RegistryItemPtr {
            return std::unique_ptr<BehaviorTreeDataModel>(
                new BehaviorTreeDataModel(model));
        };
        _modelRegistry->registerModel(category, creator, ID);
    };

    for (const auto &model : BuiltinNodeModels()) {
        registerModel(model.first, model.second);
        _treenodeModels.insert({model.first, model.second});
    }
}

void BehaviorTreePanel::setupUI()
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    _mainLayout->setSpacing(4);

    // Toolbar
    _toolbar = new QToolBar(this);
    _toolbar->setIconSize(QSize(20, 20));

    auto *btnNew = _toolbar->addAction(QIcon(":/icons/svg/list_add.svg"), QString::fromUtf8("新建"));
    auto *btnLoad = _toolbar->addAction(QIcon(":/icons/svg/folder.svg"), QString::fromUtf8("加载"));
    auto *btnSave = _toolbar->addAction(QIcon(":/icons/svg/save_dark.svg"), QString::fromUtf8("保存"));
    _toolbar->addSeparator();
    auto *btnArrange = _toolbar->addAction(QIcon(":/icons/svg/magic-wand.svg"), QString::fromUtf8("自动排列"));
    auto *btnCenter = _toolbar->addAction(QIcon(":/icons/svg/zoom_home.svg"), QString::fromUtf8("居中视图"));
    auto *btnLayout = _toolbar->addAction(QIcon(":/icons/BT-vertical.png"), QString::fromUtf8("切换布局"));
    _toolbar->addSeparator();
    auto *btnSvg = _toolbar->addAction(QIcon(":/icons/svg/download.svg"), QString::fromUtf8("导出SVG"));

    _toolbar->addSeparator();
    auto *btnSend = _toolbar->addAction(QIcon(":/icons/svg/save_dark.svg"), QString::fromUtf8("下发到控制器"));
    auto *btnStart = _toolbar->addAction(QIcon(":/icons/svg/play.svg"), QString::fromUtf8("启动执行"));
    auto *btnStop = _toolbar->addAction(QIcon(":/icons/svg/stop.svg"), QString::fromUtf8("停止执行"));

    connect(btnNew, &QAction::triggered, this, &BehaviorTreePanel::onNewTree);
    connect(btnLoad, &QAction::triggered, this, &BehaviorTreePanel::onLoadTree);
    connect(btnSave, &QAction::triggered, this, &BehaviorTreePanel::onSaveTree);
    connect(btnArrange, &QAction::triggered, this, &BehaviorTreePanel::onAutoArrange);
    connect(btnCenter, &QAction::triggered, this, &BehaviorTreePanel::onCenterView);
    connect(btnLayout, &QAction::triggered, this, &BehaviorTreePanel::onToggleLayout);
    connect(btnSvg, &QAction::triggered, this, &BehaviorTreePanel::onSaveSvg);
    connect(btnSend, &QAction::triggered, this, &BehaviorTreePanel::onSendToController);
    connect(btnStart, &QAction::triggered, this, &BehaviorTreePanel::onStartExecution);
    connect(btnStop, &QAction::triggered, this, &BehaviorTreePanel::onStopExecution);

    _mainLayout->addWidget(_toolbar);

    // Splitter: side panel | tree tab widget
    _splitter = new QSplitter(Qt::Horizontal, this);

    _editorWidget = new SidepanelEditor(_modelRegistry.get(), _treenodeModels, this);
    _treeTabWidget = new QTabWidget(this);
    _treeTabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);

    _splitter->addWidget(_editorWidget);
    _splitter->addWidget(_treeTabWidget);
    _splitter->setStretchFactor(0, 1);
    _splitter->setStretchFactor(1, 4);

    _mainLayout->addWidget(_splitter);
}

void BehaviorTreePanel::setupConnections()
{
    // Editor widget signals
    connect(_editorWidget, &SidepanelEditor::nodeModelEdited,
            this, &BehaviorTreePanel::onTreeNodeEdited);
    connect(_editorWidget, &SidepanelEditor::addNewModel,
            this, &BehaviorTreePanel::onAddToModelRegistry);
    connect(_editorWidget, &SidepanelEditor::destroySubtree,
            this, &BehaviorTreePanel::onDestroySubTree);
    connect(_editorWidget, &SidepanelEditor::modelRemoveRequested,
            this, &BehaviorTreePanel::onModelRemoveRequested);

    connect(_editorWidget, &SidepanelEditor::addSubtree,
            this, [this](QString ID) {
                this->createTab(ID);
            });

    connect(_editorWidget, &SidepanelEditor::setTabScope,
            this, &BehaviorTreePanel::onSubtreeSelected);

    connect(_editorWidget, &SidepanelEditor::renameSubtree,
            this, [this](QString prev_ID, QString new_ID) {
                if (prev_ID == new_ID) return;
                for (int index = 0; index < _treeTabWidget->count(); index++) {
                    if (_treeTabWidget->tabText(index) == prev_ID) {
                        _treeTabWidget->setTabText(index, new_ID);
                        _tabInfo.insert({new_ID, _tabInfo.at(prev_ID)});
                        _tabInfo.erase(prev_ID);
                        break;
                    }
                }
            });

    // Keyboard shortcuts
    auto *undoShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z), this);
    connect(undoShortcut, &QShortcut::activated, this, &BehaviorTreePanel::onUndoInvoked);

    auto *redoShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), this);
    connect(redoShortcut, &QShortcut::activated, this, &BehaviorTreePanel::onRedoInvoked);

    auto *saveShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
    connect(saveShortcut, &QShortcut::activated, this, &BehaviorTreePanel::onSaveTree);

    auto *arrangeShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    connect(arrangeShortcut, &QShortcut::activated, this, &BehaviorTreePanel::onAutoArrange);
}

// =========== Tab management ===========

GraphicContainer *BehaviorTreePanel::createTab(const QString &name)
{
    if (_tabInfo.count(name) > 0) {
        return _tabInfo.at(name);
    }
    auto *ti = new GraphicContainer(_modelRegistry, this);
    _tabInfo.insert({name, ti});

    ti->scene()->setLayout(_currentLayout);
    _treeTabWidget->addTab(ti->view(), name);
    ti->scene()->createNodeAtPos("Root", "Root", QPointF(-30, -30));
    ti->zoomHomeView();

    connect(ti, &GraphicContainer::undoableChange,
            this, &BehaviorTreePanel::onPushUndo);
    connect(ti, &GraphicContainer::undoableChange,
            this, &BehaviorTreePanel::onSceneChanged);
    connect(ti, &GraphicContainer::requestSubTreeExpand,
            this, &BehaviorTreePanel::onRequestSubTreeExpand);
    connect(ti, &GraphicContainer::requestSubTreeCreate,
            this, [this](const AbsBehaviorTree &tree, const QString &bt_name) {
                onCreateAbsBehaviorTree(tree, bt_name, false);
            });
    connect(ti, &GraphicContainer::addNewModel,
            this, &BehaviorTreePanel::onAddToModelRegistry);

    return ti;
}

GraphicContainer *BehaviorTreePanel::currentTabInfo()
{
    int index = _treeTabWidget->currentIndex();
    if (index < 0) return nullptr;
    QString tab_name = _treeTabWidget->tabText(index);
    return getTabByName(tab_name);
}

GraphicContainer *BehaviorTreePanel::getTabByName(const QString &name)
{
    auto it = _tabInfo.find(name);
    return (it != _tabInfo.end()) ? it->second : nullptr;
}

// =========== File operations ===========

void BehaviorTreePanel::onNewTree()
{
    onActionClearTriggered(true);
    clearTreeModels();
    clearUndoStacks();
}

void BehaviorTreePanel::onLoadTree()
{
    QSettings settings;
    QString directory_path = settings.value("BehaviorTreePanel.lastLoadDirectory",
                                             QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(this, tr("从文件加载行为树"),
                                                     directory_path,
                                                     tr("行为树文件 (*.xml)"));
    if (!QFileInfo::exists(fileName)) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return;

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("BehaviorTreePanel.lastLoadDirectory", directory_path);
    settings.sync();

    QString xml_text;
    QTextStream in(&file);
    while (!in.atEnd()) {
        xml_text += in.readLine();
    }
    loadFromXML(xml_text);
}

void BehaviorTreePanel::onSaveTree()
{
    for (auto &it : _tabInfo) {
        if (!it.second->containsValidTree()) {
            QMessageBox::warning(this, tr("错误"),
                                  tr("行为树格式错误，无法保存"),
                                  QMessageBox::Cancel);
            return;
        }
    }

    if (_tabInfo.size() == 1) {
        _mainTree = _tabInfo.begin()->first;
    }

    QSettings settings;
    QString directory_path = settings.value("BehaviorTreePanel.lastSaveDirectory",
                                             QDir::currentPath()).toString();

    auto fileName = QFileDialog::getSaveFileName(this, "保存行为树到文件",
                                                  directory_path,
                                                  "行为树文件 (*.xml)");
    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(".xml")) {
        fileName += ".xml";
    }

    QString xml_text = saveToXML();

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << xml_text;
    }

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("BehaviorTreePanel.lastSaveDirectory", directory_path);
}

void BehaviorTreePanel::loadFromXML(const QString &xml_text)
{
    QDomDocument document;
    try {
        QString errorMsg;
        int errorLine;
        if (!document.setContent(xml_text, &errorMsg, &errorLine)) {
            throw std::runtime_error(
                tr("解析XML错误 (第%1行): %2")
                    .arg(errorLine).arg(errorMsg).toStdString());
        }

        std::vector<QString> registered_ID;
        for (const auto &it : _treenodeModels) {
            registered_ID.push_back(it.first);
        }
        std::vector<QString> error_messages;
        bool done = VerifyXML(document, registered_ID, error_messages);

        if (!done) {
            QString merged_error;
            for (const auto &err : error_messages) {
                merged_error += err + "\n";
            }
            throw std::runtime_error(merged_error.toStdString());
        }
    } catch (std::runtime_error &err) {
        QMessageBox messageBox;
        messageBox.critical(this, "解析XML错误", err.what());
        messageBox.show();
        return;
    }

    bool error = false;
    QString err_message;
    auto saved_state = _currentState;
    auto prev_tree_model = _treenodeModels;

    try {
        auto document_root = document.documentElement();

        if (document_root.hasAttribute("main_tree_to_execute")) {
            _mainTree = document_root.attribute("main_tree_to_execute");
        }

        auto custom_models = ReadTreeNodesModel(document_root);
        for (const auto &model : custom_models) {
            onAddToModelRegistry(model.second);
        }
        _editorWidget->updateTreeView();

        onActionClearTriggered(false);
        const QSignalBlocker blocker(currentTabInfo());

        for (auto bt_root = document_root.firstChildElement("BehaviorTree");
             !bt_root.isNull();
             bt_root = bt_root.nextSiblingElement("BehaviorTree"))
        {
            auto tree = BuildTreeFromXML(bt_root, _treenodeModels);
            QString tree_name(QString::fromUtf8("行为树"));

            if (bt_root.hasAttribute("ID")) {
                tree_name = bt_root.attribute("ID");
                if (_mainTree.isEmpty()) {
                    _mainTree = tree_name;
                }
            }
            onCreateAbsBehaviorTree(tree, tree_name);
        }

        if (!_mainTree.isEmpty()) {
            for (int i = 0; i < _treeTabWidget->count(); i++) {
                if (_treeTabWidget->tabText(i) == _mainTree) {
                    _treeTabWidget->tabBar()->moveTab(i, 0);
                    _treeTabWidget->setCurrentIndex(0);
                    _treeTabWidget->tabBar()->setTabIcon(0, QIcon(":/icons/svg/star.svg"));
                    break;
                }
            }
        }

        if (currentTabInfo() == nullptr) {
            createTab(QString::fromUtf8("行为树"));
            _mainTree = QString::fromUtf8("行为树");
        } else {
            currentTabInfo()->nodeReorder();
        }

        auto models_to_remove = GetModelsToRemove(this, _treenodeModels, custom_models);
        for (QString model_name : models_to_remove) {
            onModelRemoveRequested(model_name);
        }
    } catch (std::exception &err) {
        error = true;
        err_message = err.what();
    }

    if (error) {
        _treenodeModels = prev_tree_model;
        loadSavedStateFromJson(saved_state);
        QMessageBox::warning(this, tr("异常"),
                              tr("无法解析文件。错误:\n\n%1").arg(err_message),
                              QMessageBox::Ok);
    } else {
        onSceneChanged();
        onPushUndo();
    }
}

QString BehaviorTreePanel::saveToXML() const
{
    QDomDocument doc;
    const char *COMMENT_SEPARATOR = " ////////// ";

    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    if (!_mainTree.isEmpty()) {
        root.setAttribute("main_tree_to_execute", _mainTree.toStdString().c_str());
    }

    for (auto &it : _tabInfo) {
        auto &container = it.second;
        auto scene = container->scene();
        auto abs_tree = BuildTreeFromScene(scene);
        auto abs_root = abs_tree.rootNode();

        if (abs_root->children_index.size() == 1 &&
            abs_root->model.registration_ID == "Root") {
            abs_root = abs_tree.node(abs_root->children_index.front());
        }

        QtNodes::Node *root_node = abs_root->graphic_node;

        root.appendChild(doc.createComment(COMMENT_SEPARATOR));
        QDomElement root_element = doc.createElement("BehaviorTree");
        root_element.setAttribute("ID", it.first.toStdString().c_str());
        root.appendChild(root_element);

        RecursivelyCreateXml(*scene, doc, root_element, root_node);
    }
    root.appendChild(doc.createComment(COMMENT_SEPARATOR));

    QDomElement root_models = doc.createElement("TreeNodesModel");
    for (const auto &tree_it : _treenodeModels) {
        const auto &ID = tree_it.first;
        const auto &model = tree_it.second;

        if (BuiltinNodeModels().count(ID) != 0) {
            continue;
        }

        QDomElement node = doc.createElement(QString::fromStdString(toStr(model.type)));
        if (!node.isNull()) {
            node.setAttribute("ID", ID);
            for (const auto &port_it : model.ports) {
                const auto &port_name = port_it.first;
                const auto &port = port_it.second;
                QDomElement port_element = writePortModel(port_name, port, doc);
                node.appendChild(port_element);
            }
        }
        root_models.appendChild(node);
    }
    root.appendChild(root_models);
    root.appendChild(doc.createComment(COMMENT_SEPARATOR));

    return xmlDocumentToString(doc);
}

// =========== 行为树下发控制 ===========

void BehaviorTreePanel::onSendToController()
{
    for (auto &it : _tabInfo) {
        if (!it.second->containsValidTree()) {
            QMessageBox::warning(this, tr("错误"),
                                  tr("行为树格式错误，无法下发"),
                                  QMessageBox::Cancel);
            return;
        }
    }

    QString xml = saveToXML();
    if (xml.isEmpty()) {
        QMessageBox::warning(this, tr("错误"),
                              tr("无法生成行为树 XML"),
                              QMessageBox::Cancel);
        return;
    }

    emit requestBTLoad(xml);
    qDebug() << "[BehaviorTreePanel] Sent BT XML to controller";
}

void BehaviorTreePanel::onStartExecution()
{
    emit requestBTStart();
}

void BehaviorTreePanel::onStopExecution()
{
    emit requestBTStop();
}

// =========== Toolbar actions ===========

void BehaviorTreePanel::onAutoArrange()
{
    if (currentTabInfo()) {
        currentTabInfo()->nodeReorder();
    }
}

void BehaviorTreePanel::onCenterView()
{
    if (currentTabInfo()) {
        currentTabInfo()->zoomHomeView();
    }
}

void BehaviorTreePanel::onToggleLayout()
{
    QtNodes::PortLayout new_layout =
        (_currentLayout == QtNodes::PortLayout::Vertical)
            ? QtNodes::PortLayout::Horizontal
            : QtNodes::PortLayout::Vertical;

    if (new_layout != _currentLayout) {
        const QSignalBlocker blocker(currentTabInfo());
        for (auto &tab : _tabInfo) {
            auto scene = tab.second->scene();
            if (scene->layout() != new_layout) {
                auto abstract_tree = BuildTreeFromScene(scene);
                scene->setLayout(new_layout);
                NodeReorder(*scene, abstract_tree);
            }
        }
        _currentLayout = new_layout;
    }
}

void BehaviorTreePanel::onSaveSvg()
{
    if (!currentTabInfo()) return;

    QSettings settings;
    QString directory_path = settings.value("BehaviorTreePanel.lastSaveSvgDirectory",
                                             QDir::homePath()).toString();

    QString fileName = QFileDialog::getSaveFileName(this, tr("保存行为树为SVG"),
                                                     directory_path,
                                                     tr("SVG文件 (*.svg)"));
    if (fileName.isEmpty()) return;
    currentTabInfo()->saveSvgFile(fileName);

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("BehaviorTreePanel.lastSaveSvgDirectory", directory_path);
}

// =========== Scene / model management ===========

void BehaviorTreePanel::onSceneChanged()
{
    // Update toolbar state based on tree validity
    if (currentTabInfo()) {
        const bool valid = currentTabInfo()->containsValidTree();
        Q_UNUSED(valid);
    }
}

void BehaviorTreePanel::onAddToModelRegistry(const NodeModel &model)
{
    namespace util = QtNodes::detail;
    const auto &ID = model.registration_ID;

    DataModelRegistry::RegistryItemCreator node_creator =
        [model]() -> DataModelRegistry::RegistryItemPtr {
            if (model.type == NodeType::SUBTREE) {
                return util::make_unique<SubtreeNodeModel>(model);
            }
            return util::make_unique<BehaviorTreeDataModel>(model);
        };

    _modelRegistry->registerModel(
        QString::fromStdString(toStr(model.type)), node_creator, ID);
    _treenodeModels.insert({ID, model});
    _editorWidget->updateTreeView();
}

void BehaviorTreePanel::onModelRemoveRequested(QString ID)
{
    BehaviorTreeDataModel *node_found = nullptr;
    QString tab_containing_node;

    for (auto &it : _tabInfo) {
        auto container = it.second;
        for (const auto &node_it : container->scene()->nodes()) {
            QtNodes::Node *graphic_node = node_it.second.get();
            auto bt_node = dynamic_cast<BehaviorTreeDataModel *>(
                graphic_node->nodeDataModel());
            if (bt_node->model().registration_ID == ID) {
                node_found = bt_node;
                tab_containing_node = it.first;
                break;
            }
        }
        if (node_found) break;
    }

    if (!node_found) {
        _editorWidget->onRemoveModel(ID);
        return;
    }

    NodeType node_type = _treenodeModels.at(ID).type;

    if (node_found && node_type != NodeType::SUBTREE) {
        QMessageBox::warning(
            this, "无法删除此模型",
            QString("您正在树 [%1] 中使用此模型。\n"
                    "除非删除所有 [%2] 的实例，否则无法删除此模型。")
                .arg(tab_containing_node, ID),
            QMessageBox::Ok);
    } else {
        int ret = QMessageBox::Cancel;
        if (node_found->model().type != NodeType::SUBTREE) {
            ret = QMessageBox::warning(
                this, "删除树节点模型?",
                "确定要删除吗？此操作无法撤销。",
                QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);
        } else {
            ret = QMessageBox::warning(
                this, "删除子树?",
                "子树的模型将被删除。"
                "展开的版本将添加到父树中。\n"
                "确定要删除吗？此操作无法撤销。",
                QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);
        }
        if (ret == QMessageBox::Yes) {
            _editorWidget->onRemoveModel(ID);
            clearUndoStacks();
        }
    }
}

void BehaviorTreePanel::onDestroySubTree(const QString &ID)
{
    auto sub_container = getTabByName(ID);

    for (auto &it : _tabInfo) {
        if (it.first == ID) continue;
        auto container = it.second;
        auto tree = BuildTreeFromScene(container->scene());
        for (const auto &abs_node : tree.nodes()) {
            auto qt_node = abs_node.graphic_node;
            auto bt_node = dynamic_cast<BehaviorTreeDataModel *>(
                qt_node->nodeDataModel());
            if (bt_node->nodeType() == NodeType::SUBTREE &&
                bt_node->instanceName() == ID) {
                auto new_node = qt_node;
                auto subtree_model = dynamic_cast<SubtreeNodeModel *>(bt_node);
                if (subtree_model && !subtree_model->expanded()) {
                    new_node = subTreeExpand(*container, *qt_node, SUBTREE_EXPAND);
                }
                container->lockSubtreeEditing(*new_node, false, false);
                container->onSmartRemove(new_node);
            }
        }
        container->nodeReorder();
    }

    for (int index = 0; index < _treeTabWidget->count(); index++) {
        if (_treeTabWidget->tabText(index) == ID) {
            sub_container->scene()->clearScene();
            sub_container->deleteLater();
            _treeTabWidget->removeTab(index);
            _tabInfo.erase(ID);
            break;
        }
    }

    if (_treeTabWidget->count() == 1) {
        _mainTree = _treeTabWidget->tabText(0);
    }
    clearUndoStacks();
}

void BehaviorTreePanel::onTreeNodeEdited(QString prevID, QString newID)
{
    for (auto &it : _tabInfo) {
        auto container = it.second;
        std::vector<QtNodes::Node *> nodes_to_rename;

        for (const auto &node_it : container->scene()->nodes()) {
            QtNodes::Node *graphic_node = node_it.second.get();
            if (!graphic_node) continue;
            auto bt_node = dynamic_cast<BehaviorTreeDataModel *>(
                graphic_node->nodeDataModel());
            if (!bt_node) continue;
            if (bt_node->model().registration_ID == prevID) {
                nodes_to_rename.push_back(graphic_node);
            }
        }

        for (auto &graphic_node : nodes_to_rename) {
            auto bt_node = dynamic_cast<BehaviorTreeDataModel *>(
                graphic_node->nodeDataModel());
            bool is_expanded_subtree = false;

            if (bt_node->model().type == NodeType::SUBTREE) {
                auto subtree_model = dynamic_cast<SubtreeNodeModel *>(bt_node);
                if (subtree_model && subtree_model->expanded()) {
                    is_expanded_subtree = true;
                    subTreeExpand(*container, *graphic_node, SUBTREE_COLLAPSE);
                }
            }

            auto new_node = container->substituteNode(graphic_node, newID);

            if (is_expanded_subtree) {
                subTreeExpand(*container, *new_node, SUBTREE_EXPAND);
            }
        }
    }
}

void BehaviorTreePanel::onSubtreeSelected(const QString &subtreeName)
{
    for (int i = 0; i < _treeTabWidget->count(); i++) {
        if (_treeTabWidget->tabText(i) == subtreeName) {
            _treeTabWidget->setCurrentIndex(i);
            break;
        }
    }
}

void BehaviorTreePanel::onCreateAbsBehaviorTree(const AbsBehaviorTree &tree,
                                                  const QString &bt_name,
                                                  bool secondary_tabs)
{
    auto container = getTabByName(bt_name);
    if (!container) {
        container = createTab(bt_name);
    }
    const QSignalBlocker blocker(container);
    container->loadSceneFromTree(tree);
    container->nodeReorder();

    if (secondary_tabs) {
        for (const auto &node : tree.nodes()) {
            if (node.model.type == NodeType::SUBTREE &&
                getTabByName(node.model.registration_ID) == nullptr) {
                createTab(node.model.registration_ID);
            }
        }
    }
    clearUndoStacks();
}

void BehaviorTreePanel::onRequestSubTreeExpand(GraphicContainer &container,
                                                 QtNodes::Node &node)
{
    auto subtree = dynamic_cast<SubtreeNodeModel *>(node.nodeDataModel());
    if (!subtree) return;

    if (subtree->expanded()) {
        subTreeExpand(container, node, SUBTREE_COLLAPSE);
    } else {
        subTreeExpand(container, node, SUBTREE_EXPAND);
    }
}

// =========== Internal helpers ===========

void BehaviorTreePanel::onActionClearTriggered(bool create_new)
{
    for (auto &it : _tabInfo) {
        it.second->clearScene();
        it.second->deleteLater();
    }
    _tabInfo.clear();
    _treeTabWidget->clear();

    if (create_new) {
        createTab(QString::fromUtf8("行为树"));
    }
    _editorWidget->clear();
}

void BehaviorTreePanel::clearTreeModels()
{
    // Remove non-builtin models
    auto it = _treenodeModels.begin();
    while (it != _treenodeModels.end()) {
        if (BuiltinNodeModels().count(it->first) == 0) {
            it = _treenodeModels.erase(it);
        } else {
            ++it;
        }
    }
    _editorWidget->updateTreeView();
}

void BehaviorTreePanel::lockEditing(bool locked)
{
    for (auto &tab_it : _tabInfo) {
        tab_it.second->lockEditing(locked);
    }
}

QtNodes::Node *BehaviorTreePanel::subTreeExpand(GraphicContainer &container,
                                                  QtNodes::Node &node,
                                                  SubtreeExpandOption option)
{
    const QSignalBlocker blocker(this);
    auto subtree_model = dynamic_cast<SubtreeNodeModel *>(node.nodeDataModel());
    const QString &subtree_name = subtree_model->registrationName();

    if (option == SUBTREE_EXPAND && !subtree_model->expanded()) {
        auto subtree_container = getTabByName(subtree_name);
        if (!subtree_container) {
            QMessageBox::warning(this, tr("错误"),
                                  tr("找不到子树标签页，无法展开。"),
                                  QMessageBox::Cancel);
            return &node;
        }
        if (!subtree_container->containsValidTree()) {
            QMessageBox::warning(this, tr("错误"),
                                  tr("无效的子树，无法展开。"),
                                  QMessageBox::Cancel);
            return &node;
        }

        auto abs_subtree = BuildTreeFromScene(subtree_container->scene());
        subtree_model->setExpanded(true);
        node.nodeState().getEntries(PortType::Out).resize(1);
        container.appendTreeToNode(node, abs_subtree);
        container.lockSubtreeEditing(node, true, true);

        if (abs_subtree.nodes().size() > 1) {
            container.nodeReorder();
        }
        return &node;
    }

    if (option == SUBTREE_COLLAPSE && subtree_model->expanded()) {
        bool need_reorder = true;
        const auto &conn_out = node.nodeState().connections(PortType::Out, 0);
        QtNodes::Node *child_node = nullptr;
        if (conn_out.size() == 1) {
            child_node = conn_out.begin()->second->getNode(PortType::In);
        }

        const QSignalBlocker blocker2(container);
        if (child_node) {
            container.deleteSubTreeRecursively(*child_node);
        } else {
            need_reorder = false;
        }

        subtree_model->setExpanded(false);
        node.nodeState().getEntries(PortType::Out).resize(0);
        container.lockSubtreeEditing(node, false, true);
        if (need_reorder) {
            container.nodeReorder();
        }
        return &node;
    }

    if (option == SUBTREE_REFRESH && subtree_model->expanded()) {
        const auto &conn_out = node.nodeState().connections(PortType::Out, 0);
        if (conn_out.size() != 1) return &node;

        QtNodes::Node *child_node = conn_out.begin()->second->getNode(PortType::In);
        auto subtree_container = getTabByName(subtree_name);
        auto subtree = BuildTreeFromScene(subtree_container->scene());

        container.deleteSubTreeRecursively(*child_node);
        container.appendTreeToNode(node, subtree);
        container.nodeReorder();
        container.lockSubtreeEditing(node, true, true);
        return &node;
    }

    return nullptr;
}

// =========== Undo / Redo ===========

bool BehaviorTreePanel::SavedState::operator==(const SavedState &other) const
{
    if (main_tree != other.main_tree) return false;
    if (current_tab_name != other.current_tab_name) return false;
    if (json_states.size() != other.json_states.size()) return false;
    for (const auto &it : json_states) {
        auto other_it = other.json_states.find(it.first);
        if (other_it == other.json_states.end()) return false;
        if (it.second != other_it->second) return false;
    }
    return true;
}

BehaviorTreePanel::SavedState BehaviorTreePanel::saveCurrentState()
{
    SavedState saved;
    int index = _treeTabWidget->currentIndex();
    if (index < 0) return saved;

    saved.main_tree = _mainTree;
    saved.current_tab_name = _treeTabWidget->tabText(index);
    auto current_view = getTabByName(saved.current_tab_name);
    if (current_view) {
        saved.view_transform = current_view->view()->transform();
        saved.view_area = current_view->view()->sceneRect();
    }

    for (auto &it : _tabInfo) {
        saved.json_states[it.first] = it.second->scene()->saveToMemory();
    }
    return saved;
}

void BehaviorTreePanel::onPushUndo()
{
    SavedState saved = saveCurrentState();

    if (_undoStack.empty() ||
        (saved != _currentState && _undoStack.back() != _currentState)) {
        _undoStack.push_back(std::move(_currentState));
        _redoStack.clear();
    }
    _currentState = saved;
}

void BehaviorTreePanel::onUndoInvoked()
{
    if (_undoStack.size() > 0) {
        _redoStack.push_back(std::move(_currentState));
        _currentState = _undoStack.back();
        _undoStack.pop_back();
        loadSavedStateFromJson(_currentState);
    }
}

void BehaviorTreePanel::onRedoInvoked()
{
    if (_redoStack.size() > 0) {
        _undoStack.push_back(_currentState);
        _currentState = std::move(_redoStack.back());
        _redoStack.pop_back();
        loadSavedStateFromJson(_currentState);
    }
}

void BehaviorTreePanel::loadSavedStateFromJson(SavedState saved_state)
{
    for (auto &it : _tabInfo) {
        it.second->clearScene();
        it.second->deleteLater();
    }
    _tabInfo.clear();
    _treeTabWidget->clear();

    _mainTree = saved_state.main_tree;

    for (const auto &it : saved_state.json_states) {
        QString tab_name = it.first;
        _tabInfo.insert({tab_name, createTab(tab_name)});
    }
    for (const auto &it : saved_state.json_states) {
        QString name = it.first;
        auto container = getTabByName(name);
        if (container) {
            container->loadFromJson(it.second);
            container->view()->setTransform(saved_state.view_transform);
            container->view()->setSceneRect(saved_state.view_area);
        }
    }

    for (int i = 0; i < _treeTabWidget->count(); i++) {
        if (_treeTabWidget->tabText(i) == saved_state.current_tab_name) {
            _treeTabWidget->setCurrentIndex(i);
            _treeTabWidget->widget(i)->setFocus();
        }
    }
    onSceneChanged();
}

void BehaviorTreePanel::clearUndoStacks()
{
    _undoStack.clear();
    _redoStack.clear();
    onSceneChanged();
    onPushUndo();
}

// =========== XML helpers ===========

QString BehaviorTreePanel::xmlDocumentToString(const QDomDocument &document) const
{
    QString output_string;
    QXmlStreamWriter stream(&output_string);
    stream.setAutoFormatting(true);
    stream.setAutoFormattingIndent(4);
    stream.writeStartDocument();

    auto root_element = document.documentElement();
    stream.writeStartElement(root_element.tagName());
    streamElementAttributes(stream, root_element);

    auto next_node = root_element.firstChild();
    while (!next_node.isNull()) {
        recursivelySaveNodeCanonically(stream, next_node);
        if (stream.hasError()) break;
        next_node = next_node.nextSibling();
    }

    stream.writeEndElement();
    stream.writeEndDocument();
    return output_string;
}

void BehaviorTreePanel::streamElementAttributes(QXmlStreamWriter &stream,
                                                  const QDomElement &element) const
{
    if (element.hasAttributes()) {
        QMap<QString, QString> attributes;
        const QDomNamedNodeMap attributes_map = element.attributes();
        for (int i = 0; i < attributes_map.count(); ++i) {
            auto attribute = attributes_map.item(i);
            attributes.insert(attribute.nodeName(), attribute.nodeValue());
        }
        auto i = attributes.constBegin();
        while (i != attributes.constEnd()) {
            stream.writeAttribute(i.key(), i.value());
            ++i;
        }
    }
}

void BehaviorTreePanel::recursivelySaveNodeCanonically(QXmlStreamWriter &stream,
                                                         const QDomNode &parent_node) const
{
    if (stream.hasError()) return;

    if (parent_node.isElement()) {
        const QDomElement parent_element = parent_node.toElement();
        if (!parent_element.isNull()) {
            stream.writeStartElement(parent_element.tagName());
            streamElementAttributes(stream, parent_element);
            if (parent_element.hasChildNodes()) {
                auto child = parent_element.firstChild();
                while (!child.isNull()) {
                    recursivelySaveNodeCanonically(stream, child);
                    child = child.nextSibling();
                }
            }
            stream.writeEndElement();
        }
    } else if (parent_node.isComment()) {
        stream.writeComment(parent_node.nodeValue());
    } else if (parent_node.isText()) {
        stream.writeCharacters(parent_node.nodeValue());
    }
}
