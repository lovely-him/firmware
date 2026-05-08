# 动画帧转换工具

将 Windows `.ani` 光标文件批量转换为 Meshtastic E-ink 动画头文件（`anim_frames.h`）。
每个 `.ani` 文件成为一个独立动画，保留其真实帧数，运行时可通过设备菜单切换。

---

## 文件说明

| 文件          | 说明                                              |
| ------------- | ------------------------------------------------- |
| `img2anim.sh` | 入口脚本：自动建 venv、安装依赖、调用转换         |
| `img2anim.py` | 转换核心：解析 ANI 帧 → 生成 C 头文件             |
| `.venv/`      | Python 虚拟环境（首次运行自动创建，勿提交至 git） |

---

## 快速使用

```bash
# 1. 进入目标板级目录（输出默认写到当前目录的 anim_frames.h）
cd variants/nrf52840/eink-ra01s-gp02

# 2. 扫描目录下所有 .ani 文件并生成头文件
/path/to/.github/tools/anim_frames/img2anim.sh \
    --input /path/to/cursors/

# 3. 仅转换指定文件（逗号分隔）
/path/to/.github/tools/anim_frames/img2anim.sh \
    --input pointer.ani,busy.ani,work.ani

# 4. 预览每个动画的第一帧（ASCII 字符画）
/path/to/.github/tools/anim_frames/img2anim.sh \
    --input /path/to/cursors/ \
    --preview

# 5. 如果 E-ink 颜色反了，加 --invert 重跑
/path/to/.github/tools/anim_frames/img2anim.sh \
    --input /path/to/cursors/ \
    --invert

# 6. 指定输出路径
/path/to/.github/tools/anim_frames/img2anim.sh \
    --input /path/to/cursors/ \
    --output /custom/path/anim_frames.h
```

---

## 所有参数

| 参数          | 简写 | 默认值               | 说明                                    |
| ------------- | ---- | -------------------- | --------------------------------------- |
| `--input`     | `-i` | 必填                 | ANI 目录路径，或逗号分隔的文件列表      |
| `--width`     | `-W` | `64`                 | 输出帧宽度（像素）                      |
| `--height`    | `-H` | `64`                 | 输出帧高度（像素）                      |
| `--threshold` | `-t` | `128`                | 灰度阈值 0–255，≤ 此值的像素为前景      |
| `--invert`    | `-v` | 否                   | 反转前景/背景（E-ink 显示颜色反时使用） |
| `--output`    | `-o` | `$PWD/anim_frames.h` | 输出头文件路径                          |
| `--preview`   | `-p` | 否                   | 在终端打印每个动画第一帧的 ASCII 预览   |

---

## 支持的输入格式

| 格式                              | 说明                                           |
| --------------------------------- | ---------------------------------------------- |
| `.ani`                            | Windows 动态光标（RIFF/ACON 格式，提取全部帧） |
| `.png` / `.jpg` / `.bmp` / `.ico` | 静态图片，作为单帧动画处理                     |

**透明通道处理**：RGBA / LA 模式图片会以白色背景合并透明区域后再做二值化。

---

## 输出文件结构

生成的 `anim_frames.h` 包含以下内容：

```c
// 全局常量
#define ANIM_FRAME_WIDTH     64
#define ANIM_FRAME_HEIGHT    64
#define ANIM_BYTES_PER_FRAME 512   // (64/8) * 64
#define ANIM_COUNT           15    // ANI 文件数量

// 每个动画一个扁平数组（帧数 × ANIM_BYTES_PER_FRAME 字节）
static const uint8_t ANIM_DATA_BUSY[] = { /* 11 frames */ ... };
static const uint8_t ANIM_DATA_POINTER[] = { /* 2 frames */ ... };
// ...

// 描述符结构体
struct AnimDesc {
    const char    *name;       // 显示名（文件名去掉扩展名）
    const uint8_t *data;       // 指向对应 ANIM_DATA_xxx 数组
    uint8_t        frameCount; // 该动画帧数
};

// 描述符表
static const AnimDesc ANIM_LIST[ANIM_COUNT] = {
    { "busy",    ANIM_DATA_BUSY,    11 },
    { "pointer", ANIM_DATA_POINTER,  2 },
    // ...
};
```

固件通过 `AnimationRenderer::setAnimation(idx)` 切换当前动画，
通过 `AnimationRenderer::getAnimName(idx)` 获取显示名称用于菜单。

---

## 固件集成说明

生成的头文件须放在板级变体目录下，并通过 `variant.h` 中定义的
`EINK_RA01S_GP02` 宏条件引入，无需修改固件其他部分：

```text
variants/nrf52840/eink-ra01s-gp02/anim_frames.h   ← 生成于此
src/graphics/draw/AnimationRenderer.cpp             ← #include "anim_frames.h"（已有）
src/graphics/draw/MenuHandler.cpp                   ← 动态从 ANIM_LIST 构建菜单（已有）
```

重新生成头文件后直接编译烧录即可，菜单中的动画列表会自动更新。

---

## 添加/更换动画

1. 将新的 `.ani` 文件放入素材目录
2. 重新运行 `img2anim.sh --input /path/to/cursors/`
3. 生成的 `anim_frames.h` 覆盖旧文件
4. `pio run -e eink-ra01s-gp02 -t upload` 编译并烧录

---

## E-ink 颜色极性说明

与 OEM 启动画面相同，Meshtastic E-ink 驱动颜色映射是**反转的**：

- XBM bit=1 → 实际显示为**黑色**
- XBM bit=0 → 实际显示为**白纸色（白色）**

因此深色像素 → 黑色（正常）。若显示效果相反，加 `--invert` 重新生成。

---

## 刷新率说明

E-ink 快速刷新约 **300–500 ms**，慢刷新约 **2 s**（每 20 帧触发一次）。
动画帧间隔固定为 **1000 ms**，适合光标等低速动画素材。

---
