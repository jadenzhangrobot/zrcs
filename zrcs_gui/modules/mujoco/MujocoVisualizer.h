#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>
#include <QVector>
#include <QWidget>

#include <memory>

class MujocoVisualizer3D : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit MujocoVisualizer3D(QWidget *parent = nullptr);
    ~MujocoVisualizer3D() override;

    void setAxisPositions(const QVector<double> &positions);
    void reloadModel();

    void clearState();
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
    struct Impl;
    std::unique_ptr<Impl> impl_;
    QPoint lastMousePos_;
    Qt::MouseButton activeButton_ = Qt::NoButton;
};

class MujocoPanel : public QWidget {
    Q_OBJECT

public:
    explicit MujocoPanel(QWidget *parent = nullptr);
    ~MujocoPanel() override;

    void clearState();
    void updateAxisPositions(const QVector<double> &positions);
    void reloadModel();

signals:
    void modelLoaded(int jointCount);

private:
    void setupUI();

    MujocoVisualizer3D *mujocoView = nullptr;
};
