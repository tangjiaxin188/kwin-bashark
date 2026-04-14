/*
    SPDX-FileCopyrightText: 2025
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "renderhelper.h"
#include "core/renderviewport.h"
#include "opengl/glutils.h"
#include <QVector2D>
#include <QDateTime>
#include <cmath>

namespace KWin
{

// 圆周率常量 (替代 M_PI)
static constexpr float PI = 3.1415926535f;

RenderHelper::RenderHelper()
{
}

// ─────────────────────────── 状态输入 ─────────────────────────────

void RenderHelper::addTrailPoint(const QPointF &pos, qint64 timestampMs)
{
    if (m_trailCount < 16) {
        m_trail[m_trailCount++] = {pos, timestampMs};
    } else {
        // 满了就把最老的挤掉（前移）
        for (int i = 0; i < 15; ++i) m_trail[i] = m_trail[i+1];
        m_trail[15] = {pos, timestampMs};
    }
}

void RenderHelper::addWave(const QPointF &pos, const QColor &color)
{
    for (int i = 0; i < 64; ++i) {
        if (!m_waves[i].active) {
            WaveData &w = m_waves[i];
            w.active = true;
            w.pos = pos;
            w.color = color;
            w.radius = 0;
            w.life = 0;
            w.ringLife = 0;
            
            // 使用实例配置
            w.maxRadius = 40.0f * static_cast<float>(particleScale);
            w.maxLife = static_cast<float>(waveMaxLife);
            w.ringMaxLife = static_cast<float>(ringMaxLife);
            w.ringAngle = static_cast<float>(std::rand()) / RAND_MAX * 6.2832f;
            w.ringSpeed = static_cast<float>(ringRotateSpeed);
            
            // 显式初始化 segs
            w.segs[0] = {-0.25f * PI, 1.15f * PI};
            w.segs[1] = { 0.00f * PI, 1.15f * PI};
            w.segs[2] = { 0.25f * PI, 1.15f * PI};
            
            return; // 找到空闲位直接返回
        }
    }
}

void RenderHelper::addExplosionParticles(const QPointF &pos, const QColor &color, int count, qreal spawnOffset)
{
    int spawned = 0;
    const float sOffset = static_cast<float>(spawnOffset);
    for (int i = 0; i < 512 && spawned < count; ++i) {
        if (!m_particles[i].active) {
            Particle &p = m_particles[i];
            p.active = true;
            p.color = color;

            // 50%~70% 半径处生成 (内侧)
            float spawnDist = sOffset * (0.5f + static_cast<float>(std::rand()) / RAND_MAX * 0.2f);
            float angle = static_cast<float>(std::rand()) / RAND_MAX * 6.2832f;
            p.pos = QPointF(pos.x() + std::cos(angle) * spawnDist, pos.y() + std::sin(angle) * spawnDist);

            // 随机小速度
            float vAngle = static_cast<float>(std::rand()) / RAND_MAX * 6.2832f;
            float speed = 0.5f + static_cast<float>(std::rand()) / RAND_MAX * 0.5f;
            p.velocity = QPointF(std::cos(vAngle) * speed, std::sin(vAngle) * speed);

            p.size = (3.0f + static_cast<float>(std::rand()) / RAND_MAX * 4.0f) * static_cast<float>(particleScale);
            p.rotation = static_cast<float>(std::rand()) / RAND_MAX * 6.2832f;
            p.rotationSpeed = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 0.15f;
            p.alpha = 1.0f;
            p.decay = 0.95f;
            p.isTriangle = true;
            spawned++;
        }
    }
}

void RenderHelper::addTrailParticle(const QPointF &pos, const QColor &color)
{
    for (int i = 0; i < 512; ++i) {
        if (!m_particles[i].active) {
            Particle &p = m_particles[i];
            p.active = true;
            p.pos = pos;
            p.color = color;
            const float angle = static_cast<float>(std::rand()) / RAND_MAX * 6.2832f;
            const float speed = 1.3f * static_cast<float>(particleSpeedScale) * static_cast<float>(particleScale);
            p.velocity = QPointF(std::cos(angle) * speed, std::sin(angle) * speed);
            p.rotation = static_cast<float>(std::rand()) / RAND_MAX * 6.2832f;
            p.rotationSpeed = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 0.15f;
            p.size = 9.0f * static_cast<float>(particleScale);
            p.alpha = 1.0f;
            p.decay = 0.97f;
            p.isTriangle = false;
            return;
        }
    }
}

// ─────────────────────────── 状态更新 ─────────────────────────────

void RenderHelper::update(qreal frameScale)
{
    const qint64 trailNow = QDateTime::currentMSecsSinceEpoch();
    // 轨迹清理
    while (m_trailCount > 0 && (trailNow - m_trail[0].timestamp) > 300) {
        for(int i=0; i<m_trailCount-1; ++i) m_trail[i] = m_trail[i+1];
        m_trailCount--;
    }

    // 更新波纹
    for (int i = 0; i < 64; ++i) {
        WaveData &w = m_waves[i];
        if (w.active) {
            w.life += frameScale;
            w.ringLife += frameScale;

            const float progress = w.life / w.maxLife;
            const float spreadProgress = std::min(progress * static_cast<float>(waveSpreadSpeed), 1.0f);
            w.radius = w.maxRadius * (1.0f - std::pow(1.0f - spreadProgress, 2.5f));

            if (w.life >= w.maxLife) {
                w.active = false; // 回收对象
            }
        }
    }

    // 更新粒子
    for (int i = 0; i < 512; ++i) {
        Particle &p = m_particles[i];
        if (p.active) {
            p.pos += p.velocity * frameScale;
            p.velocity *= std::pow(p.decay, frameScale);
            p.rotation += p.rotationSpeed * static_cast<float>(frameScale);
            p.alpha -= 0.01f * static_cast<float>(frameScale);
            if (p.alpha <= 0) {
                p.active = false; // 回收对象
            }
        }
    }
}

bool RenderHelper::hasContent() const
{
    if (m_trailCount > 1) return true;
    for(int i=0; i<64; ++i) if(m_waves[i].active) return true;
    for(int i=0; i<512; ++i) if(m_particles[i].active) return true;
    return false;
}

void RenderHelper::clear()
{
    m_trailCount = 0;
    for(int i=0; i<64; ++i) m_waves[i].active = false;
    for(int i=0; i<512; ++i) m_particles[i].active = false;
}

// ─────────────────────────── GL 绘制 ─────────────────────────────

void RenderHelper::renderGl(const RenderViewport &viewport) const
{
    const float scale = viewport.scale();
    GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();

    // ── 1️⃣ 拖尾轨迹 ──
    if (m_trailCount > 1) {
        glLineWidth(static_cast<float>(trailLineWidth));
        for (int i = 1; i < m_trailCount; ++i) {
            const float alpha = static_cast<float>(i) / (m_trailCount - 1);
            // 修复 QColor 参数类型
            ShaderManager::instance()->getBoundShader()->setUniform(
                GLShader::ColorUniform::Color, QColor(255, 255, 255, static_cast<int>(alpha * 255)));
            
            QList<QVector2D> verts;
            verts.reserve(2);
            verts.append(QVector2D(m_trail[i-1].pos.x() * scale, m_trail[i-1].pos.y() * scale));
            verts.append(QVector2D(m_trail[i].pos.x() * scale, m_trail[i].pos.y() * scale));
            vbo->reset();
            vbo->setVertices(verts);
            vbo->render(GL_LINES);
        }
    }

    // ── 2️⃣ 波纹填充圆 ──
    static const int CIRCLE_SEG = 64;
    static const float theta = 6.2832f / CIRCLE_SEG;
    static const float cc = std::cos(theta);
    static const float ss = std::sin(theta);

    for (int i = 0; i < 64; ++i) {
        const WaveData &w = m_waves[i];
        if (!w.active) continue;

        const float progress = w.life / w.maxLife;
        const float fadeProgress = std::min(progress, 1.0f);
        const float alpha = std::pow(1.0f - fadeProgress, static_cast<float>(waveFadeCurve));

        if (alpha < 0.01f) continue;

        const float r = w.radius * scale;
        const float cx = w.pos.x() * scale;
        const float cy = w.pos.y() * scale;
        float x = r, y = 0, tmp;

        QList<QVector2D> verts;
        verts.reserve(CIRCLE_SEG + 2);
        verts.append(QVector2D(cx, cy)); // 中心点
        for (int k = 0; k < CIRCLE_SEG; ++k) {
            verts.append(QVector2D(x + cx, y + cy));
            tmp = x; x = cc * x - ss * y; y = ss * tmp + cc * y;
        }

        vbo->reset();
        vbo->setVertices(verts);
        ShaderManager::instance()->getBoundShader()->setUniform(
            GLShader::ColorUniform::Color, w.color);
        vbo->render(GL_TRIANGLE_FAN);
    }

    // ── 3️⃣ 装饰弧线 ──
    glLineWidth(3.7f);
    for (int i = 0; i < 64; ++i) {
        const WaveData &w = m_waves[i];
        if (!w.active) continue;

        const float ringProgress = w.ringLife / w.ringMaxLife;
        const float ringAlpha = std::pow(1.0f - std::min(ringProgress, 1.0f), static_cast<float>(waveFadeCurve));

        if (ringAlpha < 0.01f) continue;

        const float shrink = std::max(0.0f, 1.0f - ringProgress);
        const float r = (w.radius + 3.0f) * scale;
        const float cx = w.pos.x() * scale;
        const float cy = w.pos.y() * scale;

        for (int si = 0; si < 3; ++si) {
            const float len = w.segs[si].len * shrink;
            const float start = w.ringAngle + w.segs[si].off;
            const int steps = static_cast<int>(len / 0.08f) + 2;

            QList<QVector2D> verts;
            verts.reserve(steps);
            for (int k = 0; k < steps; ++k) {
                const float a = start + len * static_cast<float>(k) / (steps - 1);
                verts.append(QVector2D(cx + std::cos(a) * r, cy + std::sin(a) * r));
            }

            vbo->reset();
            vbo->setVertices(verts);
            ShaderManager::instance()->getBoundShader()->setUniform(
                GLShader::ColorUniform::Color, QColor(245, 248, 252));
            vbo->render(GL_LINE_STRIP);
        }
    }

    // ── 4️⃣ 粒子 ──
    glLineWidth(1.5f);
    for (int i = 0; i < 512; ++i) {
        const Particle &p = m_particles[i];
        if (!p.active) continue;

        const float cx = p.pos.x() * scale;
        const float cy = p.pos.y() * scale;
        const float cosR = std::cos(p.rotation);
        const float sinR = std::sin(p.rotation);
        const float sz = p.size * scale;

        // 内联旋转计算替代 Lambda
        auto rotX = [&](float px, float py) { return cx + (px * cosR - py * sinR); };
        auto rotY = [&](float px, float py) { return cy + (px * sinR + py * cosR); };

        vbo->reset();
        if (p.isTriangle) {
            QList<QVector2D> verts;
            verts.append(QVector2D(rotX(0, -sz), rotY(0, -sz)));
            verts.append(QVector2D(rotX(sz * 0.6f, sz * 0.6f), rotY(sz * 0.6f, sz * 0.6f)));
            verts.append(QVector2D(rotX(-sz * 0.6f, sz * 0.6f), rotY(-sz * 0.6f, sz * 0.6f)));
            vbo->setVertices(verts);
            ShaderManager::instance()->getBoundShader()->setUniform(
                GLShader::ColorUniform::Color, Qt::white);
            vbo->render(GL_TRIANGLES);
        } else {
            static const int P_SEG = 16;
            static const float p_t = 6.2832f / P_SEG;
            static const float p_cc = std::cos(p_t);
            static const float p_ss = std::sin(p_t);
            const float r = sz * 0.3f;
            float x = r, y = 0, tmp;

            QList<QVector2D> verts;
            verts.reserve(P_SEG);
            for (int k = 0; k < P_SEG; ++k) {
                verts.append(QVector2D(rotX(x, y), rotY(x, y)));
                tmp = x; x = p_cc * x - p_ss * y; y = p_ss * tmp + p_cc * y;
            }
            vbo->setVertices(verts);
            ShaderManager::instance()->getBoundShader()->setUniform(
                GLShader::ColorUniform::Color, p.color);
            vbo->render(GL_LINE_LOOP);
        }
    }
}

} // namespace KWin