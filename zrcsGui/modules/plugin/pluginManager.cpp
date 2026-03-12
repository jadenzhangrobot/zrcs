#include "plugin/pluginManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QDebug>
#include <QDir>

// ============================================================================
// PluginManager 实现
// ============================================================================

PluginManager &PluginManager::instance()
{
    static PluginManager manager;
    return manager;
}

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
}

void PluginManager::loadPlugins(const QString &pluginDir)
{
    QDir dir(pluginDir);
    if (!dir.exists()) {
        emit pluginError("", "插件目录不存在: " + pluginDir);
        return;
    }
    
    qDebug() << "加载插件目录:" << pluginDir;
}

void PluginManager::unloadPlugin(const QString &pluginName)
{
    if (plugins.contains(pluginName)) {
        plugins.remove(pluginName);
        emit pluginUnloaded(pluginName);
    }
}

void PluginManager::unloadAllPlugins()
{
    QStringList names = plugins.keys();
    for (const QString &name : names) {
        unloadPlugin(name);
    }
}

QVector<IToolPlugin *> PluginManager::getToolPlugins() const
{
    QVector<IToolPlugin *> result;
    // TODO: 实现插件类型过滤
    return result;
}

QVector<IVisualizationPlugin *> PluginManager::getVisualizationPlugins() const
{
    QVector<IVisualizationPlugin *> result;
    // TODO: 实现插件类型过滤
    return result;
}

IPlugin *PluginManager::getPlugin(const QString &name) const
{
    if (plugins.contains(name)) {
        return plugins[name].instance;
    }
    return nullptr;
}

// ============================================================================
// PluginPanel 实现
// ============================================================================

PluginPanel::PluginPanel(QWidget *parent)
    : QWidget(parent), manager(&PluginManager::instance())
{
    setupUI();
}

PluginPanel::~PluginPanel()
{
}

void PluginPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    stackedWidget = new QStackedWidget();
    
    QListWidget *pluginList = new QListWidget();
    pluginList->setStyleSheet("background-color: #1a1a1a; color: #00FF00;");
    
    stackedWidget->addWidget(pluginList);
    
    mainLayout->addWidget(stackedWidget);
    
    setStyleSheet("background-color: #1a1a1a;");
}

void PluginPanel::loadPlugins()
{
    manager->loadPlugins("./plugins");
    refreshPluginList();
}

void PluginPanel::refreshPluginList()
{
    qDebug() << "刷新插件列表";
}

void PluginPanel::onPluginLoaded(const QString &name)
{
    qDebug() << "插件已加载:" << name;
}

void PluginPanel::onPluginUnloaded(const QString &name)
{
    qDebug() << "插件已卸载:" << name;
}
