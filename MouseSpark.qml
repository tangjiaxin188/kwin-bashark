// MouseSpark.qml
// 多按钮颜色版：左键蓝/中键绿/右键红 + 慢速优雅动画
// 完全复刻并优化 https://github.com/DoomVoss/BASpark 的鼠标特效
import QtQuick 2.15

Item {
    id: root
    width: 1920
    height: 1080
    clip: true
    
    // 🎨 基础参数
    property color sparkColor: "#2DAFFF"        // 默认颜色（左键）
    property color middleButtonColor: "#2ECC71" // 🔥 中键颜色：绿色
    property color rightButtonColor: "#E74C3C"  // 🔥 右键颜色：红色
    property real particleScale: 1.5
    property real animationSpeed: 1.0
    property int maxTrail: 16
    
    // ⚙️ 光环动画速度配置
    property int waveMaxLife: 120
    property int ringMaxLife: 180
    property real waveSpreadSpeed: 0.5
    property real ringRotateSpeed: 0.02
    property real particleSpeedScale: 0.5
    property real waveFadeCurve: 1.2
    
    // 🔄 内部状态
    property var trail: []
    property var waves: []
    property var particles: []
    property point lastPos: Qt.point(width/2, height/2)
    property color activeColor: sparkColor      // 🔥 当前活跃颜色（拖尾跟随）
    
    // ⏱️ 帧率控制
    property int lastFrameTime: 0
    property real frameScale: 1.0
    readonly property int baseFrameMs: 16
    readonly property int maxDeltaMs: 100
    
    // 🖱️ 测试用鼠标区域（🔥 修改：接受所有按钮）
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton  // 🔥 关键：支持三键
        hoverEnabled: true
        
        onPositionChanged: (mouse) => handleMouseMove(mouse.x, mouse.y)
        onPressed: (mouse) => {
            // 🔥 关键：根据按键类型选择颜色
            let clickColor = sparkColor  // 默认左键蓝
            if (mouse.button === Qt.MiddleButton) {
                clickColor = middleButtonColor  // 中键绿
            } else if (mouse.button === Qt.RightButton) {
                clickColor = rightButtonColor   // 右键红
            }
            activeColor = clickColor  // 🔥 更新活跃颜色，拖尾跟随
            handleMouseClick(mouse.x, mouse.y, clickColor)
        }
        onReleased: {
            activeColor = sparkColor  // 🔥 松开后恢复默认颜色
        }
    }
    
    function handleMouseMove(x, y) {
        const dist = Math.hypot(x - lastPos.x, y - lastPos.y)
        if (dist > 2) {
            trail.push({x: x, y: y, t: Date.now()})
            if (trail.length > maxTrail) trail.shift()
            lastPos = Qt.point(x, y)
            // 拖尾粒子使用当前活跃颜色
            if (Math.random() < 0.3 && mouseArea.pressed) {
                spawnParticle(x, y, false, activeColor)
            }
        }
        canvas.requestPaint()
    }
    
    // 🔥 修改：添加 color 参数
    function handleMouseClick(x, y, color) {
        spawnWave(x, y, color)  // 🔥 传递颜色
        for (let i = 0; i < 4; i++) spawnParticle(x, y, true, color)  // 🔥 传递颜色
        canvas.requestPaint()
    }
    
    // 🔥 修改：添加 color 参数，存储到 wave 对象
    function spawnWave(x, y, color) {
        waves.push({
            x: x, y: y,
            color: color,  // 🔥 存储该波纹的颜色
            life: 0, maxLife: waveMaxLife,
            radius: 0, maxRadius: 40 * particleScale,
            ringAngle: Math.random() * Math.PI * 2,
            ringSegments: [
                {offset: -0.25 * Math.PI, length: 1.15 * Math.PI},
                {offset: 0.00 * Math.PI, length: 1.15 * Math.PI},
                {offset: 0.25 * Math.PI, length: 1.15 * Math.PI}
            ],
            ringLife: 0, ringMaxLife: ringMaxLife, 
            ringSpeed: ringRotateSpeed
        })
    }
    
    // 🔥 修改：添加 color 参数，存储到 particle 对象
    function spawnParticle(x, y, isExplosion, color) {
        const angle = Math.random() * Math.PI * 2
        const speedBase = isExplosion ? 
            (2.0 + Math.random() * 1.5) * particleSpeedScale : 
            1.3 * particleSpeedScale
        const speed = speedBase * particleScale
        
        particles.push({
            x: x, y: y,
            color: color,  // 🔥 存储该粒子的颜色
            vx: Math.cos(angle) * speed,
            vy: Math.sin(angle) * speed,
            rotation: Math.random() * Math.PI * 2,
            rotationSpeed: (Math.random() - 0.5) * 0.15,
            size: isExplosion ? 
                (4 + Math.random() * 3) * particleScale : 
                9 * particleScale,
            alpha: 1.0,
            decay: isExplosion ? 0.95 : 0.97,
            isTriangle: isExplosion
        })
    }
    
    function updateAnimations() {
        const now = Date.now()
        const dt = Math.min(now - lastFrameTime, maxDeltaMs)
        frameScale = (dt / baseFrameMs) * animationSpeed
        lastFrameTime = now
        
        const trailNow = Date.now()
        trail = trail.filter(p => (trailNow - p.t) < 300)
        
        // 更新波纹
        for (let i = waves.length - 1; i >= 0; i--) {
            const w = waves[i]
            w.life += frameScale
            w.ringLife += frameScale
            
            const progress = w.life / w.maxLife
            const spreadProgress = Math.min(progress * waveSpreadSpeed, 1)
            const ease = 1 - Math.pow(1 - spreadProgress, 2.5)
            w.radius = w.maxRadius * ease
            
            const fadeProgress = Math.min(progress, 1)
            const alpha = Math.pow(1 - fadeProgress, waveFadeCurve)
            const ringProgress = w.ringLife / w.ringMaxLife
            const ringAlpha = Math.pow(1 - Math.min(ringProgress, 1), waveFadeCurve)
            
            if (progress >= 1 && ringProgress >= 1) {
                waves.splice(i, 1)
            }
        }
        
        // 更新粒子
        for (let i = particles.length - 1; i >= 0; i--) {
            const p = particles[i]
            p.x += p.vx * frameScale
            p.y += p.vy * frameScale
            p.vx *= Math.pow(p.decay, frameScale)
            p.vy *= Math.pow(p.decay, frameScale)
            p.rotation += p.rotationSpeed * frameScale
            p.alpha -= 0.01 * frameScale
            if (p.alpha <= 0) particles.splice(i, 1)
        }
    }
    
    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        
        onPaint: {
            const ctx = getContext("2d")
            if (!ctx) return
            ctx.clearRect(0, 0, width, height)
            ctx.globalCompositeOperation = "lighter"
            
            // 1️⃣ 拖尾轨迹（🔥 使用活跃颜色）
            if (trail.length > 1) {
                const r = Math.round(activeColor.r * 255)
                const g = Math.round(activeColor.g * 255)
                const b = Math.round(activeColor.b * 255)
                
                ctx.beginPath()
                ctx.moveTo(trail[0].x, trail[0].y)
                for (let i = 1; i < trail.length; i++) ctx.lineTo(trail[i].x, trail[i].y)
                const head = trail[trail.length - 1], tail = trail[0]
                const gradient = ctx.createLinearGradient(head.x, head.y, tail.x, tail.y)
                gradient.addColorStop(0, `rgba(${r},${g},${b},0)`)
                gradient.addColorStop(1, `rgba(${r},${g},${b},1)`)
                ctx.strokeStyle = gradient
                ctx.lineWidth = 5
                ctx.shadowColor = `rgba(${r},${g},${b},0.6)`
                ctx.shadowBlur = 3
                ctx.stroke()
                ctx.shadowBlur = 0
            }
            
            // 2️⃣ 点击波纹（🔥 关键：使用各自存储的颜色）
            for (const w of waves) {
                // 🔥 解析该波纹的独立颜色
                const r = Math.round(w.color.r * 255)
                const g = Math.round(w.color.g * 255)
                const b = Math.round(w.color.b * 255)
                
                const progress = w.life / w.maxLife
                const fadeProgress = Math.min(progress, 1)
                const alpha = Math.pow(1 - fadeProgress, waveFadeCurve)
                
                if (alpha > 0.01) {
                    ctx.beginPath()
                    ctx.arc(w.x, w.y, w.radius, 0, Math.PI * 2)
                    ctx.fillStyle = `rgba(${r},${g},${b},${alpha})`
                    ctx.fill()
                }
                
                // 装饰弧线（白色，保持原样）
                const ringProgress = w.ringLife / w.ringMaxLife
                const ringAlpha = Math.pow(1 - Math.min(ringProgress, 1), waveFadeCurve)
                
                if (ringAlpha > 0.01) {
                    const shrink = Math.max(0, 1 - ringProgress)
                    w.ringAngle -= w.ringSpeed * frameScale
                    
                    for (const seg of w.ringSegments) {
                        const len = seg.length * shrink
                        const start = w.ringAngle + seg.offset
                        ctx.beginPath()
                        ctx.arc(w.x, w.y, w.radius + 3, start, start + len)
                        ctx.strokeStyle = `rgba(245,248,252,${ringAlpha})`
                        ctx.lineWidth = 3.7
                        ctx.stroke()
                    }
                }
            }
            
            // 3️⃣ 粒子（🔥 关键：使用各自存储的颜色）
            for (const p of particles) {
                // 🔥 解析该粒子的独立颜色
                const r = Math.round(p.color.r * 255)
                const g = Math.round(p.color.g * 255)
                const b = Math.round(p.color.b * 255)
                
                ctx.save()
                ctx.translate(p.x, p.y)
                ctx.rotate(p.rotation)
                if (p.isTriangle) {
                    // 爆炸粒子：白色三角形（保持原设计）
                    ctx.beginPath()
                    ctx.moveTo(0, -p.size)
                    ctx.lineTo(p.size * 0.6, p.size * 0.6)
                    ctx.lineTo(-p.size * 0.6, p.size * 0.6)
                    ctx.closePath()
                    ctx.fillStyle = `rgba(255,255,255,${p.alpha})`
                } else {
                    // 拖尾粒子：使用对应颜色
                    ctx.beginPath()
                    ctx.arc(0, 0, p.size * 0.3, 0, Math.PI * 2)
                    ctx.fillStyle = `rgba(${r},${g},${b},${p.alpha})`
                }
                ctx.fill()
                ctx.restore()
            }
            ctx.globalCompositeOperation = "source-over"
        }
    }
    
    Timer {
        interval: 16
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            updateAnimations()
            canvas.requestPaint()
        }
    }
}
