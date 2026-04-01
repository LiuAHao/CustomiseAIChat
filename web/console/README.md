# console 前端模块

## 模块定位

该模块是 `go-agent-platform` 的桌面化前端控制台，视觉方向参考 Codex 类应用的浅色新拟物极简风格，但将核心对象从“线程”重构为“Agent”。

## 当前实现

- 采用 React + Vite 构建
- 使用组件拆分结构：`App`、`Sidebar`、`AgentDashboard`、`AuthPage`
- 左侧为项目与 Agent 列表，包含技能、MCP、系统设置入口
- 中间为主工作区，包含顶部状态栏、中心引导区、快捷能力卡片、底部命令输入框
- 提供基础登录页与退出登录交互
- 提供多视图切换：Dashboard、技能与应用、MCP 配置、系统设置

## 目录说明

- `src/App.tsx`：应用总布局与登录态切换
- `src/components/Sidebar.tsx`：左侧导航和 Agent 列表
- `src/components/AgentDashboard.tsx`：主工作区与多视图内容
- `src/components/AuthPage.tsx`：登录注册原型页
- `src/styles.css`：整套视觉样式和响应式布局

## 后续建议

- 把当前模拟的 Agent 列表替换成后端接口数据
- 将技能页和 MCP 页接入真实配置项与状态流
- 为命令输入框增加真正的消息发送、上下文引用和文件选择能力
- 接入 Monaco Editor 或可视化流程画布以支持技能编辑和 Agent 编排
