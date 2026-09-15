# 42 Compute Water

独立的 DX11 水波示例，拥有自己的入口、模拟实现、着色器和资源。
仅复用仓库的 `Project 19-/Common` 窗口框架与 ImGui，不依赖另一水波工程或原对比示例。

使用计算着色器和 R32F 高度纹理，保留 8×8 工作组、雨滴扰动和法线水面；不加载船体或陆地资源。

## 构建运行

需要 Windows、Visual Studio C++ 桌面工具、Windows SDK 和 CMake。
在本目录执行：

```powershell
.\Build.ps1 -Configuration Release -Validate
```

默认输出：`build-compute-water/Release/42 Compute Water.exe`。
去掉 `-Validate` 只构建；直接打开 exe 进行交互。
Debug 验证使用 `-Configuration Debug`，需要安装 Windows 图形工具以提供 D3D11 调试层。

也可在根解决方案中选择 `42_Compute_Water` 为启动项目，使用 Debug/Release、x64。
通过根 CMake 或 xmake 构建时，目标名称同为 `42_Compute_Water`。

## 操作

- 左键：注入水波；右键拖动：旋转视角；滚轮：缩放。
- Pause / Reset：暂停或清空模拟。
- Resolution：128、256、512，切换时重置。
- Display：水面、波高。
- Rain drops / Damping：雨滴开关和衰减。

## 验证

`--validate` 执行 CPU/GPU 数值对照、重置、暂停、鼠标扰动、各显示模式、分辨率和窗口尺寸切换，并渲染 240 帧。
输出目录中生成 `validation.txt` 和 `validation.png`；Debug 验证还检查 D3D11 调试层。
第三方许可保存在 `Assets` 目录。
