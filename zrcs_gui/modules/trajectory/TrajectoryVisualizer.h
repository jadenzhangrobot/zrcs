#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QVector3D>
#include <QPushButton>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class TrajectoryVisualizer3D : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit TrajectoryVisualizer3D(QWidget *parent = nullptr);
    ~TrajectoryVisualizer3D();

    void setTrajectoryPoints(const QVector<QVector3D> &points);
    void setModelMesh(const QVector<QVector3D> &triangles, const QVector<QVector3D> &segments);
    void setModelLineSegments(const QVector<QVector3D> &segments);
    void setWireframeOverlayEnabled(bool enabled);
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
    QVector<QVector3D> trajectoryPoints;
    QVector<QVector3D> modelTriangles;
    QVector<QVector3D> modelLineSegments;
    bool wireframeOverlayEnabled;
    QColor trajectoryColor;
    float rotateX;
    float rotateY;
    float distance;
    float panX;
    float panY;
    QPoint lastMousePos;
    Qt::MouseButton activeButton;
};

/**
 * 轨迹可视化面板
 * 集成 3D 可视化器（支持 2D 俯视）
 */
class TrajectoryPanel : public QWidget {
    Q_OBJECT

public:
    explicit TrajectoryPanel(QWidget *parent = nullptr);
    ~TrajectoryPanel();

    void loadGCodeFile(const QString &filePath);
    void loadSTEPFile(const QString &filePath);
    void addTrajectoryPoints(const QVector<QPointF> &points2D);
    void switchTo2DView();
    void switchTo3DView();
    void clearTrajectory();

signals:
    void trajectoryLoaded(int pointCount);
    void viewModeChanged(const QString &mode);

private:
    void setupUI();
    void parseGCode(const QString &filePath);
    void parseSTEP(const QString &filePath);
    QVector<QVector3D> to3DPoints(const QVector<QPointF> &points2D);
    QVector<QVector3D> normalize3DPoints(const QVector<QVector3D> &points3D);
    QVector<QVector3D> extractStepMeshOCC(const QString &filePath, QVector<QVector3D> &trianglesOut, bool &ok);

    TrajectoryVisualizer3D *visualizer3D;
    
    QVector<QPointF> trajectoryPoints2D;
    QVector<QVector3D> trajectoryPoints3D;
};

