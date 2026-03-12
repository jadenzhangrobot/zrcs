#include "trajectory/trajectoryVisualizer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>

// ============================================================================
// TrajectoryVisualizer2D 实现
// ============================================================================

TrajectoryVisualizer2D::TrajectoryVisualizer2D(QWidget *parent)
    : QGraphicsView(parent), trajectoryColor(Qt::green), zoomLevel(1.0), isPanning(false)
{
    scene = new QGraphicsScene(this);
    setScene(scene);
    
    scene->setBackgroundBrush(QBrush(QColor("#1a1a1a")));
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setStyleSheet("QGraphicsView { border: 1px solid #444; background-color: #1a1a1a; }");
}

TrajectoryVisualizer2D::~TrajectoryVisualizer2D()
{
}

void TrajectoryVisualizer2D::addTrajectoryPoints(const QVector<QPointF> &points)
{
    if (points.isEmpty()) return;
    
    scene->clear();
    
    for (int i = 0; i < points.size() - 1; ++i) {
        QGraphicsLineItem *line = scene->addLine(
            points[i].x(), points[i].y(),
            points[i + 1].x(), points[i + 1].y()
        );
        line->setPen(QPen(trajectoryColor, 2));
    }
    
    if (!points.isEmpty()) {
        QGraphicsEllipseItem *startPoint = scene->addEllipse(
            points.first().x() - 5, points.first().y() - 5, 10, 10
        );
        startPoint->setBrush(QBrush(Qt::green));
        startPoint->setPen(QPen(Qt::green));
        
        QGraphicsEllipseItem *endPoint = scene->addEllipse(
            points.last().x() - 5, points.last().y() - 5, 10, 10
        );
        endPoint->setBrush(QBrush(Qt::red));
        endPoint->setPen(QPen(Qt::red));
    }
    
    fitInView();
}

void TrajectoryVisualizer2D::clearTrajectory()
{
    scene->clear();
}

void TrajectoryVisualizer2D::setTrajectoryColor(const QColor &color)
{
    trajectoryColor = color;
}

void TrajectoryVisualizer2D::fitInView()
{
    if (scene->itemsBoundingRect().isEmpty()) return;
    
    QGraphicsView::fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    zoomLevel = 1.0;
}

double TrajectoryVisualizer2D::getZoomLevel() const
{
    return zoomLevel;
}

void TrajectoryVisualizer2D::setZoomLevel(double level)
{
    zoomLevel = level;
    scale(level, level);
}

void TrajectoryVisualizer2D::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        scale(1.2, 1.2);
        zoomLevel *= 1.2;
    } else {
        scale(0.8, 0.8);
        zoomLevel *= 0.8;
    }
}

void TrajectoryVisualizer2D::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        isPanning = true;
        lastMousePos = event->pos();
    }
    QGraphicsView::mousePressEvent(event);
}

void TrajectoryVisualizer2D::mouseMoveEvent(QMouseEvent *event)
{
    if (isPanning) {
        QPointF p1 = mapToScene(event->pos().x(), event->pos().y());
        QPointF p2 = mapToScene(lastMousePos.x(), lastMousePos.y());
        QPointF delta = p1 - p2;
        translate(delta.x(), delta.y());
        lastMousePos = event->pos();
    }
    QGraphicsView::mouseMoveEvent(event);
}

void TrajectoryVisualizer2D::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        isPanning = false;
    }
    QGraphicsView::mouseReleaseEvent(event);
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
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    stackedWidget = new QStackedWidget();
    visualizer2D = new TrajectoryVisualizer2D();
    stackedWidget->addWidget(visualizer2D);
    
    mainLayout->addWidget(stackedWidget);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *btn2D = new QPushButton("2D 视图");
    QPushButton *btnClear = new QPushButton("清空");
    
    btn2D->setStyleSheet("background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; padding: 8px;");
    btnClear->setStyleSheet("background-color: #5a2a2a; color: #FF6347; border: 1px solid #FF6347; padding: 8px;");
    
    connect(btn2D, &QPushButton::clicked, this, &TrajectoryPanel::switchTo2DView);
    connect(btnClear, &QPushButton::clicked, this, &TrajectoryPanel::clearTrajectory);
    
    buttonLayout->addWidget(btn2D);
    buttonLayout->addWidget(btnClear);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    setStyleSheet("background-color: #1a1a1a;");
}

void TrajectoryPanel::loadGCodeFile(const QString &filePath)
{
    qDebug() << "加载 G-code 文件:" << filePath;
}

void TrajectoryPanel::addTrajectoryPoints(const QVector<QPointF> &points2D)
{
    trajectoryPoints2D = points2D;
    visualizer2D->addTrajectoryPoints(points2D);
    emit trajectoryLoaded(points2D.size());
}

void TrajectoryPanel::switchTo2DView()
{
    stackedWidget->setCurrentWidget(visualizer2D);
    emit viewModeChanged("2D");
}

void TrajectoryPanel::clearTrajectory()
{
    visualizer2D->clearTrajectory();
    trajectoryPoints2D.clear();
}

void TrajectoryPanel::parseGCode(const QString &filePath)
{
    // TODO: 实现 G-code 解析逻辑
}
