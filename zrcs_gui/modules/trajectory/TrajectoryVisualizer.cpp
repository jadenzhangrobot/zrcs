#include "trajectory/TrajectoryVisualizer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMatrix4x4>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <cmath>
#include "ui_trajectory_panel.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// TrajectoryVisualizer3D 实现
// ============================================================================

TrajectoryVisualizer3D::TrajectoryVisualizer3D(QWidget *parent)
    : QOpenGLWidget(parent),
      eePos(0, 0, 0),
      eeRpy(0, 0, 0),
      hasEEPose(false),
      trajectoryColor(QColor("#66a3ff")),
            rotateX(30.0f),
            rotateY(45.0f),
      distance(220.0f),
      panX(0.0f),
    panY(-18.0f),
      activeButton(Qt::NoButton)
{
    setMinimumHeight(360);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

TrajectoryVisualizer3D::~TrajectoryVisualizer3D()
{
}

void TrajectoryVisualizer3D::setTrajectoryPoints(const QVector<QVector3D> &points)
{
    trajectoryPoints = points;
    update();
}

void TrajectoryVisualizer3D::setEndEffectorPose(const QVector3D &pos, const QVector3D &rpy)
{
    eePos = pos;
    eeRpy = rpy;
    if (!hasEEPose) {
        hasEEPose = true;
    } else {
        // 与上次位置距离 > 0.001 时才记录轨迹历史
        if (trajectoryHistory.isEmpty() ||
            (pos - trajectoryHistory.last()).lengthSquared() > 1e-6f) {
            trajectoryHistory.append(pos);
        }
    }
    // 限制轨迹历史长度
    while (trajectoryHistory.size() > 5000) {
        trajectoryHistory.removeFirst();
    }
    update();
}

void TrajectoryVisualizer3D::clearTrajectory()
{
    trajectoryPoints.clear();
    trajectoryHistory.clear();
    hasEEPose = false;
    update();
}

void TrajectoryVisualizer3D::setTrajectoryColor(const QColor &color)
{
    trajectoryColor = color;
    update();
}

void TrajectoryVisualizer3D::resetView()
{
    rotateX = 30.0f;
    rotateY = 45.0f;
    distance = 220.0f;
    panX = 0.0f;
    panY = -18.0f;
    update();
}

void TrajectoryVisualizer3D::setTopView()
{
    rotateX = 90.0f;
    rotateY = 0.0f;
    panX = 0.0f;
    panY = 0.0f;
    update();
}

void TrajectoryVisualizer3D::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

void TrajectoryVisualizer3D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void TrajectoryVisualizer3D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#101722"));

    QMatrix4x4 transform;
    transform.translate(panX, panY, -distance);
    transform.rotate(rotateX, 1.0f, 0.0f, 0.0f);
    transform.rotate(rotateY, 0.0f, 1.0f, 0.0f);
    transform.rotate(-90.0f, 1.0f, 0.0f, 0.0f);

    auto projectPoint = [this, &transform](const QVector3D &p, bool &ok) {
        QVector4D v = transform * QVector4D(p, 1.0f);
        float z = -v.z();
        if (z <= 0.1f) {
            ok = false;
            return QPointF();
        }
        float f = static_cast<float>(qMin(width(), height())) * 0.72f;
        float x = width() * 0.5f + (v.x() / z) * f;
        float y = height() * 0.5f - (v.y() / z) * f;
        ok = true;
        return QPointF(x, y);
    };

    drawGrid(painter, projectPoint);
    drawAxes(painter, projectPoint);
    drawTrajectoryPath(painter, projectPoint);
    drawEEFrame(painter, projectPoint);
}

void TrajectoryVisualizer3D::drawGrid(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    const int gridSize = 120;
    const int gridStep = 20;
    painter.setPen(QPen(QColor("#27364a"), 1.0));
    for (int i = -gridSize; i <= gridSize; i += gridStep) {
        bool ok1 = false, ok2 = false, ok3 = false, ok4 = false;
        QPointF a = projectPoint(QVector3D(static_cast<float>(i), -gridSize, 0.0f), ok1);
        QPointF b = projectPoint(QVector3D(static_cast<float>(i), gridSize, 0.0f), ok2);
        QPointF c = projectPoint(QVector3D(-gridSize, static_cast<float>(i), 0.0f), ok3);
        QPointF d = projectPoint(QVector3D(gridSize, static_cast<float>(i), 0.0f), ok4);
        if (ok1 && ok2) painter.drawLine(a, b);
        if (ok3 && ok4) painter.drawLine(c, d);
    }
}

void TrajectoryVisualizer3D::drawAxes(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QVector3D axes[6] = {
        QVector3D(-120.0f, 0.0f, 0.0f), QVector3D(120.0f, 0.0f, 0.0f),
        QVector3D(0.0f, -120.0f, 0.0f), QVector3D(0.0f, 120.0f, 0.0f),
        QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 120.0f)
    };
    QColor axisColors[3] = { QColor("#ef6b73"), QColor("#6ec27d"), QColor("#5da9ff") };
    for (int i = 0; i < 3; ++i) {
        bool ok1 = false;
        bool ok2 = false;
        QPointF p1 = projectPoint(axes[i * 2], ok1);
        QPointF p2 = projectPoint(axes[i * 2 + 1], ok2);
        if (ok1 && ok2) {
            painter.setPen(QPen(axisColors[i], 1.5));
            painter.drawLine(p1, p2);
        }
    }
    painter.setPen(QColor("#5da9ff"));
    bool xOk = false;
    QPointF xTip = projectPoint(QVector3D(132.0f, 0.0f, 0.0f), xOk);
    if (xOk) painter.drawText(xTip + QPointF(4, -4), "X");
    painter.setPen(QColor("#6ec27d"));
    bool yOk = false;
    QPointF yTip = projectPoint(QVector3D(0.0f, 132.0f, 0.0f), yOk);
    if (yOk) painter.drawText(yTip + QPointF(4, -4), "Y");
    painter.setPen(QColor("#ef6b73"));
    bool zOk = false;
    QPointF zTip = projectPoint(QVector3D(0.0f, 0.0f, 132.0f), zOk);
    if (zOk) painter.drawText(zTip + QPointF(4, -4), "Z");
}

void TrajectoryVisualizer3D::drawTrajectoryPath(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    if (trajectoryPoints.size() >= 2) {
        painter.setPen(QPen(trajectoryColor, 2.2));
        QPointF lastPoint;
        bool hasLastPoint = false;
        for (const QVector3D &pt : trajectoryPoints) {
            bool ok = false;
            QPointF current = projectPoint(pt, ok);
            if (!ok) {
                hasLastPoint = false;
                continue;
            }
            if (hasLastPoint) {
                painter.drawLine(lastPoint, current);
            }
            lastPoint = current;
            hasLastPoint = true;
        }

        bool startOk = false;
        bool endOk = false;
        QPointF startPt = projectPoint(trajectoryPoints.first(), startOk);
        QPointF endPt = projectPoint(trajectoryPoints.last(), endOk);
        if (startOk) {
            painter.setBrush(QColor("#6ec27d"));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(startPt, 4.5, 4.5);
        }
        if (endOk) {
            painter.setBrush(QColor("#ef6b73"));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(endPt, 4.5, 4.5);
        }
    }

    // 实时轨迹历史线 (金色)
    if (trajectoryHistory.size() >= 2) {
        painter.setPen(QPen(QColor("#FFD700"), 1.8));
        QPointF lastPoint;
        bool hasLastPoint = false;
        for (const QVector3D &pt : trajectoryHistory) {
            bool ok = false;
            QPointF current = projectPoint(pt, ok);
            if (!ok) {
                hasLastPoint = false;
                continue;
            }
            if (hasLastPoint) {
                painter.drawLine(lastPoint, current);
            }
            lastPoint = current;
            hasLastPoint = true;
        }
    }
}

void TrajectoryVisualizer3D::drawEEFrame(QPainter &painter, const std::function<QPointF(const QVector3D&, bool&)> &projectPoint)
{
    if (!hasEEPose) return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    // RPY (ZYX convention) → 旋转矩阵
    float rx = eeRpy.x() * M_PI / 180.0f;
    float ry = eeRpy.y() * M_PI / 180.0f;
    float rz = eeRpy.z() * M_PI / 180.0f;

    float sx = std::sin(rx), cx = std::cos(rx);
    float sy = std::sin(ry), cy = std::cos(ry);
    float sz = std::sin(rz), cz = std::cos(rz);

    // R = Rz * Ry * Rx
    // X 轴方向
    QVector3D axisX(
        cy * cz,
        cz * sx * sy + cx * sz,
        -cx * cz * sy + sx * sz
    );
    // Y 轴方向
    QVector3D axisY(
        -cy * sz,
        cx * cz - sx * sy * sz,
        cz * sx + cx * sy * sz
    );
    // Z 轴方向
    QVector3D axisZ(
        sy,
        -cy * sx,
        cx * cy
    );

    const float axisLen = 15.0f;
    QVector3D xEnd = eePos + axisX * axisLen;
    QVector3D yEnd = eePos + axisY * axisLen;
    QVector3D zEnd = eePos + axisZ * axisLen;

    // 绘制坐标轴 (红X / 绿Y / 蓝Z)
    struct { QVector3D start; QVector3D end; QColor color; float width; } lines[] = {
        {eePos, xEnd, QColor("#ef6b73"), 2.5f},
        {eePos, yEnd, QColor("#6ec27d"), 2.5f},
        {eePos, zEnd, QColor("#5da9ff"), 2.5f},
    };

    for (const auto &line : lines) {
        bool ok1 = false, ok2 = false;
        QPointF p1 = projectPoint(line.start, ok1);
        QPointF p2 = projectPoint(line.end, ok2);
        if (ok1 && ok2) {
            painter.setPen(QPen(line.color, line.width));
            painter.drawLine(p1, p2);
        }
    }

    // 绘制 TCP 原点圆点
    bool originOk = false;
    QPointF origin = projectPoint(eePos, originOk);
    if (originOk) {
        painter.setBrush(QColor("#FFFFFF"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(origin, 4.0, 4.0);
    }
}

void TrajectoryVisualizer3D::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    distance -= delta * 0.08f;
    distance = qBound(40.0f, distance, 900.0f);
    update();
}

void TrajectoryVisualizer3D::mousePressEvent(QMouseEvent *event)
{
    activeButton = event->button();
    lastMousePos = event->pos();
    event->accept();
}

void TrajectoryVisualizer3D::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    if (activeButton == Qt::LeftButton) {
        rotateY += delta.x() * 0.5f;
        rotateX += delta.y() * 0.5f;
    } else if (activeButton == Qt::RightButton || activeButton == Qt::MiddleButton) {
        panX += delta.x() * 0.28f;
        panY -= delta.y() * 0.28f;
    }
    update();
    event->accept();
}

void TrajectoryVisualizer3D::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == activeButton) {
        activeButton = Qt::NoButton;
    }
    event->accept();
}

// ============================================================================
// TrajectoryPanel 实现
// ============================================================================

TrajectoryPanel::TrajectoryPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

TrajectoryPanel::~TrajectoryPanel()
{
}

void TrajectoryPanel::setupUI()
{
    Ui::TrajectoryPanelUi ui;
    ui.setupUi(this);
    visualizer3D = findChild<TrajectoryVisualizer3D*>("visualizer3D");

    QPushButton *btn2D = findChild<QPushButton*>("btn2D");
    QPushButton *btn3D = findChild<QPushButton*>("btn3D");
    QPushButton *btnResetView = findChild<QPushButton*>("btnResetView");
    QPushButton *btnClear = findChild<QPushButton*>("btnClear");
    if (!visualizer3D || !btn2D || !btn3D || !btnResetView || !btnClear) {
        return;
    }

    btn2D->setProperty("kind", "accent");
    btn3D->setProperty("kind", "accentBlue");
    btnResetView->setProperty("kind", "neutral");
    btnClear->setProperty("kind", "danger");

    connect(btn2D, &QPushButton::clicked, visualizer3D, &TrajectoryVisualizer3D::setTopView);
    connect(btn3D, &QPushButton::clicked, visualizer3D, &TrajectoryVisualizer3D::resetView);
    connect(btnResetView, &QPushButton::clicked, visualizer3D, &TrajectoryVisualizer3D::resetView);
    connect(btnClear, &QPushButton::clicked, this, &TrajectoryPanel::clearTrajectory);
}

void TrajectoryPanel::clearTrajectory()
{
    visualizer3D->clearTrajectory();
    trajectoryPoints3D.clear();
}

void TrajectoryPanel::updateEndEffectorPose(const QVector3D &pos, const QVector3D &rpy)
{
    visualizer3D->setEndEffectorPose(pos, rpy);
}
