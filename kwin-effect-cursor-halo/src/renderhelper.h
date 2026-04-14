/*
    SPDX-FileCopyrightText: 2025
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QColor>
#include <QList>
#include <QPointF>
#include <QtGlobal>

namespace KWin
{

class RenderViewport;

// ──────────────────────────── 对象池结构体 ─────────────────────────────

struct WaveData
{
    bool    active = false;    // ✅ 对象池标记
    QPointF pos;
    QColor  color;
    float   radius = 0;
    float   maxRadius = 40;
    float   ringAngle = 0;
    float   ringLife = 0;
    float   ringMaxLife = 120;
    float   ringSpeed = 0.03f;
    float   life = 0;          // ✅ 改为 float 解决帧率波动导致的计时器截断问题
    float   maxLife = 25;      // ✅ 改为 float
    struct Seg { float off, len; };
    Seg segs[3];
};

struct Particle
{
    bool    active = false;    // ✅ 对象池标记
    QPointF pos;
    QColor  color;
    QPointF velocity;
    float   rotation = 0;
    float   rotationSpeed = 0;
    float   size = 0;
    float   alpha = 1.0f;
    float   decay = 0.95f;
    bool    isTriangle = false;
};

struct TrailPoint
{
    QPointF pos;
    qint64  timestamp; // ms
    QColor  color;     // ✅ 增加颜色存储
};

// ──────────────────────────── 渲染助手 ────────────────────────────

class RenderHelper
{
public:
    RenderHelper();

    // ── 状态输入 ──
    // ✅ 增加颜色参数以支持彩色轨迹
    void addTrailPoint(const QPointF &pos, qint64 timestampMs, const QColor &color);
    void addWave(const QPointF &pos, const QColor &color);
    void addExplosionParticles(const QPointF &pos, const QColor &color, int count = 4, qreal spawnOffset = 0);
    void addTrailParticle(const QPointF &pos, const QColor &color);

    // ── 状态更新 ──
    void update(qreal frameScale);

    // ── GL 绘制 ──
    void renderGl(const RenderViewport &viewport) const;

    bool hasContent() const;
    void clear();

    // ── 配置参数 ──
    int    maxTrail = 16;
    qreal  particleScale = 1.5;
    qreal  waveMaxLife = 25;
    qreal  ringMaxLife = 50;
    qreal  waveSpreadSpeed = 0.5;
    qreal  ringRotateSpeed = 0.03;
    qreal  particleSpeedScale = 0.5;
    qreal  waveFadeCurve = 1.2;
    int    trailLineWidth = 6;

private:
    // ── 对象池 ──
    TrailPoint  m_trail[16];
    int         m_trailCount = 0;

    WaveData    m_waves[64];      // 最多 64 个波纹
    int         m_waveCount = 0;

    Particle    m_particles[512]; // 最多 512 个粒子
    int         m_particleCount = 0;
};

} // namespace KWin