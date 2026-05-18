#pragma once

#include "behavior_tree/BehaviorTreePanel.h"
#include <QWidget>
#include <QSplitter>
#include <QToolBar>
#include <QTabWidget>
#include <QShortcut>
#include <QTransform>
#include <QRectF>
#include <QDomDocument>
#include <QXmlStreamWriter>
#include <deque>
#include <mutex>
#include <map>
#include <memory>

// Include full Groot/QtNodes headers in correct order
#include <nodes/NodeDataModel>
#include <nodes/ConnectionStyle>
#include <nodes/DataModelRegistry>
#include <nodes/FlowView>
#include <nodes/FlowScene>

#include "bt_editor/bt_editor_base.h"
#include "bt_editor/graphic_container.h"
#include "bt_editor/sidepanel_editor.h"

class BehaviorTreePanel : public QWidget
{
    Q_OBJECT

public:
    explicit BehaviorTreePanel(QWidget *parent = nullptr);
    ~BehaviorTreePanel();

    void loadFromXML(const QString &xmlText);
    QString saveToXML() const;

public slots:
    void onNewTree();
    void onLoadTree();
    void onSaveTree();
    void onAutoArrange();
    void onCenterView();
    void onToggleLayout();
    void onSaveSvg();

    // 行为树下发控制
    void onSendToController();
    void onStartExecution();
    void onStopExecution();

    void onSceneChanged();
    void onPushUndo();
    void onUndoInvoked();
    void onRedoInvoked();

    void onAddToModelRegistry(const NodeModel &model);
    void onModelRemoveRequested(QString ID);
    void onDestroySubTree(const QString &ID);
    void onTreeNodeEdited(QString prevID, QString newID);
    void onSubtreeSelected(const QString &subtreeName);

    void onCreateAbsBehaviorTree(const AbsBehaviorTree &tree,
                                  const QString &bt_name,
                                  bool secondary_tabs = true);

    void onRequestSubTreeExpand(GraphicContainer &container,
                                 QtNodes::Node &node);

signals:
    // 行为树下发信号，由 MainWindow 连接到 ZMQClient
    void requestBTLoad(const QString &xml);
    void requestBTStart();
    void requestBTStop();

private:
    enum SubtreeExpandOption { SUBTREE_EXPAND, SUBTREE_COLLAPSE,
                               SUBTREE_CHANGE, SUBTREE_REFRESH };

    void setupUI();
    void setupConnections();
    void initializeNodeModels();

    GraphicContainer* createTab(const QString &name);
    GraphicContainer* currentTabInfo();
    GraphicContainer* getTabByName(const QString &name);

    void onActionClearTriggered(bool create_new);
    void clearTreeModels();
    void lockEditing(bool locked);

    QtNodes::Node* subTreeExpand(GraphicContainer &container,
                                  QtNodes::Node &node,
                                  SubtreeExpandOption option);

    // Undo/redo
    struct SavedState {
        QString main_tree;
        QString current_tab_name;
        QTransform view_transform;
        QRectF view_area;
        std::map<QString, QByteArray> json_states;
        bool operator==(const SavedState &other) const;
        bool operator!=(const SavedState &other) const { return !(*this == other); }
    };
    SavedState saveCurrentState();
    void loadSavedStateFromJson(SavedState state);
    void clearUndoStacks();

    // XML serialization helpers
    QString xmlDocumentToString(const QDomDocument &document) const;
    void streamElementAttributes(QXmlStreamWriter &stream, const QDomElement &element) const;
    void recursivelySaveNodeCanonically(QXmlStreamWriter &stream, const QDomNode &parent_node) const;

    // Layout
    QToolBar *_toolbar;
    QSplitter *_splitter;
    QTabWidget *_treeTabWidget;

    // Groot components
    SidepanelEditor *_editorWidget;
    std::shared_ptr<QtNodes::DataModelRegistry> _modelRegistry;
    std::map<QString, GraphicContainer*> _tabInfo;

    // State
    NodeModels _treenodeModels;
    QtNodes::PortLayout _currentLayout;
    QString _mainTree;
    std::mutex _mutex;
    std::deque<SavedState> _undoStack;
    std::deque<SavedState> _redoStack;
    SavedState _currentState;
    QString _lastSaveDirectory;
};

