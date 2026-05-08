# OEM Splash Screen 工具

将任意图片转换为 Meshtastic 开机屏 XBM 头文件（`oem_splash.h`）。

---

## 文件说明

| 文件         | 说明                                              |
| ------------ | ------------------------------------------------- |
| `img2oem.sh` | 入口脚本：自动建 venv、安装依赖、调用转换         |
| `img2xbm.py` | 转换核心：解析图片 → 生成 C 头文件                |
| `.venv/`     | Python 虚拟环境（首次运行自动创建，勿提交至 git） |

---

## 快速使用

```bash
# 1. 进入目标板级目录（输出默认写到当前目录的 oem_splash.h）
cd variants/nrf52840/eink-ra01s-gp02

# 2. 转换图片（首次自动建 venv 并安装 Pillow）
/path/to/.github/tools/oem_splash/img2oem.sh \
    --input /path/to/icon.png \
    --width 64 --height 64 \
    --text "MY DEVICE" \
    --preview

# 3. 如果 E-ink 显示颜色反了，加 --invert 重跑
/path/to/.github/tools/oem_splash/img2oem.sh \
    --input /path/to/icon.png \
    --width 64 --height 64 \
    --text "MY DEVICE" \
    --invert \
    --preview

# 4. 指定输出路径（不使用默认的 $PWD/oem_splash.h）
/path/to/.github/tools/oem_splash/img2oem.sh \
    --input icon.png \
    --output /custom/path/oem_splash.h
```

---

## 所有参数

| 参数          | 简写 | 默认值              | 说明                                       |
| ------------- | ---- | ------------------- | ------------------------------------------ |
| `--input`     | `-i` | 必填                | 输入图片路径                               |
| `--width`     | `-W` | `64`                | 输出宽度（像素）                           |
| `--height`    | `-H` | `64`                | 输出高度（像素）                           |
| `--threshold` | `-t` | `128`               | 灰度阈值 0–255，≤ 此值的像素为前景         |
| `--invert`    | `-v` | 否                  | 反转前景/背景（E-ink 显示颜色反时使用）    |
| `--text`      | `-T` | `MY DEVICE`         | 图标下方显示的 OEM 文字                    |
| `--font-size` | `-f` | `1`                 | 文字大小：0=小 1=中 2=大                   |
| `--output`    | `-o` | `$PWD/oem_splash.h` | 输出头文件路径                             |
| `--preview`   | `-p` | 否                  | 在终端打印 ASCII 预览（`#`=前景 `.`=背景） |

---

## 支持的输入格式

| 格式             | 说明                                          |
| ---------------- | --------------------------------------------- |
| `.png`           | 推荐格式，支持透明通道                        |
| `.jpg` / `.jpeg` | 有损压缩，细节可能丢失                        |
| `.bmp`           | Windows 位图                                  |
| `.ico`           | Windows 图标（多尺寸，自动选最大帧）          |
| `.ani`           | Windows 动态光标（RIFF 格式，自动提取第一帧） |

**透明通道处理**：RGBA / LA 模式图片会以白色背景合并透明区域后再做二值化。

---

## 输出文件要求

生成的 `oem_splash.h` 通过以下宏定义与固件交互：

```c
#define USERPREFS_OEM_TEXT         "MY DEVICE"   // 图标下方文字
#define USERPREFS_OEM_FONT_SIZE    1              // 字号 0/1/2
#define USERPREFS_OEM_IMAGE_WIDTH  64             // 图像宽度
#define USERPREFS_OEM_IMAGE_HEIGHT 64             // 图像高度
#define USERPREFS_OEM_IMAGE_DATA   { 0x00, ... }  // XBM 字节数据
```

`variant.h` 中通过 `#include "oem_splash.h"` 引入，无需其他修改。

---

## 图像尺寸建议

屏幕为 **250×122 px**（E-ink GDEM0213B74），图标会水平居中显示，下方留一行文字。

| 图像高度 | 可用文字行数   | 建议图像宽度 |
| -------- | -------------- | ------------ |
| ≤ 60 px  | 有文字空间     | ≤ 200 px     |
| ≤ 80 px  | 文字空间很小   | ≤ 200 px     |
| > 100 px | 文字可能被遮挡 | —            |

推荐：**64×64 px** 或 **80×60 px**，正方形图标最通用。

---

## E-ink 颜色极性说明

Meshtastic E-ink 驱动中颜色映射是**反转的**：

- XBM bit=1 → `OLEDDISPLAY_COLOR::WHITE` → 实际显示为**黑色**
- XBM bit=0 → 实际显示为**白纸色（白色）**

因此：

- 源图中**深色像素** → bit=1 → E-ink 黑色（正常）
- 如果实际效果相反，加 `--invert` 重新生成

可以用 `--preview` 的 ASCII 预览提前验证：`#` 对应的区域在 E-ink 上会显示为黑色。

---

## 开机屏流程说明

定义了 `USERPREFS_OEM_TEXT` 后，固件开机流程变为两阶段：

```text
上电
  └─ 0 ~ 5 s   : 显示标准 Meshtastic logo（icon.xbm）
  └─ 5 ~ 10 s  : 切换到 OEM 屏（oem_splash.h 中的图像 + 文字）
  └─ 10 s 后   : 进入正常操作界面
```

---

## 依赖

- Python 3.8+（系统自带）
- [Pillow](https://pillow.readthedocs.io/)（首次运行由 `img2oem.sh` 自动安装到 `.venv/`）

无需 root 权限，无需修改系统 Python 环境。
