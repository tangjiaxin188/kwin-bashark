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
    bool    active = false;
    QPointF pos;
    QColor  color;
    float   radius = 0;
    float   maxRadius = 40;
    float   ringAngle = 0;
    float   ringLife = 0;
    float   ringMaxLife = 120;
    float   ringSpeed = 0.03f;
    float   life = 0;
    float   maxLife = 25;
    struct Seg { float off, len; };
    Seg segs[3];
};

struct Particle
{
    bool    active = false;
    QPointF pos;
    QColor  color;
    QPointF velocity;
    float   rotation = 0;
    float   rotationSpeed = 0;
    float   size = 0;
    float   alpha = 1.0f;
    float   life = 0;      // ✅ 新增：当前存活帧数
    float   maxLife = 35;  // ✅ 新增：最大存活帧数
    float   decay = 0.95f;
    bool    isTriangle = false;
};

struct TrailPoint
{
    QPointF pos;
    qint64  timestamp;
    QColor  color;
};

class RenderHelper
{
public:
    RenderHelper();

    void addTrailPoint(const QPointF &pos, qint64 timestampMs, const QColor &color);
    void addWave(const QPointF &pos, const QColor &color);
    void addExplosionParticles(const QPointF &pos, const QColor &color, int count = 4, qreal spawnOffset = 0);
    void addTrailParticle(const QPointF &pos, const QColor &color);

    void update(qreal frameScale);
    void renderGl(const RenderViewport &viewport) const;

    bool hasContent() const;
    void clear();

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
    TrailPoint  m_trail[16];
    int         m_trailCount = 0;
    WaveData    m_waves[64];
    int         m_waveCount = 0;
    Particle    m_particles[512];
    int         m_particleCount = 0;
};

} // namespace KWin