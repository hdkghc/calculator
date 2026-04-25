Under development


# Calculator for Raspberry Pi Pico / 适用于树莓派Pico的计算器

## English | 英文

This project aims to be the most powerful and feature-rich scientific and graphical calculator for the Raspberry Pi Pico microcontroller—far surpassing most commercial scientific and graphical calculators in both capability and flexibility. It is not a simple demo, but a long-term, ambitious open-source project.

本项目致力于打造功能最强大、特性最丰富的树莓派Pico科学与图形计算器，目标远超市面上绝大多数同类产品。这不仅仅是一个简单的演示，而是一个长期的、充满雄心的开源项目。

### Features | 功能
- Advanced scientific and graphical calculation (in progress)
- Modular, extensible codebase
- Full support for ST7735 color display (required)
- Custom font and UI rendering
- Designed for Raspberry Pi Pico
- Ambitious roadmap: graphing, programmability, symbolic math, and more

### Project Structure | 项目结构
- `main.cpp`: Main application source code | 主程序源代码
- `CMakeLists.txt`: Build configuration for CMake | CMake 构建配置
- `inc/`: Header files | 头文件目录
- `fonts/`, `font2h/`: Font resources and conversion tools | 字体资源及转换工具
- `pico-st7735/`: ST7735 display driver and related code (required) | ST7735 显示驱动及相关代码（必需）
- `build/`, `bin/`: Build output directories | 构建输出目录

### Build Instructions | 构建方法
1. Install the Raspberry Pi Pico SDK and toolchain. | 安装树莓派 Pico SDK 及工具链。
2. Clone this repository and initialize submodules if needed. | 克隆本仓库，如有子模块请初始化。
3. **Recommended:** Use the provided `build.sh` script in the project root for automated build. | **推荐：**使用根目录下的 `build.sh` 脚本自动构建。
   ```sh
   ./build.sh
   ```
   Or, build manually. | 或手动构建。
4. Flash the generated `.uf2` file to your Pico using your preferred method (e.g., drag-and-drop or `picotool`). | 使用你喜欢的方法（如拖拽或 `picotool`）将生成的 `.uf2` 文件烧录到 Pico。

### Dependencies | 依赖
- Raspberry Pi Pico SDK
- **ST7735 display library (required, included in `pico-st7735/`)** | **ST7735 显示屏库（必需，已包含在 `pico-st7735/`）**

### License | 许可证
This project is licensed under the GNU General Public License v3.0 (GPLv3). See LICENSE for details.

本项目采用 GNU 通用公共许可证 第三版（GPLv3）。详情见 LICENSE 文件。

---

## Contribution | 贡献
Contributions are welcome! Please read CONTRIBUTION.md for guidelines.

欢迎贡献代码！请阅读 CONTRIBUTION.md 了解贡献指南。
