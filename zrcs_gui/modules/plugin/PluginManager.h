#pragma once

#include <QString>
#include <QMap>
#include <QPluginLoader>
#include <QWidget>
#include <QTabWidget>
#include <QStackedWidget>
#include "PluginInterface.h"

/**
 * 插件信息
 */
struct PluginInfo {
    QString name;
    QString version;
    IPlugin *instance;
};

/**
 * 插件管理器
 * 负责加载、卸载和管理插件
 */
class PluginManager : public QObject {
    Q_OBJECT

public:
    static PluginManager &instance();

    void loadPlugins(const QString &pluginDir);
    void unloadPlugin(const QString &pluginName);
    void unloadAllPlugins();

    QVector<IToolPlugin *> getToolPlugins() const;
    QVector<IVisualizationPlugin *> getVisualizationPlugins() const;
    IPlugin *getPlugin(const QString &name) const;

signals:
    void pluginLoaded(const QString &name);
    void pluginUnloaded(const QString &name);
    void pluginError(const QString &name, const QString &error);

private:
    PluginManager();
    ~PluginManager();

    QMap<QString, PluginInfo> plugins;
    QMap<QString, QPluginLoader *> loaders;
};

/**
 * 插件面板
 * 显示和管理插件
 */
class PluginPanel : public QWidget {
    Q_OBJECT

public:
    explicit PluginPanel(QWidget *parent = nullptr);
    ~PluginPanel();

    void loadPlugins();
    void refreshPluginList();

signals:
    void pluginSelected(const QString &name);

private slots:
    void onPluginLoaded(const QString &name);
    void onPluginUnloaded(const QString &name);

private:
    void setupUI();

    PluginManager *manager;
    QStackedWidget *stackedWidget;
};

