#include "trajectory/TrajectoryVisualizer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMatrix4x4>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include "ui_trajectory_panel.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifdef ZRCSGUI_HAS_OPENCASCADE
#include <STEPControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <gp_Pnt.hxx>
#endif

// ============================================================================
// TrajectoryVisualizer3D 实现
// ============================================================================

TrajectoryVisualizer3D::TrajectoryVisualizer3D(QWidget *parent)
    : QOpenGLWidget(parent),
      wireframeOverlayEnabled(true),
      trajectoryColor(QColor("#66a3ff")),
      rotateX(-25.0f),
      rotateY(35.0f),
      distance(220.0f),
      panX(0.0f),
      panY(0.0f),
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

void TrajectoryVisualizer3D::setModelMesh(const QVector<QVector3D> &triangles, const QVector<QVector3D> &segments)
{
    modelTriangles = triangles;
    modelLineSegments = segments;
    update();
}

void TrajectoryVisualizer3D::setModelLineSegments(const QVector<QVector3D> &segments)
{
    modelLineSegments = segments;
    update();
}

void TrajectoryVisualizer3D::setWireframeOverlayEnabled(bool enabled)
{
    wireframeOverlayEnabled = enabled;
    update();
}

void TrajectoryVisualizer3D::clearTrajectory()
{
    trajectoryPoints.clear();
    modelTriangles.clear();
    modelLineSegments.clear();
    update();
}

void TrajectoryVisualizer3D::setTrajectoryColor(const QColor &color)
{
    trajectoryColor = color;
    update();
}

void TrajectoryVisualizer3D::resetView()
{
    rotateX = -25.0f;
    rotateY = 35.0f;
    distance = 220.0f;
    panX = 0.0f;
    panY = 0.0f;
    update();
}

void TrajectoryVisualizer3D::setTopView()
{
    rotateX = 0.0f;
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

    QVector3D axes[6] = {
        QVector3D(-120.0f, 0.0f, 0.0f), QVector3D(120.0f, 0.0f, 0.0f),
        QVector3D(0.0f, -120.0f, 0.0f), QVector3D(0.0f, 120.0f, 0.0f),
        QVector3D(0.0f, 0.0f, -120.0f), QVector3D(0.0f, 0.0f, 120.0f)
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

    if (!modelTriangles.isEmpty()) {
        struct TriangleDraw {
            QPointF p1;
            QPointF p2;
            QPointF p3;
            float depth;
            QColor color;
        };
        QVector<TriangleDraw> drawTriangles;
        drawTriangles.reserve(modelTriangles.size() / 3);

        const QVector3D lightDir = QVector3D(0.35f, -0.25f, -1.0f).normalized();
        const QColor baseColor("#8aaee0");

        for (int i = 0; i + 2 < modelTriangles.size(); i += 3) {
            QVector3D p1 = modelTriangles[i];
            QVector3D p2 = modelTriangles[i + 1];
            QVector3D p3 = modelTriangles[i + 2];
            QVector4D v1 = transform * QVector4D(p1, 1.0f);
            QVector4D v2 = transform * QVector4D(p2, 1.0f);
            QVector4D v3 = transform * QVector4D(p3, 1.0f);
            float z1 = -v1.z();
            float z2 = -v2.z();
            float z3 = -v3.z();
            if (z1 <= 0.1f || z2 <= 0.1f || z3 <= 0.1f) {
                continue;
            }

            float f = static_cast<float>(qMin(width(), height())) * 0.72f;
            QPointF s1(width() * 0.5f + (v1.x() / z1) * f, height() * 0.5f - (v1.y() / z1) * f);
            QPointF s2(width() * 0.5f + (v2.x() / z2) * f, height() * 0.5f - (v2.y() / z2) * f);
            QPointF s3(width() * 0.5f + (v3.x() / z3) * f, height() * 0.5f - (v3.y() / z3) * f);

            QVector3D cv1(v1.x(), v1.y(), v1.z());
            QVector3D cv2(v2.x(), v2.y(), v2.z());
            QVector3D cv3(v3.x(), v3.y(), v3.z());
            QVector3D normal = QVector3D::crossProduct(cv2 - cv1, cv3 - cv1);
            if (normal.lengthSquared() < 1e-8f) {
                continue;
            }
            normal.normalize();
            float intensity = qAbs(QVector3D::dotProduct(normal, -lightDir));
            intensity = qBound(0.18f, intensity, 1.0f);
            QColor shaded = baseColor;
            shaded.setRedF(qBound(0.0, baseColor.redF() * intensity, 1.0));
            shaded.setGreenF(qBound(0.0, baseColor.greenF() * intensity, 1.0));
            shaded.setBlueF(qBound(0.0, baseColor.blueF() * intensity, 1.0));

            drawTriangles.append({s1, s2, s3, (z1 + z2 + z3) / 3.0f, shaded});
        }

        std::sort(drawTriangles.begin(), drawTriangles.end(), [](const TriangleDraw &a, const TriangleDraw &b) {
            return a.depth > b.depth;
        });

        painter.setPen(Qt::NoPen);
        for (const TriangleDraw &t : drawTriangles) {
            painter.setBrush(t.color);
            QPolygonF poly;
            poly << t.p1 << t.p2 << t.p3;
            painter.drawPolygon(poly);
        }
    }

    if (wireframeOverlayEnabled && !modelLineSegments.isEmpty()) {
        painter.setPen(QPen(QColor("#8ec7ff"), 1.0));
        for (int i = 0; i + 1 < modelLineSegments.size(); i += 2) {
            bool ok1 = false;
            bool ok2 = false;
            QPointF p1 = projectPoint(modelLineSegments[i], ok1);
            QPointF p2 = projectPoint(modelLineSegments[i + 1], ok2);
            if (ok1 && ok2) {
                painter.drawLine(p1, p2);
            }
        }
    }

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
    QPushButton *btnImport = findChild<QPushButton*>("btnImport");
    QPushButton *btnWireframe = findChild<QPushButton*>("btnWireframe");
    QPushButton *btnResetView = findChild<QPushButton*>("btnResetView");
    QPushButton *btnClear = findChild<QPushButton*>("btnClear");
    if (!visualizer3D || !btn2D || !btn3D || !btnImport || !btnWireframe || !btnResetView || !btnClear) {
        return;
    }
    
    btn2D->setProperty("kind", "accent");
    btn3D->setProperty("kind", "accentBlue");
    btnImport->setProperty("kind", "accent");
    btnWireframe->setProperty("kind", "neutral");
    btnResetView->setProperty("kind", "neutral");
    btnClear->setProperty("kind", "danger");
    btnWireframe->setCheckable(true);
    btnWireframe->setChecked(true);
    
    connect(btn2D, &QPushButton::clicked, visualizer3D, &TrajectoryVisualizer3D::setTopView);
    connect(btn3D, &QPushButton::clicked, this, &TrajectoryPanel::switchTo3DView);
    connect(btnImport, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "导入轨迹文件",
            "",
            "轨迹文件 (*.gcode *.nc *.tap *.txt *.step *.stp);;G-code 文件 (*.gcode *.nc *.tap *.txt);;STEP 文件 (*.step *.stp)"
        );
        if (!filePath.isEmpty()) {
            if (filePath.endsWith(".step", Qt::CaseInsensitive) || filePath.endsWith(".stp", Qt::CaseInsensitive)) {
                loadSTEPFile(filePath);
            } else {
                loadGCodeFile(filePath);
            }
        }
    });
    connect(btnWireframe, &QPushButton::toggled, visualizer3D, &TrajectoryVisualizer3D::setWireframeOverlayEnabled);
    connect(btnResetView, &QPushButton::clicked, visualizer3D, &TrajectoryVisualizer3D::resetView);
    connect(btnClear, &QPushButton::clicked, this, &TrajectoryPanel::clearTrajectory);
    
}

void TrajectoryPanel::loadGCodeFile(const QString &filePath)
{
    parseGCode(filePath);
}

void TrajectoryPanel::loadSTEPFile(const QString &filePath)
{
    parseSTEP(filePath);
}

void TrajectoryPanel::addTrajectoryPoints(const QVector<QPointF> &points2D)
{
    trajectoryPoints2D = points2D;
    trajectoryPoints3D = to3DPoints(points2D);
    visualizer3D->setTrajectoryPoints(trajectoryPoints3D);
    emit trajectoryLoaded(points2D.size());
}

void TrajectoryPanel::switchTo2DView()
{
    visualizer3D->setTopView();
    emit viewModeChanged("2D");
}

void TrajectoryPanel::switchTo3DView()
{
    visualizer3D->resetView();
    emit viewModeChanged("3D");
}

void TrajectoryPanel::clearTrajectory()
{
    visualizer3D->clearTrajectory();
    trajectoryPoints2D.clear();
    trajectoryPoints3D.clear();
}

void TrajectoryPanel::parseGCode(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开 G-code 文件:" << filePath;
        return;
    }

    QTextStream in(&file);
    QVector<QVector3D> rawPoints;
    rawPoints.reserve(4096);

    double currentX = 0.0;
    double currentY = 0.0;
    double currentZ = 0.0;
    bool seenMotion = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        int semicolonPos = line.indexOf(';');
        if (semicolonPos >= 0) {
            line = line.left(semicolonPos).trimmed();
        }
        line.remove(QRegularExpression("\\([^\\)]*\\)"));
        line = line.trimmed();
        if (line.isEmpty()) continue;

        bool isMotionLine = line.contains(QRegularExpression("\\bG0?0\\b|\\bG0?1\\b", QRegularExpression::CaseInsensitiveOption));
        bool hasCoordinate = line.contains(QRegularExpression("[XYZ][-+]?\\d*\\.?\\d+", QRegularExpression::CaseInsensitiveOption));
        if (!isMotionLine && !hasCoordinate) {
            continue;
        }

        QRegularExpression re("([XYZ])\\s*([-+]?\\d*\\.?\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = re.globalMatch(line);
        bool changed = false;
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            const QString axis = m.captured(1).toUpper();
            const double v = m.captured(2).toDouble();
            if (axis == "X") {
                currentX = v;
                changed = true;
            } else if (axis == "Y") {
                currentY = v;
                changed = true;
            } else if (axis == "Z") {
                currentZ = v;
                changed = true;
            }
        }

        if (changed || (isMotionLine && !seenMotion)) {
            rawPoints.append(QVector3D(static_cast<float>(currentX), static_cast<float>(currentY), static_cast<float>(currentZ)));
            seenMotion = true;
        }
    }

    file.close();

    if (rawPoints.size() < 2) {
        qWarning() << "G-code 轨迹点不足，无法显示:" << filePath;
        clearTrajectory();
        return;
    }

    trajectoryPoints3D = normalize3DPoints(rawPoints);
    trajectoryPoints2D.clear();
    trajectoryPoints2D.reserve(trajectoryPoints3D.size());
    for (const QVector3D &p : trajectoryPoints3D) {
        trajectoryPoints2D.append(QPointF(p.x(), p.y()));
    }
    visualizer3D->setTrajectoryPoints(trajectoryPoints3D);
    emit trajectoryLoaded(trajectoryPoints3D.size());
}

void TrajectoryPanel::parseSTEP(const QString &filePath)
{
    bool occOk = false;
    QVector<QVector3D> occTriangles;
    QVector<QVector3D> occSegments = extractStepMeshOCC(filePath, occTriangles, occOk);
    if (occOk && !occSegments.isEmpty()) {
        visualizer3D->setModelMesh(occTriangles, occSegments);
        trajectoryPoints3D.clear();
        trajectoryPoints2D.clear();
        emit trajectoryLoaded(occSegments.size() / 2);
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开 STEP 文件:" << filePath;
        return;
    }

    QVector<QVector3D> rawPoints;
    rawPoints.reserve(2048);

    QTextStream in(&file);
    QRegularExpression pointRe(
        "CARTESIAN_POINT\\s*\\(\\s*'[^']*'\\s*,\\s*\\(\\s*([-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)\\s*,\\s*([-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)\\s*,\\s*([-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)\\s*\\)\\s*\\)",
        QRegularExpression::CaseInsensitiveOption
    );

    while (!in.atEnd()) {
        const QString line = in.readLine();
        QRegularExpressionMatch match = pointRe.match(line);
        if (!match.hasMatch()) {
            continue;
        }
        bool okX = false;
        bool okY = false;
        bool okZ = false;
        const float x = match.captured(1).toFloat(&okX);
        const float y = match.captured(2).toFloat(&okY);
        const float z = match.captured(3).toFloat(&okZ);
        if (okX && okY && okZ) {
            rawPoints.append(QVector3D(x, y, z));
        }
    }

    file.close();

    if (rawPoints.size() < 2) {
        qWarning() << "STEP 点数据不足，无法显示:" << filePath;
        clearTrajectory();
        return;
    }

    trajectoryPoints3D = normalize3DPoints(rawPoints);
    trajectoryPoints2D.clear();
    trajectoryPoints2D.reserve(trajectoryPoints3D.size());
    for (const QVector3D &p : trajectoryPoints3D) {
        trajectoryPoints2D.append(QPointF(p.x(), p.y()));
    }
    visualizer3D->setTrajectoryPoints(trajectoryPoints3D);
    emit trajectoryLoaded(trajectoryPoints3D.size());
}

QVector<QVector3D> TrajectoryPanel::to3DPoints(const QVector<QPointF> &points2D)
{
    QVector<QVector3D> points3D;
    points3D.reserve(points2D.size());
    if (points2D.isEmpty()) {
        return points3D;
    }

    float minX = static_cast<float>(points2D.first().x());
    float maxX = minX;
    float minY = static_cast<float>(points2D.first().y());
    float maxY = minY;

    for (const QPointF &p : points2D) {
        minX = qMin(minX, static_cast<float>(p.x()));
        maxX = qMax(maxX, static_cast<float>(p.x()));
        minY = qMin(minY, static_cast<float>(p.y()));
        maxY = qMax(maxY, static_cast<float>(p.y()));
    }

    const float centerX = (minX + maxX) * 0.5f;
    const float centerY = (minY + maxY) * 0.5f;
    const float span = qMax(maxX - minX, maxY - minY);
    const float scale = span > 1e-6f ? 180.0f / span : 1.0f;

    for (const QPointF &p : points2D) {
        float x = (static_cast<float>(p.x()) - centerX) * scale;
        float y = (static_cast<float>(p.y()) - centerY) * scale;
        points3D.append(QVector3D(x, -y, 0.0f));
    }

    return points3D;
}

QVector<QVector3D> TrajectoryPanel::normalize3DPoints(const QVector<QVector3D> &points3D)
{
    QVector<QVector3D> normalized;
    normalized.reserve(points3D.size());
    if (points3D.isEmpty()) {
        return normalized;
    }

    float minX = points3D.first().x();
    float maxX = minX;
    float minY = points3D.first().y();
    float maxY = minY;
    float minZ = points3D.first().z();
    float maxZ = minZ;

    for (const QVector3D &p : points3D) {
        minX = qMin(minX, p.x());
        maxX = qMax(maxX, p.x());
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
        minZ = qMin(minZ, p.z());
        maxZ = qMax(maxZ, p.z());
    }

    const float centerX = (minX + maxX) * 0.5f;
    const float centerY = (minY + maxY) * 0.5f;
    const float centerZ = (minZ + maxZ) * 0.5f;
    const float span = qMax(qMax(maxX - minX, maxY - minY), maxZ - minZ);
    const float scale = span > 1e-6f ? 180.0f / span : 1.0f;

    for (const QVector3D &p : points3D) {
        normalized.append(QVector3D(
            (p.x() - centerX) * scale,
            -(p.y() - centerY) * scale,
            (p.z() - centerZ) * scale
        ));
    }
    return normalized;
}

QVector<QVector3D> TrajectoryPanel::extractStepMeshOCC(const QString &filePath, QVector<QVector3D> &trianglesOut, bool &ok)
{
    ok = false;
    QVector<QVector3D> segments;
    trianglesOut.clear();
#ifdef ZRCSGUI_HAS_OPENCASCADE
    STEPControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(filePath.toStdString().c_str());
    if (status != IFSelect_RetDone) {
        return segments;
    }

    reader.TransferRoots();
    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull()) {
        return segments;
    }

    BRepMesh_IncrementalMesh mesher(shape, 0.8, false, 0.5, false);
    (void)mesher;

    QVector<QVector3D> triangleVertices;
    triangleVertices.reserve(12288);
    QVector<QVector3D> lineVertices;
    lineVertices.reserve(24576);
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) {
            continue;
        }

        gp_Trsf trsf = loc.Transformation();
        for (int t = 1; t <= tri->NbTriangles(); ++t) {
            int n1 = 0, n2 = 0, n3 = 0;
            tri->Triangle(t).Get(n1, n2, n3);
            gp_Pnt p1 = tri->Node(n1).Transformed(trsf);
            gp_Pnt p2 = tri->Node(n2).Transformed(trsf);
            gp_Pnt p3 = tri->Node(n3).Transformed(trsf);
            QVector3D v1(static_cast<float>(p1.X()), static_cast<float>(p1.Y()), static_cast<float>(p1.Z()));
            QVector3D v2(static_cast<float>(p2.X()), static_cast<float>(p2.Y()), static_cast<float>(p2.Z()));
            QVector3D v3(static_cast<float>(p3.X()), static_cast<float>(p3.Y()), static_cast<float>(p3.Z()));
            triangleVertices.append(v1);
            triangleVertices.append(v2);
            triangleVertices.append(v3);

            lineVertices.append(v1);
            lineVertices.append(v2);
            lineVertices.append(v2);
            lineVertices.append(v3);
            lineVertices.append(v3);
            lineVertices.append(v1);
        }
    }

    if (triangleVertices.size() < 3) {
        return segments;
    }

    trianglesOut = normalize3DPoints(triangleVertices);
    segments = normalize3DPoints(lineVertices);

    ok = true;
#else
    Q_UNUSED(filePath);
#endif
    return segments;
}
