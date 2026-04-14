/*
    SPDX-FileCopyrightText: 2025

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "effect/effect.h"
#include "renderhelper.h"

namespace KWin
{

class LogicalOutput;

class CursorHaloEffect : public Effect
{
    Q_OBJECT

public:
    CursorHaloEffect();
    ~CursorHaloEffect() override;

    void reconfigure(ReconfigureFlags flags) override;
    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask,
                     const Region &deviceRegion, LogicalOutput *output) override;
    void postPaintScreen() override;
    bool isActive() const override;

private Q_SLOTS:
    void slotMouseChanged(const QPointF &pos, const QPointF &old,
                          Qt::MouseButtons buttons, Qt::MouseButtons oldButtons,
                          Qt::KeyboardModifiers modifiers, Qt::KeyboardModifiers oldModifiers);

private:
    void setupGl(const RenderTarget &renderTarget, const QMatrix4x4 &projectionMatrix);
    void finishGl();

    RenderHelper m_renderer;

    // 颜色配置（对应 QML 属性）
    QColor m_sparkColor = QColor(45, 175, 255);     // #2DAFFF 左键蓝
    QColor m_midColor = QColor(46, 204, 113);       // #2ECC71 中键绿
    QColor m_rightColor = QColor(231, 76, 60);      // #E74C3C 右键红

    QColor m_activeColor;
    bool m_mouseDown = false;

    std::chrono::milliseconds m_lastPresentTime;
    bool m_needRepaint = false;
};

} // namespace KWin
