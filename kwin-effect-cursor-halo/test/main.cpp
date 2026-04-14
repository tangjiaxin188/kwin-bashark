// MouseSpark 效果预览 - 独立 Qt 程序
// 1:1 还原 MouseSpark.qml 的所有效果
#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QDateTime>
#include <QTimer>
#include <cmath>

// ─── 数据结构（1:1 对应 QML）───
struct TrailPoint { QPointF pos; qint64 timestamp; };
struct WaveData {
    QPointF pos; QColor color;
    float radius = 0, maxRadius = 60, ringAngle = 0;
    float ringLife = 0, ringMaxLife = 50, ringSpeed = 0.03f;
    int life = 0, maxLife = 25;
    struct Seg { float off, len; } segs[3] = {
        {-0.25f*M_PI, 1.15f*M_PI}, {0, 1.15f*M_PI}, {0.25f*M_PI, 1.15f*M_PI}
    };
};
struct Particle {
    QPointF pos; QColor color; QPointF velocity;
    float rotation = 0, rotationSpeed = 0, size = 0;
    float alpha = 1.0f, decay = 0.95f; bool isTriangle = false;
};

class PreviewWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    PreviewWidget() {
        setWindowTitle("MouseSpark 预览 - 左键蓝/中键绿/右键红 | 移动→拖尾 点击→波纹+粒子");
        resize(1400, 900);
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]{
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            qreal dt = std::min(qreal(now - m_t0), qreal(100));
            m_t0 = now; m_fs = dt / 16.0;
            updateState();
            update();
        });
        m_timer->start(16);
    }

private:
    void updateState() {
        const qint64 trailNow = QDateTime::currentMSecsSinceEpoch();
        while (!m_trail.isEmpty() && (trailNow - m_trail.first().timestamp) > 300)
            m_trail.removeFirst();

        for (int i = m_waves.size()-1; i >= 0; --i) {
            auto &w = m_waves[i];
            w.life += m_fs; w.ringLife += m_fs;
            qreal pr = qreal(w.life) / w.maxLife;
            qreal sp = std::min(pr * 0.5, 1.0);
            w.radius = w.maxRadius * (1 - std::pow(1 - float(sp), 2.5f));
            w.ringAngle -= w.ringSpeed * m_fs;
            if (w.life >= w.maxLife && w.ringLife >= w.ringMaxLife) m_waves.removeAt(i);
        }
        for (int i = m_particles.size()-1; i >= 0; --i) {
            auto &p = m_particles[i];
            p.pos += p.velocity * m_fs;
            p.velocity *= std::pow(p.decay, m_fs);
            p.rotation += p.rotationSpeed * m_fs;
            p.alpha -= 0.01f * m_fs;
            if (p.alpha <= 0) m_particles.removeAt(i);
        }
    }

    void render() {
        glClearColor(0.04f, 0.04f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glOrtho(0, width(), height(), 0, -1, 1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        // 1️⃣ 拖尾轨迹（渐变线）
        if (m_trail.size() > 1) {
            glLineWidth(6);
            for (int i = 1; i < m_trail.size(); ++i) {
                float alpha = float(i) / (m_trail.size() - 1);
                QColor c = m_ac;
                glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
                glBegin(GL_LINES);
                glVertex2f(m_trail[i-1].pos.x(), m_trail[i-1].pos.y());
                glVertex2f(m_trail[i].pos.x(), m_trail[i].pos.y());
                glEnd();
            }
        }

        // 2️⃣ 波纹填充圆
        for (const auto &w : m_waves) {
            float pr = float(w.life) / w.maxLife;
            float alpha = std::pow(1 - std::min(pr, 1.0f), 1.2f);
            if (alpha < 0.01f) continue;
            const int N = 64; const float t = 6.2832f/N;
            const float cc = std::cos(t), ss = std::sin(t);
            float r = w.radius, cx = w.pos.x(), cy = w.pos.y();
            float x = r, y = 0, tmp;
            glColor4f(w.color.redF(), w.color.greenF(), w.color.blueF(), alpha);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(cx, cy);
            for (int i = 0; i < N; ++i) {
                glVertex2f(x + cx, y + cy);
                tmp = x; x = cc*x - ss*y; y = ss*tmp + cc*y;
            }
            glEnd();
        }

        // 3️⃣ 装饰弧线
        for (const auto &w : m_waves) {
            float rp = float(w.ringLife) / w.ringMaxLife;
            float rA = std::pow(1 - std::min(rp, 1.0f), 1.2f);
            if (rA < 0.01f) continue;
            float shrink = std::max(0.0f, 1.0f - rp);
            float r = w.radius + 3;
            for (int si = 0; si < 3; ++si) {
                float len = w.segs[si].len * shrink;
                float start = w.ringAngle + w.segs[si].off;
                int steps = int(len / 0.08f) + 2;
                glColor4f(245/255.0f, 248/255.0f, 252/255.0f, rA);
                glLineWidth(3.7);
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i < steps; ++i) {
                    float a = start + len * float(i) / (steps - 1);
                    glVertex2f(w.pos.x() + std::cos(a)*r, w.pos.y() + std::sin(a)*r);
                }
                glEnd();
            }
        }

        // 4️⃣ 粒子
        for (const auto &p : m_particles) {
            float cx = p.pos.x(), cy = p.pos.y();
            float cr = std::cos(p.rotation), sr = std::sin(p.rotation);
            auto rp = [&](float px, float py) {
                return QPointF(cx + (px*cr - py*sr), cy + (px*sr + py*cr));
            };
            glColor4f(p.isTriangle ? 1 : p.color.redF(),
                      p.isTriangle ? 1 : p.color.greenF(),
                      p.isTriangle ? 1 : p.color.blueF(), p.alpha);
            if (p.isTriangle) {
                float s = p.size;
                auto v0 = rp(0, -s), v1 = rp(s*0.6f, s*0.6f), v2 = rp(-s*0.6f, s*0.6f);
                glBegin(GL_TRIANGLES);
                glVertex2f(v0.x(), v0.y()); glVertex2f(v1.x(), v1.y()); glVertex2f(v2.x(), v2.y());
                glEnd();
            } else {
                const int N = 16; const float t = 6.2832f/N;
                const float cc = std::cos(t), ss = std::sin(t);
                float r = p.size * 0.3f, x = r, y = 0, tmp;
                glLineWidth(1.5);
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < N; ++i) {
                    auto pt = rp(x, y);
                    glVertex2f(pt.x(), pt.y());
                    tmp = x; x = cc*x - ss*y; y = ss*tmp + cc*y;
                }
                glEnd();
            }
        }

        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
    }

    void initializeGL() override { initializeOpenGLFunctions(); }
    void paintGL() override { render(); }

    void mouseMoveEvent(QMouseEvent *e) override {
        QPointF p = e->position();
        if (std::hypot(p.x()-m_last.x(), p.y()-m_last.y()) > 2) {
            m_trail.append({p, QDateTime::currentMSecsSinceEpoch()});
            if (m_trail.size() > 16) m_trail.removeFirst();
            m_last = p;
            if (m_down && std::rand()%100 < 30) spawnP(p, false, m_ac);
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        m_down = true;
        if (e->buttons() & Qt::MiddleButton) m_ac = QColor(46,204,113);
        else if (e->buttons() & Qt::RightButton) m_ac = QColor(231,76,60);
        else m_ac = QColor(45,175,255);

        spawnWave(e->position(), m_ac);
        int count = 4 + std::rand() % 4; // 4~7 个
        for (int i = 0; i < count; ++i) spawnP(e->position(), true, m_ac, 60);
        m_last = e->position();
    }
    void mouseReleaseEvent(QMouseEvent*) override { m_down = false; m_ac = QColor(45,175,255); }

    void spawnWave(const QPointF &pos, const QColor &color) {
        WaveData w; w.pos = pos; w.color = color;
        w.maxRadius = 60;
        w.ringAngle = std::rand()/float(RAND_MAX) * 6.2832f;
        m_waves.append(w);
    }
    void spawnP(const QPointF &pos, bool boom, const QColor &color, float offset = 0) {
        Particle p;
        p.color = color;
        
        if (boom && offset > 0) {
            // Explosion particles: spawn at inner edge of the ring (50%~70% of radius)
            float r = offset * (0.5f + std::rand()/float(RAND_MAX) * 0.2f);
            float a = std::rand()/float(RAND_MAX) * 6.2832f;
            p.pos = QPointF(pos.x() + std::cos(a)*r, pos.y() + std::sin(a)*r);
            
            // Velocity: random direction (some in, some out), small magnitude
            float va = std::rand()/float(RAND_MAX) * 6.2832f;
            float speed = 0.5f + std::rand()/float(RAND_MAX) * 0.5f; // 0.5 ~ 1.0
            p.velocity = QPointF(std::cos(va)*speed, std::sin(va)*speed);
            
            p.size = (3.0f + std::rand()/float(RAND_MAX) * 4.0f) * 1.5f;
            p.decay = 0.95f;
            p.isTriangle = true;
        } else {
            // Trail particles
            p.pos = pos;
            float a = std::rand()/float(RAND_MAX) * 6.2832f;
            float sp = 1.3f * 0.5f * 1.5f; // ~1.0
            p.velocity = QPointF(std::cos(a)*sp + (std::rand()/float(RAND_MAX)-0.5f)*2,
                                 std::sin(a)*sp + (std::rand()/float(RAND_MAX)-0.5f)*2);
            p.size = 9 * 1.5f;
            p.decay = 0.97f;
            p.isTriangle = false;
        }
        
        p.rotation = std::rand()/float(RAND_MAX) * 6.2832f;
        p.rotationSpeed = (std::rand()/float(RAND_MAX)-0.5f) * 0.15f;
        p.alpha = 1;
        m_particles.append(p);
    }

    QList<TrailPoint> m_trail;
    QList<WaveData> m_waves;
    QList<Particle> m_particles;
    QTimer *m_timer;
    qint64 m_t0 = QDateTime::currentMSecsSinceEpoch();
    qreal m_fs = 1;
    QPointF m_last;
    bool m_down = false;
    QColor m_ac = QColor(45,175,255);
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    PreviewWidget w;
    w.show();
    return app.exec();
}

#include "main.moc"
