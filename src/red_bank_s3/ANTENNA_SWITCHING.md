# 天线自动切换功能

## 概述

Meshtastic固件现在支持根据LoRa区域配置自动切换天线。当用户通过手机App更改LoRa区域设置时（例如从中国470-510MHz切换到美国902-928MHz），设备会自动切换相应的天线。

## 功能特性

- **自动检测区域变化**：当LoRa区域配置更改时自动触发天线切换
- **支持多种区域**：支持所有Meshtastic定义的LoRa区域
- **智能切换逻辑**：根据频率范围自动选择合适的天线
- **用户通知**：切换时向用户发送通知消息
- **状态跟踪**：跟踪当前使用的天线类型

## 支持的频率分组

### 432MHz天线组
- 中国 (CN): 470-510MHz
- 欧盟433MHz (EU_433): 433-434MHz
- 乌克兰433MHz (UA_433): 433-434.7MHz
- 马来西亚433MHz (MY_433): 433-435MHz
- 菲律宾433MHz (PH_433): 433-434.7MHz

### 900MHz天线组
- 美国 (US): 902-928MHz
- 澳大利亚/新西兰 (ANZ): 915-928MHz
- 日本 (JP): 920.5-923.5MHz
- 韩国 (KR): 920-923MHz
- 台湾 (TW): 920-925MHz
- 泰国 (TH): 920-925MHz
- 马来西亚919MHz (MY_919): 919-924MHz
- 新加坡923MHz (SG_923): 917-925MHz
- 菲律宾915MHz (PH_915): 915-918MHz
- 欧盟868MHz (EU_868): 869.4-869.65MHz
- 俄罗斯 (RU): 868.7-869.2MHz
- 印度 (IN): 865-867MHz
- 新西兰865MHz (NZ_865): 864-868MHz
- 乌克兰868MHz (UA_868): 868-868.6MHz
- 菲律宾868MHz (PH_868): 868-869.4MHz

## 硬件要求

### 基本要求
- 支持双天线切换的硬件设计
- 两个独立的GPIO引脚用于天线控制
- 天线切换开关（继电器或RF开关）

### 引脚定义
在您的硬件变体文件中定义以下引脚：

```cpp
#define ANTENNA_432_PIN 25  // 432MHz天线使能引脚
#define ANTENNA_900_PIN 26  // 900MHz天线使能引脚
```

### 硬件连接建议
1. **天线切换开关**：使用高质量的RF开关或继电器
2. **隔离设计**：确保两个天线之间不会产生干扰
3. **LED指示**：可选添加LED指示当前使用的天线
4. **保护电路**：添加适当的保护电路避免损坏

## 使用方法

### 1. 硬件配置
1. 将示例变体文件复制到您的硬件目录
2. 根据实际硬件修改引脚定义
3. 编译时选择相应的变体

### 2. 软件配置
天线切换功能已集成到固件中，无需额外配置。当用户通过手机App更改LoRa区域时，系统会自动：

1. 检测区域变化
2. 确定需要切换的天线类型
3. 控制相应的GPIO引脚
4. 发送通知给用户

### 3. 用户操作
1. 打开Meshtastic手机App
2. 进入设备设置
3. 更改LoRa区域设置
4. 设备会自动切换天线并显示通知

## 代码结构

### 核心文件
- `src/AntennaManager.h` - 天线管理器头文件
- `src/AntennaManager.cpp` - 天线管理器实现
- `src/modules/AdminModule.cpp` - 配置更改处理
- `src/main.cpp` - 初始化代码

### 主要类和方法

#### AntennaManager类
```cpp
class AntennaManager {
public:
    static void init();  // 初始化天线管理器
    static void switchAntennaForRegion(oldRegion, newRegion);  // 切换天线
    static const char* getCurrentAntennaType();  // 获取当前天线类型
    static bool needsAntennaSwitch(oldRegion, newRegion);  // 检查是否需要切换
};
```

#### 配置处理流程
1. `AdminModule::handleSetConfig()` 接收配置更改
2. 调用 `AntennaManager::switchAntennaForRegion()` 处理天线切换
3. 根据区域类型控制相应的GPIO引脚
4. 发送用户通知

## 调试和故障排除

### 常见问题

1. **天线不切换**
   - 检查引脚定义是否正确
   - 验证硬件连接
   - 查看日志输出

2. **切换时产生干扰**
   - 确保两个天线不会同时激活
   - 检查天线隔离设计
   - 考虑添加切换延时

3. **通知不显示**
   - 检查客户端通知功能是否启用
   - 查看设备日志

### 调试方法

1. **查看日志**
   ```
   LOG_INFO("Switching antenna: old region=%d, new region=%d", oldRegion, newRegion);
   LOG_INFO("Switching to 432MHz antenna");
   LOG_INFO("Switching to 900MHz antenna");
   ```

2. **测试引脚**
   ```cpp
   // 手动测试天线切换
   digitalWrite(ANTENNA_432_PIN, HIGH);
   digitalWrite(ANTENNA_900_PIN, LOW);
   ```

3. **检查状态**
   ```cpp
   const char* antennaType = AntennaManager::getCurrentAntennaType();
   LOG_INFO("Current antenna: %s", antennaType);
   ```

## 扩展功能

### 自定义天线配置
您可以修改 `AntennaManager.cpp` 中的区域映射来支持自定义天线配置：

```cpp
case meshtastic_Config_LoRaConfig_RegionCode_CUSTOM:
    switchToCustomAntenna();
    break;
```

### 多天线支持
对于支持更多天线的硬件，可以扩展天线管理器：

```cpp
#define ANTENNA_2_4GHZ_PIN 27  // 2.4GHz天线
#define ANTENNA_5_8GHZ_PIN 28  // 5.8GHz天线
```

### 自动测试功能
可以添加天线自动测试功能：

```cpp
void AntennaManager::testAntennas() {
    // 测试432MHz天线
    switchTo432MHzAntenna();
    delay(1000);
    
    // 测试900MHz天线
    switchTo900MHzAntenna();
    delay(1000);
}
```

## 注意事项

1. **安全性**：确保天线切换不会影响设备稳定性
2. **兼容性**：新功能向后兼容，不影响现有设备
3. **性能**：天线切换是即时操作，不会影响通信性能
4. **功耗**：天线切换会增加少量功耗，但影响很小

## 贡献

如果您发现bug或有改进建议，请通过以下方式贡献：

1. 提交Issue到GitHub
2. 创建Pull Request
3. 参与社区讨论

## 许可证

此功能遵循Meshtastic项目的开源许可证。 