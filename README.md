# BAShake (KWin 6 鼠标特效插件)

一个高性能的原生 KWin 插件，灵感来源于 [BASpark](https://github.com/DoomVoss/BASpark)。为你的 KDE Plasma 桌面带来炫酷的鼠标点击光环、拖尾与粒子爆炸效果。

## ✨ 特性
- 🌈 **多按键颜色**：左键（蓝色）、中键（绿色）、右键（红色）独立颜色反馈
- 💧 **半透明光环**：点击时扩散的半透明波纹（Alpha=170），色彩通透且不遮挡背景内容
- ✨ **精准粒子生命周期**：爆炸三角形粒子在光环消失后约 0.15 秒自然消散，拖尾粒子同步快速消隐，视觉节奏干净利落
- 📝 **动态彩色拖尾**：鼠标按住移动时生成跟随当前按键颜色的渐变轨迹
- 🚀 **极致性能**：采用 C++ 对象池内存管理，运行全程零动态分配，GL 绘制开销极低

## 预览

```bash
qmlsence main.qml
#或者
cd test/build
cmake .. && make && ./preview
```

## 📦 安装
确保系统已安装 KDE Plasma 6 开发环境。

```bash
# 1. 进入项目目录并创建构建目录
cd kwin-effect-cursor-halo
mkdir build && cd build

# 2. 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr

# 3. 编译
cmake --build .

# 4. 安装
sudo cmake --install .
```
安装完成后，前往 **系统设置 > 窗口管理 > 桌面特效** 搜索 **"BAShark-plasma"** 并启用即可。

## 🗑️ 卸载
如需彻底移除该特效并恢复原始鼠标行为，请依次执行：

```bash
# 1. 删除插件文件
sudo rm /usr/lib/qt6/plugins/kwin/effects/plugins/cursorhalo.so
```

执行完毕后，特效将立即停止运行。

## 📜 许可证
本项目基于 **MIT License** 开源。
