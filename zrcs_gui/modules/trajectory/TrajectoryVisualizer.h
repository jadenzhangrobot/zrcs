#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QVector3D>
#include <QPushButton>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <functional>

class TrajectoryVisualizer3D : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit TrajectoryVisualizer3D(QWidget *parent = nullptr);
    ~TrajectoryVisualizer3D();

    void setTrajectoryPoints(const QVector<QVector3D> &points);
    void setEndEffectorPose(const QVector3D &pos, const QVector3D &rpy);
    void clearTrajectory();
    void setTrajectoryColor(const QColor &color);
    void resetView();
    void setTopView();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void drawGrid(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint);
    void drawAxes(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint);
    void drawTrajectoryPath(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint);
    void drawEEFrame(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint);

    QVector<QVector3D> trajectoryPoints;
    QVector<QVector3D> trajectoryHistory;  // 实时轨迹历史点 (max ~5000)
    QVector3D eePos;      // 当前末端位置
    QVector3D eeRpy;      // 当前末端姿态 (RX, RY, RZ in degrees)
    bool hasEEPose;       // 是否已收到位姿数据
    QColor trajectoryColor;
    float rotateX;
    float rotateY;
    float distance;
    float panX;
    float panY;
    QPoint lastMousePos;
    Qt::MouseButton activeButton;
};

class TrajectoryPanel : public QWidget {
    Q_OBJECT

public:
    explicit TrajectoryPanel(QWidget *parent = nullptr);
    ~TrajectoryPanel();

    void clearTrajectory();
    void updateEndEffectorPose(const QVector3D &pos, const QVector3D &rpy);

signals:
    void trajectoryLoaded(int pointCount);

private:
    void setupUI();

    TrajectoryVisualizer3D *visualizer3D;
    QVector<QVector3D> trajectoryPoints3D;
};
