/*
    SPDX-FileCopyrightText: 2025

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "core/colorspace.h"
#include "effect/effect.h"
#include "opengl/glutils.h"

namespace KWin
{

class LogicalOutput;

class CursorHaloEffect : public Effect
{
    Q_OBJECT
    Q_PROPERTY(QColor haloColor READ haloColor)
    Q_PROPERTY(qreal haloRadius READ haloRadius)
    Q_PROPERTY(qreal haloWidth READ haloWidth)

public:
    CursorHaloEffect();
    ~CursorHaloEffect() override;

    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &region, LogicalOutput *screen) override;
    bool isActive() const override;

    QColor haloColor() const { return m_haloColor; }
    qreal haloRadius() const { return m_haloRadius; }
    qreal haloWidth() const { return m_haloWidth; }

private Q_SLOTS:
    void slotMouseChanged(const QPointF &pos, const QPointF &old);

private:
    void drawHalo(const RenderViewport &viewport);
    void paintScreenSetupGl(const RenderTarget &renderTarget, const QMatrix4x4 &projectionMatrix);
    void paintScreenFinishGl();

    QColor m_haloColor = QColor(0, 180, 255, 180);
    qreal m_haloRadius = 35.0;
    qreal m_haloWidth = 3.0;
    QPointF m_cursorPos;
};

} // namespace KWin
