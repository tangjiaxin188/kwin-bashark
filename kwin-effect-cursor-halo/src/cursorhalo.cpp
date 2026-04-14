/*
    SPDX-FileCopyrightText: 2025

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cursorhalo.h"
#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glutils.h"

namespace KWin
{

CursorHaloEffect::CursorHaloEffect()
{
    connect(effects, &EffectsHandler::mouseChanged, this, &CursorHaloEffect::slotMouseChanged);
    m_activeColor = m_sparkColor;
    m_lastPresentTime = std::chrono::milliseconds::zero();

    // 同步配置到 renderhelper
    m_renderer.particleScale = 1.5;
    m_renderer.waveMaxLife = 25;
    m_renderer.ringMaxLife = 50;
    m_renderer.waveSpreadSpeed = 0.5;
    m_renderer.ringRotateSpeed = 0.02;
    m_renderer.particleSpeedScale = 0.5;
    m_renderer.waveFadeCurve = 1.2;
    m_renderer.maxTrail = 16;
    m_renderer.trailLineWidth = 6;
}

CursorHaloEffect::~CursorHaloEffect()
{
}

void CursorHaloEffect::reconfigure(ReconfigureFlags)
{
}

void CursorHaloEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    int elapsed = 0;
    if (m_lastPresentTime.count() > 0)
        elapsed = (presentTime - m_lastPresentTime).count();
    m_lastPresentTime = presentTime;

    const qreal frameScale = qMax(0, elapsed) / 16.0;
    m_renderer.update(frameScale);

    if (m_renderer.hasContent()) {
        m_needRepaint = true;
        m_lastPresentTime = presentTime;
    } else {
        m_needRepaint = false;
        m_lastPresentTime = std::chrono::milliseconds::zero();
    }

    effects->prePaintScreen(data, presentTime);
}

void CursorHaloEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask,
                                   const Region &deviceRegion, LogicalOutput *output)
{
    effects->paintScreen(renderTarget, viewport, mask, deviceRegion, output);

    if (!m_renderer.hasContent())
        return;

    if (effects->isOpenGLCompositing()) {
        setupGl(renderTarget, viewport.projectionMatrix());
        m_renderer.renderGl(viewport);
        finishGl();
    }
}

void CursorHaloEffect::postPaintScreen()
{
    effects->postPaintScreen();
    if (m_needRepaint)
        effects->addRepaintFull();
}

void CursorHaloEffect::setupGl(const RenderTarget &renderTarget, const QMatrix4x4 &projectionMatrix)
{
    GLShader *shader = ShaderManager::instance()->pushShader(
        ShaderTrait::UniformColor | ShaderTrait::TransformColorspace);
    shader->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, projectionMatrix);
    shader->setColorspaceUniforms(ColorDescription::sRGB, renderTarget.colorDescription(), RenderingIntent::Perceptual);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void CursorHaloEffect::finishGl()
{
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    ShaderManager::instance()->popShader();
}

bool CursorHaloEffect::isActive() const
{
    return m_renderer.hasContent();
}

// ─────────────────────────── 鼠标事件（1:1 对应 QML MouseArea）────────────────────────────

void CursorHaloEffect::slotMouseChanged(const QPointF &pos, const QPointF &old,
                                        Qt::MouseButtons buttons, Qt::MouseButtons oldButtons,
                                        Qt::KeyboardModifiers, Qt::KeyboardModifiers)
{
    const bool isLeft = buttons & Qt::LeftButton;
    const bool isMid = buttons & Qt::MiddleButton;
    const bool isRight = buttons & Qt::RightButton;
    const bool wasLeft = oldButtons & Qt::LeftButton;
    const bool wasMid = oldButtons & Qt::MiddleButton;
    const bool wasRight = oldButtons & Qt::RightButton;

    // ── 移动拖尾（对应 QML handleMouseMove）──
    const qreal dist = std::hypot(pos.x() - old.x(), pos.y() - old.y());
    if (dist > 2) {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        m_renderer.addTrailPoint(pos, now);

        // 拖尾粒子（对应 QML: Math.random() < 0.3 && mouseArea.pressed）
        if ((isLeft || isMid || isRight) && (std::rand() % 100 < 30))
            m_renderer.addTrailParticle(pos, m_activeColor);
    }

    // ── 点击检测（对应 QML onPressed）──
    auto wasJustPressed = [&](bool now, bool was) { return now && !was; };

    if (wasJustPressed(isLeft, wasLeft)) {
        m_activeColor = m_sparkColor;
        m_mouseDown = true;
        m_renderer.addWave(pos, m_sparkColor);
        m_renderer.addExplosionParticles(pos, m_sparkColor, 4 + std::rand() % 4, 40 * m_renderer.particleScale);
    }
    if (wasJustPressed(isMid, wasMid)) {
        m_activeColor = m_midColor;
        m_mouseDown = true;
        m_renderer.addWave(pos, m_midColor);
        m_renderer.addExplosionParticles(pos, m_midColor, 4 + std::rand() % 4, 40 * m_renderer.particleScale);
    }
    if (wasJustPressed(isRight, wasRight)) {
        m_activeColor = m_rightColor;
        m_mouseDown = true;
        m_renderer.addWave(pos, m_rightColor);
        m_renderer.addExplosionParticles(pos, m_rightColor, 4 + std::rand() % 4, 40 * m_renderer.particleScale);
    }

    // ── 释放恢复（对应 QML onReleased）──
    if (!isLeft && !isMid && !isRight && m_mouseDown) {
        m_mouseDown = false;
        m_activeColor = m_sparkColor;
    }
}

} // namespace KWin

#include "moc_cursorhalo.cpp"
