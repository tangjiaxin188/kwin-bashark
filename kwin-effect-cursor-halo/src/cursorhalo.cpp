/*
    SPDX-FileCopyrightText: 2025

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cursorhalo.h"
#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glutils.h"
#include <QPainter>
#include <cmath>

namespace KWin
{

CursorHaloEffect::CursorHaloEffect()
{
    connect(effects, &EffectsHandler::mouseChanged, this, &CursorHaloEffect::slotMouseChanged);
    m_cursorPos = effects->cursorPos();
}

CursorHaloEffect::~CursorHaloEffect()
{
}

void CursorHaloEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &region, LogicalOutput *screen)
{
    effects->paintScreen(renderTarget, viewport, mask, region, screen);

    if (effects->isOpenGLCompositing()) {
        paintScreenSetupGl(renderTarget, viewport.projectionMatrix());
    }

    drawHalo(viewport);

    if (effects->isOpenGLCompositing()) {
        paintScreenFinishGl();
    }
}

void CursorHaloEffect::drawHalo(const RenderViewport &viewport)
{
    const float scale = viewport.scale();
    const float cx = m_cursorPos.x();
    const float cy = m_cursorPos.y();
    const float r = m_haloRadius;

    static const int num_segments = 80;
    static const float theta = 2.0f * 3.1415926f / static_cast<float>(num_segments);
    static const float c = std::cos(theta);
    static const float s = std::sin(theta);

    float x = r;
    float y = 0;
    float t;

    GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
    vbo->reset();
    QList<QVector2D> verts;
    verts.reserve(num_segments);

    for (int i = 0; i < num_segments; ++i) {
        verts.push_back(QVector2D((x + cx) * scale, (y + cy) * scale));
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }

    if (effects->isOpenGLCompositing()) {
        vbo->setVertices(verts);
        ShaderManager::instance()->getBoundShader()->setUniform(GLShader::ColorUniform::Color, m_haloColor);
        vbo->render(GL_LINE_LOOP);
    } else if (effects->compositingType() == QPainterCompositing) {
        QPainter *painter = effects->scenePainter();
        painter->save();
        QPen pen(m_haloColor);
        pen.setWidth(m_haloWidth);
        painter->setPen(pen);
        painter->drawEllipse(QPointF(cx, cy), r, r);
        painter->restore();
    }
}

void CursorHaloEffect::paintScreenSetupGl(const RenderTarget &renderTarget, const QMatrix4x4 &projectionMatrix)
{
    GLShader *shader = ShaderManager::instance()->pushShader(ShaderTrait::UniformColor | ShaderTrait::TransformColorspace);
    shader->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, projectionMatrix);
    shader->setColorspaceUniforms(ColorDescription::sRGB, renderTarget.colorDescription(), RenderingIntent::Perceptual);

    glLineWidth(m_haloWidth);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void CursorHaloEffect::paintScreenFinishGl()
{
    glDisable(GL_BLEND);
    ShaderManager::instance()->popShader();
}

bool CursorHaloEffect::isActive() const
{
    return true;
}

void CursorHaloEffect::slotMouseChanged(const QPointF &pos, const QPointF &old)
{
    m_cursorPos = pos;
    if (pos != old) {
        effects->addRepaintFull();
    }
}

} // namespace KWin

#include "moc_cursorhalo.cpp"
