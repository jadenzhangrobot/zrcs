#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QString>
#include <QWidget>
#include <QObject>

/**
 * 插件接口
 * 所有插件必须实现这个接口
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * 获取插件名称
     */
    virtual QString getName() const = 0;

    /**
     * 获取插件版本
     */
    virtual QString getVersion() const = 0;

    /**
     * 获取插件描述
     */
    virtual QString getDescription() const = 0;

    /**
     * 获取插件作者
     */
    virtual QString getAuthor() const = 0;

    /**
     * 初始化插件
     */
    virtual bool initialize() = 0;

    /**
     * 清理插件
     */
    virtual void cleanup() = 0;

    /**
     * 获取插件的 UI 组件
     */
    virtual QWidget *getWidget() = 0;

    /**
     * 获取插件的菜单项（可选）
     */
    virtual QStringList getMenuItems() const {
        return QStringList();
    }

    /**
     * 处理菜单项点击（可选）
     */
    virtual void onMenuItemClicked(const QString &item) {
        Q_UNUSED(item);
    }
};

Q_DECLARE_INTERFACE(IPlugin, "com.zrcs.IPlugin/1.0")

/**
 * 工具插件接口
 * 用于扩展工具功能
 */
class IToolPlugin : public IPlugin {
public:
    virtual ~IToolPlugin() = default;

    /**
     * 执行工具功能
     */
    virtual bool execute(const QMap<QString, QVariant> &parameters) = 0;

    /**
     * 获取工具参数
     */
    virtual QMap<QString, QVariant> getParameters() const = 0;
};

Q_DECLARE_INTERFACE(IToolPlugin, "com.zrcs.IToolPlugin/1.0")

/**
 * 可视化插件接口
 * 用于扩展可视化功能
 */
class IVisualizationPlugin : public IPlugin {
public:
    virtual ~IVisualizationPlugin() = default;

    /**
     * 更新可视化数据
     */
    virtual void updateData(const QByteArray &data) = 0;

    /**
     * 获取可视化类型
     */
    virtual QString getVisualizationType() const = 0;
};

Q_DECLARE_INTERFACE(IVisualizationPlugin, "com.zrcs.IVisualizationPlugin/1.0")

#endif // PLUGIN_INTERFACE_H
