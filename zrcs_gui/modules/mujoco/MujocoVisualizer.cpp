#include "mujoco/MujocoVisualizer.h"

#include "ui_mujoco_panel.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

#ifdef ZRCS_HAS_MUJOCO
#include "config/ConfigManager.h"
#include "controller/mujoco/MujocoConfig.h"

#include <mujoco/mujoco.h>
#endif

struct MujocoVisualizer3D::Impl {
    QString message = QStringLiteral("MuJoCo model is not loaded.");
    bool glReady = false;
    QVector<double> latestAxisPositions;

#ifdef ZRCS_HAS_MUJOCO
    struct JointBinding {
        int axisId = -1;
        uint32_t slaveId = 0;
        std::string jointName;
        int qposAdr = -1;
        double qposScale = 1.0;
        double qposOffset = 0.0;
    };

    mjModel *model = nullptr;
    mjData *data = nullptr;
    mjvCamera camera;
    mjvOption option;
    mjvScene scene;
    mjrContext context;
    bool sceneReady = false;
    bool renderContextReady = false;
    QVector<JointBinding> bindings;
    int toolTipGeomId = -1;
    QVector<std::array<mjtNum, 3>> toolTrail;
    bool toolTrailVisible = true; ///< 采样 + 绘制开关
    static constexpr int kMaxTrailPoints = 20000;
    static constexpr int kMaxRenderedTrailSegments = 2500;
    static constexpr mjtNum kMinTrailDistance = 0.0005;
    static constexpr mjtNum kTrailDisplayZ = 0.121;
    static constexpr mjtNum kTrailRadius = 0.0005;

    Impl()
    {
        mjv_defaultCamera(&camera);
        mjv_defaultOption(&option);
        mjv_defaultScene(&scene);
        mjr_defaultContext(&context);
    }

    ~Impl()
    {
        release();
    }

    void release()
    {
        if (renderContextReady) {
            mjr_freeContext(&context);
            renderContextReady = false;
            mjr_defaultContext(&context);
        }
        if (sceneReady) {
            mjv_freeScene(&scene);
            sceneReady = false;
            mjv_defaultScene(&scene);
        }
        if (data) {
            mj_deleteData(data);
            data = nullptr;
        }
        if (model) {
            mj_deleteModel(model);
            model = nullptr;
        }
        bindings.clear();
        toolTipGeomId = -1;
        toolTrail.clear();
    }

    bool ready() const
    {
        return model && data && sceneReady && renderContextReady;
    }

    void appendToolTipSample()
    {
        if (!toolTrailVisible || !model || !data || toolTipGeomId < 0) {
            return;
        }

        const mjtNum *p = data->geom_xpos + 3 * toolTipGeomId;
        std::array<mjtNum, 3> point{p[0], p[1], kTrailDisplayZ};
        if (!toolTrail.isEmpty()) {
            const auto &last = toolTrail.back();
            const mjtNum dx = point[0] - last[0];
            const mjtNum dy = point[1] - last[1];
            const mjtNum dz = point[2] - last[2];
            const mjtNum dist2 = dx * dx + dy * dy + dz * dz;
            if (dist2 < kMinTrailDistance * kMinTrailDistance) {
                return;
            }
        }

        toolTrail.append(point);
        if (toolTrail.size() > kMaxTrailPoints) {
            toolTrail.remove(0, toolTrail.size() - kMaxTrailPoints);
        }
    }

    void addTrailToScene()
    {
        if (!toolTrailVisible || !sceneReady || toolTrail.isEmpty()) {
            return;
        }

        const float trailRgba[4] = {1.00f, 0.22f, 0.48f, 1.00f};
        mjtNum size[3] = {0.0, 0.0, 0.0};
        mjtNum pos[3] = {0.0, 0.0, 0.0};
        mjtNum mat[9] = {1.0, 0.0, 0.0,
                         0.0, 1.0, 0.0,
                         0.0, 0.0, 1.0};

        const int available = std::min(scene.maxgeom - scene.ngeom, kMaxRenderedTrailSegments);
        if (available <= 0) {
            return;
        }

        if (toolTrail.size() >= 2) {
            const int trailSegments = static_cast<int>(toolTrail.size() - 1);
            const int renderedSegments = std::min(trailSegments, available);
            for (int out = 0; out < renderedSegments && scene.ngeom < scene.maxgeom; ++out) {
                int i0 = static_cast<int>(
                    (static_cast<long long>(out) * trailSegments) / renderedSegments);
                int i1 = static_cast<int>(
                    (static_cast<long long>(out + 1) * trailSegments) / renderedSegments);
                if (i1 <= i0) {
                    i1 = std::min(i0 + 1, trailSegments);
                }

                mjvGeom *geom = scene.geoms + scene.ngeom++;
                mjv_initGeom(geom, mjGEOM_CAPSULE, size, pos, mat, trailRgba);
                mjv_connector(geom,
                              mjGEOM_CAPSULE,
                              kTrailRadius,
                              toolTrail[i0].data(),
                              toolTrail[i1].data());
                geom->category = mjCAT_DECOR;
                geom->emission = 0.45f;
            }
        }
    }
#else
    void release() {}
    bool ready() const { return false; }
#endif
};

MujocoVisualizer3D::MujocoVisualizer3D(QWidget *parent)
    : QOpenGLWidget(parent),
      impl_(std::make_unique<Impl>())
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

MujocoVisualizer3D::~MujocoVisualizer3D()
{
    if (impl_) {
        if (context()) {
            makeCurrent();
        }
        impl_->release();
        if (context()) {
            doneCurrent();
        }
    }
}

void MujocoVisualizer3D::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    impl_->glReady = true;
    reloadModel();
}

void MujocoVisualizer3D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void MujocoVisualizer3D::reloadModel()
{
#ifdef ZRCS_HAS_MUJOCO
    if (!impl_->glReady) {
        return;
    }

    makeCurrent();
    impl_->release();

    try {
        const auto configManager = zrcs::config::ConfigManager::load("");
        const auto projectDir = configManager.projectDir();
        if (!ZrcsHardware::MujocoConfig::existsInProject(projectDir)) {
            impl_->message = QStringLiteral("No mujoco.xml in current project.");
            doneCurrent();
            update();
            return;
        }

        const auto mujocoConfig = ZrcsHardware::MujocoConfig::load(projectDir);
        std::array<char, 1024> error{};
        impl_->model = mj_loadXML(mujocoConfig.modelPath.string().c_str(),
                                  nullptr,
                                  error.data(),
                                  static_cast<int>(error.size()));
        if (!impl_->model) {
            throw std::runtime_error(std::string("Load MJCF failed: ") + error.data());
        }

        impl_->data = mj_makeData(impl_->model);
        if (!impl_->data) {
            throw std::runtime_error("Failed to allocate mjData.");
        }

        for (const auto &axis : configManager.axisConfig().axes) {
            for (const auto slaveId : axis.servoSlaveIds) {
                const auto *servo = mujocoConfig.findServo(slaveId);
                if (!servo) {
                    continue;
                }

                const int jointId = mj_name2id(impl_->model, mjOBJ_JOINT, servo->joint.c_str());
                if (jointId < 0) {
                    throw std::runtime_error("MuJoCo joint not found: " + servo->joint);
                }

                const int jointType = impl_->model->jnt_type[jointId];
                if (jointType != mjJNT_HINGE && jointType != mjJNT_SLIDE) {
                    throw std::runtime_error("MuJoCo viewer supports hinge/slide joint only: " +
                                             servo->joint);
                }

                Impl::JointBinding binding;
                binding.axisId = static_cast<int>(axis.axisId);
                binding.slaveId = slaveId;
                binding.jointName = servo->joint;
                binding.qposAdr = impl_->model->jnt_qposadr[jointId];
                binding.qposScale = servo->qposScale;
                binding.qposOffset = servo->qposOffset;
                impl_->bindings.append(binding);
            }
        }

        if (impl_->bindings.isEmpty()) {
            throw std::runtime_error("No axis.xml servo maps to mujoco.xml servo.");
        }

        impl_->toolTipGeomId = mj_name2id(impl_->model, mjOBJ_GEOM, "tool_tip");

        mj_forward(impl_->model, impl_->data);
        impl_->appendToolTipSample();
        mjv_makeScene(impl_->model, &impl_->scene, 3500);
        impl_->sceneReady = true;
        impl_->scene.flags[mjRND_SHADOW] = 1;
        impl_->scene.flags[mjRND_REFLECTION] = 1;
        impl_->scene.flags[mjRND_SKYBOX] = 1;
        impl_->scene.flags[mjRND_HAZE] = 1;
        mjr_makeContext(impl_->model, &impl_->context, mjFONTSCALE_150);
        impl_->renderContextReady = true;

        resetView();
        impl_->message = QStringLiteral("MuJoCo model loaded.");
        setAxisPositions(impl_->latestAxisPositions);
    } catch (const std::exception &e) {
        impl_->message = QString::fromStdString(e.what());
        impl_->release();
    }

    doneCurrent();
    update();
#else
    impl_->message = QStringLiteral("MuJoCo support is not compiled.");
    update();
#endif
}

void MujocoVisualizer3D::setAxisPositions(const QVector<double> &positions)
{
    impl_->latestAxisPositions = positions;

#ifdef ZRCS_HAS_MUJOCO
    if (impl_->model && impl_->data) {
        for (const auto &binding : impl_->bindings) {
            if (binding.axisId < 0 || binding.axisId >= positions.size()) {
                continue;
            }
            impl_->data->qpos[binding.qposAdr] =
                positions[binding.axisId] * binding.qposScale + binding.qposOffset;
        }
        mj_forward(impl_->model, impl_->data);
        impl_->appendToolTipSample();
    }
#endif

    update();
}

void MujocoVisualizer3D::clearTrajectory()
{
#ifdef ZRCS_HAS_MUJOCO
    impl_->toolTrail.clear();
    if (impl_->toolTrailVisible && impl_->model && impl_->data) {
        impl_->appendToolTipSample();
    }
#endif
    update();
}

void MujocoVisualizer3D::setToolTrailVisible(bool visible)
{
#ifdef ZRCS_HAS_MUJOCO
    impl_->toolTrailVisible = visible;
    if (visible && impl_->model && impl_->data) {
        // 重新打开时打一个当前刀尖点，避免轨迹从空跳起
        impl_->appendToolTipSample();
    }
#else
    Q_UNUSED(visible);
#endif
    update();
}

bool MujocoVisualizer3D::isToolTrailVisible() const
{
#ifdef ZRCS_HAS_MUJOCO
    return impl_->toolTrailVisible;
#else
    return false;
#endif
}

void MujocoVisualizer3D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef ZRCS_HAS_MUJOCO
    if (impl_->ready()) {
        const double ratio = devicePixelRatioF();
        mjrRect viewport{0,
                         0,
                         static_cast<int>(width() * ratio),
                         static_cast<int>(height() * ratio)};
        mjv_updateScene(impl_->model,
                        impl_->data,
                        &impl_->option,
                        nullptr,
                        &impl_->camera,
                        mjCAT_ALL,
                        &impl_->scene);
        impl_->addTrailToScene();
        mjr_render(viewport, &impl_->scene, &impl_->context);

        const std::string left = "MuJoCo";
        const std::string right =
            "axes " + std::to_string(impl_->latestAxisPositions.size()) +
            " / joints " + std::to_string(impl_->bindings.size()) +
            " / trail " + std::to_string(impl_->toolTrail.size());
        mjr_overlay(mjFONT_NORMAL,
                    mjGRID_TOPLEFT,
                    viewport,
                    left.c_str(),
                    right.c_str(),
                    &impl_->context);
        return;
    }
#endif

    QPainter painter(this);
    painter.fillRect(rect(), QColor("#101722"));
    painter.setPen(QColor("#d6deeb"));
    painter.drawText(rect().adjusted(24, 24, -24, -24),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     impl_->message);
}

void MujocoVisualizer3D::resetView()
{
#ifdef ZRCS_HAS_MUJOCO
    if (!impl_->model) {
        return;
    }
    mjv_defaultCamera(&impl_->camera);
    impl_->camera.type = mjCAMERA_FREE;
    impl_->camera.azimuth = 135.0;
    impl_->camera.elevation = -25.0;
    impl_->camera.distance = std::max(1.0, impl_->model->stat.extent * 2.2);
    impl_->camera.lookat[0] = impl_->model->stat.center[0];
    impl_->camera.lookat[1] = impl_->model->stat.center[1];
    impl_->camera.lookat[2] = impl_->model->stat.center[2];
#endif
    update();
}

void MujocoVisualizer3D::setTopView()
{
#ifdef ZRCS_HAS_MUJOCO
    if (!impl_->model) {
        return;
    }
    impl_->camera.type = mjCAMERA_FREE;
    impl_->camera.azimuth = 90.0;
    impl_->camera.elevation = -89.0;
    impl_->camera.distance = std::max(1.0, impl_->model->stat.extent * 2.4);
    impl_->camera.lookat[0] = impl_->model->stat.center[0];
    impl_->camera.lookat[1] = impl_->model->stat.center[1];
    impl_->camera.lookat[2] = impl_->model->stat.center[2];
#endif
    update();
}

void MujocoVisualizer3D::wheelEvent(QWheelEvent *event)
{
#ifdef ZRCS_HAS_MUJOCO
    if (impl_->model && impl_->sceneReady) {
        const double steps = static_cast<double>(event->angleDelta().y()) / 120.0;
        mjv_moveCamera(impl_->model,
                       mjMOUSE_ZOOM,
                       0.0,
                       -0.05 * steps,
                       &impl_->scene,
                       &impl_->camera);
        update();
    }
#endif
    event->accept();
}

void MujocoVisualizer3D::mousePressEvent(QMouseEvent *event)
{
    activeButton_ = event->button();
    lastMousePos_ = event->pos();
    event->accept();
}

void MujocoVisualizer3D::mouseMoveEvent(QMouseEvent *event)
{
#ifdef ZRCS_HAS_MUJOCO
    if (impl_->model && impl_->sceneReady && activeButton_ != Qt::NoButton) {
        const QPoint delta = event->pos() - lastMousePos_;
        const double h = std::max(1, height());
        const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);

        mjtMouse action = mjMOUSE_ZOOM;
        if (activeButton_ == Qt::LeftButton) {
            action = shift ? mjMOUSE_ROTATE_H : mjMOUSE_ROTATE_V;
        } else if (activeButton_ == Qt::RightButton) {
            action = shift ? mjMOUSE_MOVE_H : mjMOUSE_MOVE_V;
        } else if (activeButton_ == Qt::MiddleButton) {
            action = mjMOUSE_ZOOM;
        }

        mjv_moveCamera(impl_->model,
                       action,
                       static_cast<double>(delta.x()) / h,
                       static_cast<double>(delta.y()) / h,
                       &impl_->scene,
                       &impl_->camera);
        update();
    }
#endif
    lastMousePos_ = event->pos();
    event->accept();
}

void MujocoVisualizer3D::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == activeButton_) {
        activeButton_ = Qt::NoButton;
    }
    event->accept();
}

void MujocoVisualizer3D::clearState()
{
    impl_->latestAxisPositions.clear();
    reloadModel();
}

MujocoPanel::MujocoPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

MujocoPanel::~MujocoPanel() = default;

void MujocoPanel::setupUI()
{
    Ui::MujocoPanelUi ui;
    ui.setupUi(this);
    mujocoView = findChild<MujocoVisualizer3D *>("mujocoView");

    QPushButton *btn2D = findChild<QPushButton *>("btn2D");
    QPushButton *btn3D = findChild<QPushButton *>("btn3D");
    QPushButton *btnResetView = findChild<QPushButton *>("btnResetView");
    QPushButton *btnReload = findChild<QPushButton *>("btnReload");
    QPushButton *btnShowTrail = findChild<QPushButton *>("btnShowTrail");
    QPushButton *btnClearTrail = findChild<QPushButton *>("btnClearTrail");
    if (!mujocoView || !btn2D || !btn3D || !btnResetView || !btnReload || !btnShowTrail ||
        !btnClearTrail) {
        return;
    }

    btn2D->setText(QStringLiteral("Top"));
    btn3D->setText(QStringLiteral("Free"));
    btnResetView->setText(QStringLiteral("Reset"));
    btnReload->setText(QStringLiteral("Reload"));
    btnShowTrail->setText(QStringLiteral("Tool Path"));
    btnShowTrail->setCheckable(true);
    btnShowTrail->setChecked(mujocoView->isToolTrailVisible());
    btnShowTrail->setToolTip(QStringLiteral("显示/隐藏刀末端轨迹"));
    btnClearTrail->setText(QStringLiteral("Clear Path"));

    btn2D->setProperty("kind", "accent");
    btn3D->setProperty("kind", "accentBlue");
    btnResetView->setProperty("kind", "neutral");
    btnReload->setProperty("kind", "neutral");
    btnShowTrail->setProperty("kind", "accent");
    btnClearTrail->setProperty("kind", "neutral");

    connect(btn2D, &QPushButton::clicked, mujocoView, &MujocoVisualizer3D::setTopView);
    connect(btn3D, &QPushButton::clicked, mujocoView, &MujocoVisualizer3D::resetView);
    connect(btnResetView, &QPushButton::clicked, mujocoView, &MujocoVisualizer3D::resetView);
    connect(btnReload, &QPushButton::clicked, this, &MujocoPanel::reloadModel);
    connect(btnShowTrail, &QPushButton::toggled, this, &MujocoPanel::setToolTrailVisible);
    connect(btnClearTrail, &QPushButton::clicked, this, &MujocoPanel::clearTrajectory);
}

void MujocoPanel::clearState()
{
    if (mujocoView) {
        mujocoView->clearState();
    }
}

void MujocoPanel::setToolTrailVisible(bool visible)
{
    if (mujocoView) {
        mujocoView->setToolTrailVisible(visible);
    }
}

void MujocoPanel::updateAxisPositions(const QVector<double> &positions)
{
    if (mujocoView) {
        mujocoView->setAxisPositions(positions);
    }
}

void MujocoPanel::reloadModel()
{
    if (mujocoView) {
        mujocoView->reloadModel();
    }
}

void MujocoPanel::clearTrajectory()
{
    if (mujocoView) {
        mujocoView->clearTrajectory();
    }
}
