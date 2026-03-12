#ifndef TRAJECTORY_VISUALIZER_H
#define TRAJECTORY_VISUALIZER_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVector>
#include <QPointF>
#include <QStackedWidget>
#include <QPushButton>

/**
 * 2D 轨迹可视化器
 * 使用 QGraphicsView 和 QGraphicsScene，支持大量坐标点的高效渲染
 */
class TrajectoryVisualizer2D : public QGraphicsView {
    Q_OBJECT

public:
    explicit TrajectoryVisualizer2D(QWidget *parent = nullptr);
    ~TrajectoryVisualizer2D();

    void addTrajectoryPoints(const QVector<QPointF> &points);
    void clearTrajectory();
    void setTrajectoryColor(const QColor &color);
    void fitInView();
    double getZoomLevel() const;
    void setZoomLevel(double level);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QGraphicsScene *scene;
    QColor trajectoryColor;
    double zoomLevel;
    QPointF lastMousePos;
    bool isPanning;
};

/**
 * 轨迹可视化面板
 * 集成 2D 可视化器
 */
class TrajectoryPanel : public QWidget {
    Q_OBJECT

public:
    explicit TrajectoryPanel(QWidget *parent = nullptr);
    ~TrajectoryPanel();

    void loadGCodeFile(const QString &filePath);
    void addTrajectoryPoints(const QVector<QPointF> &points2D);
    void switchTo2DView();
    void clearTrajectory();

signals:
    void trajectoryLoaded(int pointCount);
    void viewModeChanged(const QString &mode);

private:
    void setupUI();
    void parseGCode(const QString &filePath);

    TrajectoryVisualizer2D *visualizer2D;
    QStackedWidget *stackedWidget;
    
    QVector<QPointF> trajectoryPoints2D;
};

#endif // TRAJECTORY_VISUALIZER_H
