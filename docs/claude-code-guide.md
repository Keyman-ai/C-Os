# Claude Code 使用指南

本指南介绍 Claude Code 的常用功能与使用方法，帮助你快速上手并高效使用这个终端内的 AI 编程助手。

## 1. Claude Code 简介

Claude Code 是运行在终端里的 AI 编程助手。与简单的代码补全工具不同，它能够：

- **理解整个代码库** — 自动读取项目结构、搜索代码、定位相关文件
- **读写文件** — 直接编辑代码、创建新文件、重构项目
- **执行命令** — 运行构建、测试、git 操作等 shell 命令
- **访问外部信息** — 搜索网页、抓取文档
- **并行处理** — 派发子代理并行处理大型任务

你可以用自然语言下达指令，Claude Code 会自主决定调用哪些工具完成工作。

## 2. 快速开始

**安装**（需要 Node.js）：

```bash
npm install -g @anthropic-ai/claude-code
```

**登录**：

```bash
claude            # 首次运行会引导 OAuth 登录
# 或通过 /login 命令重新登录
```

也可以用环境变量设置 API Key：

```bash
export ANTHROPIC_API_KEY="你的密钥"
```

**启动**：

```bash
claude            # 在项目目录中启动，自动读取项目上下文
```

启动后进入交互模式，直接在提示符后输入你的需求即可。建议**在项目根目录启动**，这样 Claude Code 才能读取项目配置（CLAUDE.md）、掌握构建命令等上下文。

## 3. 基本交互

- **发送消息**：输入内容后按 Enter
- **多行输入**：Shift+Enter 插入换行
- **中断响应**：按 Esc 或 Ctrl+C
- **历史记录**：↑ / ↓ 方向键翻阅，Ctrl+R 反向搜索
- **显示思考过程**：Ctrl+O 切换

**工具调用与审批**：当 Claude Code 需要执行命令或修改文件时，会根据权限模式决定是否征求你的同意。需要审批时输入：

- `y` — 允许本次调用
- `n` — 拒绝本次调用
- `always` — 永久允许（会写入 `.claude/settings.local.json`）

## 4. 权限模式

Claude Code 有四种权限模式，控制工具调用的审批程度：

| 模式 | 行为 | 启动方式 |
|---|---|---|
| **default**（默认） | 读操作自动批准；写文件、执行命令需确认 | `claude` |
| **acceptEdits** | 文件编辑自动批准；命令执行仍需确认 | `claude --permission-mode acceptEdits` |
| **bypassPermissions** | 所有工具调用自动批准（高风险，谨慎使用） | `claude --permission-mode bypassPermissions` |
| **plan** | 先给出计划，经你批准后再执行 | `claude --permission-mode plan` |

会话中可通过 `/config` 面板随时切换模式。

## 5. 常用斜杠命令

在提示符输入 `/` 即可看到命令列表。常用命令如下：

| 命令 | 用途 |
|---|---|
| `/help` | 查看帮助与全部命令 |
| `/clear` | 清空当前会话上下文 |
| `/compact` | 压缩会话历史，节省上下文空间 |
| `/init` | 分析代码库并生成项目的 CLAUDE.md |
| `/memory` | 编辑全局/项目记忆文件 |
| `/config` | 打开配置面板（模型、主题、权限等） |
| `/model` | 切换当前会话使用的模型 |
| `/context` | 查看当前上下文用量（token） |
| `/status` | 查看会话状态（模型、权限、时长等） |
| `/doctor` | 运行诊断，检查安装/配置/网络/权限 |
| `/resume` | 列出并恢复历史会话 |
| `/review` | 审查当前未提交的代码改动 |
| `/login` / `/logout` | 登录 / 退出 |
| `/vim` | 切换 vim 风格键位 |
| `/mcp` | 管理 MCP 服务器 |
| `/hooks` | 查看已配置的钩子 |
| `/permissions` | 查看和修改权限规则 |
| `/upgrade` | 升级 Claude Code 到新版本 |

> 提示：安装的自定义技能（Skills）也会出现在 `/` 命令列表中。

## 6. 计划模式（Plan Mode）

计划模式让 Claude Code 在执行前先产出结构化计划，经你审阅批准后再动手，适合**改动大、风险高**的任务。

使用方式：

- 启动时：`claude --permission-mode plan`
- 会话中：通过 `/config` 切换，或直接说"先给我一个计划"

典型流程：

1. 你提出需求："实现用户认证模块"
2. Claude 输出计划：`1. 创建 auth 模块 2. 添加登录接口 3. 添加会话中间件 ...`
3. 你批准："可以，开始吧"（或要求修改计划）
4. Claude 按计划执行

## 7. 子代理（Subagents）

Claude Code 可以派发子代理并行处理任务。子代理有独立的上下文，处理完把结果返回给主会话。

常用类型：

- **Explore** — 快速探索代码库、回答问题，只读不改
- **general-purpose** — 通用代理，可读写文件、执行命令，适合分解大型任务
- **verification** — 验证实现是否正确（构建、跑测试、检查）

使用场景：

- **并行开发**：大型多文件改动拆给多个子代理，每个负责一部分文件
- **代码库调研**：让 Explore 代理调查某个功能如何实现
- **后台任务**：长任务（跑测试、全库分析）放到后台，你继续干别的，用 `/agents` 查看进度
- **隔离开发**：子代理可在 git worktree 中独立工作，互不干扰

**怎么操作**：直接在指令里要求，或明确指定代理类型：

> "派一个 **Explore** 子代理查一下 `timer.cpp` 是怎么初始化的"
> "把 X 和 Y 两个任务**并行**拆给两个子代理去做"

进度监控：输入 `/agents`。

## 8. MCP 服务器

MCP（Model Context Protocol）是连接外部工具和数据源的开放协议。通过 MCP 服务器，Claude Code 能调用你自己开发或第三方提供的工具。

**添加服务器**：

```bash
claude mcp add 服务器名 -- node /path/to/server.js
claude mcp add 服务器名 -- python -m my_mcp_server
claude mcp add remote --url https://example.com/mcp
```

**管理命令**：

```bash
claude mcp list      # 列出所有服务器
claude mcp status    # 检查连接状态
claude mcp remove 名字
```

**配置文件**：

- `.mcp.json` — 项目级配置（可提交到 git）
- `~/.mcp.json` — 用户级配置（所有项目可用）

MCP 服务器提供的工具会与内置工具一起出现在会话中，Claude 可以像调用内置工具一样调用它们。

## 9. 技能（Skills）

技能是把特定行为封装成可复用的"斜杠命令"——包含系统提示、权限授权和使用说明。调用后 Claude 会遵循技能中的指引执行。

**存放位置**：

- `~/.claude/skills/` — 用户级技能，所有项目可用
- `.claude/skills/` — 项目级技能，仅当前项目可用

每个技能是一个目录，核心是 `SKILL.md` 文件，用 markdown 描述该技能的用途与使用说明。

**调用方式**：

- 斜杠调用：输入 `/技能名`
- 自动调用：Claude 在判断任务匹配时会通过 Skill 工具自动加载相关技能

## 10. 钩子（Hooks）

钩子是**事件驱动的自动化行为**，在会话的特定节点自动执行命令或脚本。必须配置在 `settings.json` 中（不能用对话或记忆配置）。

配置示例：

```json
{
  "hooks": {
    "PreToolUse": [
      { "matcher": "Bash(rm *)", "command": "echo '警告：检测到 rm 命令'" }
    ],
    "PostToolUse": [
      { "matcher": "Write(*.py)", "command": "black {{file_path}}" }
    ],
    "Notification": [
      { "matcher": "", "command": "notify-send 'Claude 已完成'" }
    ],
    "SessionStart": [
      { "command": "echo '会话开始于 $(date)'" }
    ]
  }
}
```

**事件类型**：

| 事件 | 触发时机 |
|---|---|
| `PreToolUse` | 工具执行前（可阻止调用） |
| `PostToolUse` | 工具执行后（可触发后续动作） |
| `UserPromptSubmit` | 用户提交提示词时 |
| `Notification` | Claude 完成响应或需要你注意时 |
| `SessionStart` / `SessionEnd` | 会话开始/结束时 |
| `PreCompact` / `PostCompact` | 上下文压缩前后 |

**matcher** 用于过滤触发范围，如 `Bash(*)`、`Write(*.py)`；留空表示该事件全部触发。

> 用法示例："以后每次执行 rm 命令前警告我"、"每次改完 Python 文件后自动格式化"——这些都需要配置 hooks。

不想手改 JSON 的话，直接让 Claude 用 **update-config** 技能帮你配置 hooks（改动写入 settings.json，属于持久配置，会一直生效）。

## 11. 记忆系统

Claude Code 通过 CLAUDE.md 文件记住项目与用户偏好：

| 文件 | 作用 |
|---|---|
| `~/.claude/CLAUDE.md` | 全局记忆：跨项目的个人偏好与约定 |
| `./CLAUDE.md`（项目根） | 项目记忆：构建命令、架构说明、注意事项 |
| `./AGENTS.md` | 子代理专用的精简版项目说明 |

**使用方式**：

- `/init` — 让 Claude 分析代码库，自动生成项目的 CLAUDE.md
- `/memory` — 交互式查看和编辑记忆文件
- 对话中直接让 Claude 更新 CLAUDE.md 也可以

CLAUDE.md 会在每次会话开始时被读取，是让 Claude 了解项目约定（如"交叉编译用 aarch64-linux-gnu-g++"、"提交前先 make clean"）的主要途径。

## 12. 无头 / 脚本化使用

Claude Code 可以不进入交互模式，单次执行后退出，非常适合脚本和 CI/CD。

**单次执行**：

```bash
claude -p "修复 kernel/main.cpp 中的 bug"
```

**管道输入**：

```bash
echo "解释 git status 的输出" | claude -p -
```

**结构化输出**：

```bash
claude -p "列出所有函数" --output-format json
claude -p "实时分析" --output-format stream-json
```

**限制工具范围**：

```bash
claude -p "找出所有 TODO" --allowedTools "Read,Grep,Glob"
```

**设置权限模式**：

```bash
claude -p "更新版本号" --permission-mode bypassPermissions
```

**CI/CD 典型用法**：

```bash
# 审查 PR 改动
git diff | claude -p "审查以下代码改动，指出问题" --print

# 生成变更日志
claude -p "根据 git log 生成 changelog" --allowedTools "Bash(git:*)"
```

## 13. 配置文件与位置

| 文件 | 位置 | 作用 |
|---|---|---|
| 全局设置 | `~/.claude/settings.json` | 用户级设置：模型、主题、权限、钩子、环境变量 |
| 项目设置 | `.claude/settings.json` | 项目级设置（通常提交到 git） |
| 本地设置 | `.claude/settings.local.json` | 个人项目设置（不提交 git，优先级最高） |
| 全局记忆 | `~/.claude/CLAUDE.md` | 跨项目偏好 |
| 项目记忆 | `./CLAUDE.md` | 项目说明 |
| 技能 | `~/.claude/skills/`、`.claude/skills/` | 自定义技能 |
| MCP 配置 | `.mcp.json`、`~/.mcp.json` | MCP 服务器定义 |
| 会话数据 | `~/.claude/sessions/` | 历史会话（供 /resume） |
| 检查点 | `~/.claude/checkpoints/` | 编辑前的文件快照（可回滚） |
| 键位绑定 | `~/.claude/keybindings.json` | 自定义快捷键 |

**设置优先级**（高 → 低）：`.claude/settings.local.json` → `.claude/settings.json` → `~/.claude/settings.json` → 默认值

权限规则示例：

```json
{
  "permissions": {
    "allow": [
      "Bash(make)",
      "Bash(git:*)",
      "Bash(make clean:*)"
    ],
    "deny": [
      "Bash(rm -rf /*)"
    ]
  }
}
```

## 14. 会话管理

- **恢复会话**：`/resume` 列出历史会话并选择恢复；`claude --continue` 直接恢复最近一次
- **命名会话**：`claude --session-id 会话名` 为会话命名，便于后续恢复
- **分叉会话**：`claude --fork-session 会话ID` 基于已有会话分支出一个新会话
- **多会话**：多个终端各自运行 `claude` 即为独立会话，可用 `--session-id` 区分
- **压缩上下文**：长会话用 `/compact` 压缩历史，节省 token 并继续工作

## 15. 模型选择

**切换模型**：

- 会话中：`/model`
- 启动时：`claude --model claude-opus-4-7`

**常用模型**：

| 模型 | 特点 |
|---|---|
| Opus 4.7 | 最强推理能力，适合复杂任务 |
| Sonnet 4.6 | 速度与能力均衡，默认选择 |
| Haiku 4.5 | 最快最省，适合简单任务 |

**环境变量**：

```bash
export ANTHROPIC_MODEL=claude-sonnet-4-6     # 设置默认模型
export ANTHROPIC_API_KEY=你的密钥             # 设置 API Key
export ANTHROPIC_BASE_URL=https://...        # 自定义 API 端点
```

## 16. 故障排查

**常用命令**：

- `/doctor` — 全面诊断：Node 版本、安装完整性、API 连通性、认证状态、配置合法性、MCP 连通性
- `/status` — 查看当前会话状态（模型、权限模式、会话 ID、上下文用量）
- 日志 — 设置环境变量 `CLAUDE_CODE_LOG_DIR` 指定日志目录，查看详细调试输出

**常见问题速查**：

| 问题 | 解决方式 |
|---|---|
| 认证失败 | `/login` 重新登录，或检查 `ANTHROPIC_API_KEY` |
| 工具被拒绝 | 检查 settings.json 的 deny 规则，或切换到更高权限模式 |
| 上下文已满 | `/compact` 压缩后继续，或 `/clear` 重新开始 |
| 响应变慢 | `/model` 切换更快的模型（如 Haiku） |
| MCP 服务器不可用 | `/doctor` 或 `claude mcp status` |
| `claude` 命令找不到 | `/terminal-setup` 配置终端集成，或重新安装 |

## 17. 实用技巧

- **写大型文档/复杂任务前先规划**：使用计划模式或让 Claude 先列出方案
- **给足上下文**：明确指出文件路径、错误信息、期望结果，让 Claude 少猜
- **利用项目记忆**：把构建命令、代码约定写进 CLAUDE.md，每次会话自动生效
- **控制 token 成本**：`/context` 查看用量，`/cost` 查看费用，`/compact` 定期压缩
- **放心改代码**：编辑前自动生成检查点，出错可回滚
- **善用子代理**：代码库调查用 Explore 代理，大型重构拆给多个通用代理并行做
- **`/statusline`**：配置状态栏，实时显示模型与 token 用量
- **自定义键位**：编辑 `~/.claude/keybindings.json` 绑定常用快捷键

## 18. 多代理工作流（Workflow 编排）

Workflow 是比单个子代理更进一步的**确定性多代理编排**：把一个任务写成脚本，由引擎按脚本并行派发多个子代理，等它们全部完成后统一汇总。与普通子代理的区别在于——脚本可审计、可断点续跑（resume）、可用 budget 控制总 token 消耗。

**工作流文件位置**：`.claude/workflows/*.js|.ts|.mjs`

**核心原语**（脚本中直接使用，无需 import）：

| 原语 | 作用 |
|---|---|
| `agent(name, task)` | 派发一个子代理执行任务 |
| `parallel(...)` | 并行执行多个子代理，等待全部完成 |
| `pipeline(...)` | 串行执行，前一个的输出传给后一个 |
| `phase(name, fn)` | 给一段逻辑命名分组，便于追踪进度 |
| `budget` | 控制总 token / 时长预算 |
| `return` | 脚本最终结果 |

**怎么操作**——三步建起一条并行审计流水线：

1. **建文件** `.claude/workflows/audit.js`：

```javascript
export const meta = {
  name: 'audit',
  description: '并行审计代码并汇总',
  phases: [{ title: '审计' }, { title: '汇总' }],
}

phase('审计')
const results = await parallel([
  () => agent('阅读 drivers/uart.cpp 和 kernel/timer.cpp，列出 bug', {label: '外设'}),
  () => agent('阅读 kernel/sched.cpp 和 arch/switch.S，列出 bug', {label: '调度'}),
])

phase('汇总')
return await agent('综合上面的结果，输出一份审计报告')
```

2. **运行**：在对话里说"运行 audit 工作流"，或终端执行 `claude -p "运行 audit 工作流"`
3. **监控**：`/workflows` 看实时进度；中途中断后重新运行会自动断点续跑（resume），已完成的子代理结果直接复用

特点：

- **确定性**：每个 agent 的结果记入 journal，可用 `resumeFromRunId` 从上次断点续跑，已完成的 agent 结果即时重放
- **并发上限**：默认 3，可调高（超过需确认）
- **可审计**：所有子代理输出、中间结果、最终结果全部落盘，方便复盘

## 19. Agent 团队（多代理协作）

当单个 agent 的上下文放不下整个任务时，可以组建 **agent 团队**：多个独立 agent 并行工作，通过消息互相通信，适合大型项目。

核心原语：

- `TeamCreate` — 创建团队
- `SendMessage(to: 成员名, message)` — 给指定成员发消息；`to: "*"` 广播给全员
- 成员之间消息自动送达，无需轮询；每个成员有独立上下文和完整工具权限

**怎么操作**：直接在对话里下令：

> "用**团队模式**：建一个队，把审计拆给两个成员并行做"

Claude 会创建团队（TeamCreate）、给成员发消息（SendMessage）、汇总结果。

**典型场景**：一个"队长"agent 把大型重构拆成若干子任务派给多个成员 agent，各自独立改代码，最后统一汇总验证。团队成员之间还可以互相审核对方的改动，形成开发-审查闭环。

## 20. 后台执行与并行开发

**把长任务放到后台**，不阻塞当前对话。

**怎么操作**：

> "把这个编译任务**放后台跑**，我们继续聊别的"

长命令和子代理都可后台化，完成时自动通知；随时 `/agents` 查看进度，`/tasks` 查看任务列表。

- Bash 命令加 `run_in_background: true`，立即返回任务 ID，完成时自动通知
- 子代理加 `run_in_background: true`，派发后继续干别的，用 `/agents` 查看进度
- 用 `SendMessage` 可以给**仍在运行**的 agent 追加指令（保留其全部上下文继续执行）

**git worktree 隔离开发**：派发子代理时指定 `isolation: "worktree"`，它会在临时 git worktree 中独立工作，互不污染主线；改动可整体合并回来。

## 21. 结构化任务管理

多步骤任务用 TaskCreate 建立任务清单，可设置依赖、分配负责人，子代理之间能"认领"任务：

```bash
TaskCreate  # 创建任务（subject / description）
TaskList    # 查看所有任务及状态
TaskGet     # 查看任务详情（含依赖关系）
TaskUpdate  # 更新状态 / 负责人 / 依赖
```

**怎么操作**：

> "给我建一个任务清单：1) 实现 MMU 映射 2) 写测试；第 2 个依赖第 1 个"

Claude 会用 TaskCreate/TaskUpdate 建立清单、设置依赖、标记状态。随时说"列一下任务"查看进度，说"标为完成"更新状态。

- 状态流转：`pending → in_progress → completed`（可删除）
- 依赖：`addBlockedBy: [1]` 表示任务 2 依赖任务 1；被阻塞的任务不能被认领
- **团队协作模式**：队长拆任务 → 成员 `TaskList` 找可认领的 → `TaskUpdate` 认领 → 完成后通知队长

## 22. 定时 / 循环任务

让 Claude 周期性自动执行某个提示词。

**怎么操作**：

- 对话式：> "**每 10 分钟**跑一次 `/status`"（Claude 用 CronCreate 创建）
- 循环式：直接输入 `/loop 10m /status`（每 10 分钟执行一次 `/status`）
- 查看 / 删除：`/cron-list`、`/cron-delete`

底层能力：**CronCreate / CronDelete / CronList** — 创建、删除、列出定时任务；**`/loop` 技能** — 循环执行。

适合：周期性检查 CI 状态、监控构建、轮询外部接口、"看护"一个 PR。

## 23. 自动记忆系统

除了 CLAUDE.md，Claude 还有一套**类型化自动记忆**：`~/.claude/projects/<项目路径>/memory/` 下的 markdown 文件，带 frontmatter（name / description / type）。

**怎么操作**——直接在对话里说：

> "**记住**：这个项目必须用 aarch64-linux-gnu 交叉编译"

自动存入类型化记忆文件，下次会话自动生效。手动查看 / 编辑：`/memory`；整理去重过期记忆：`/dream`。

**记忆类型**：

| 类型 | 用途 |
|---|---|
| `user` | 你的角色、偏好、知识背景 |
| `feedback` | 工作方式的反馈（含"为什么"），成功与失败都记 |
| `project` | 项目目标、进行中的事项、非代码可见的信息 |
| `reference` | 外部系统的入口（如 CI 面板、文档链接） |

- 每次会话自动加载相关记忆，跨会话生效
- 记忆会过期——若与当前代码冲突，以现在观察到的事实为准，并更新/删除记忆
- 注意：代码结构、git 历史等"读代码就能知道"的东西**不要**存进记忆

## 24. 实现验证契约

做**非平凡改动**（3+ 文件编辑、后端/API 改动、基础设施改动）后，Claude 会派一个独立 **verification** 代理做对抗性验证：跑构建、跑测试、检查输出，给出 `PASS / FAIL / PARTIAL` 结论与证据。

**怎么操作**：

> "我改完了，**验证一下**我的改动"

独立 verification 代理会跑构建、跑测试、检查输出，给出 PASS/FAIL 结论。也可以主动要求：`claude -p "验证 kernel/ 下的改动" --allowedTools "Bash(make)"`

- 验证代理独立验证，看不到你的自测结果，避免"自己查自己"
- FAIL 时带着具体发现回来，修复后重新验证直到 PASS

## 25. 技能开发进阶

自定义技能的 SKILL.md 除了正文说明，还可以用 frontmatter 声明元数据：

```markdown
---
name: my-skill
description: 在什么场景用这个技能（Claude 据此判断是否自动加载）
allowed-tools: Bash(git:*), Read
---
正文：具体怎么做……
```

**怎么操作**——建一个可复用的"构建内核"技能：

1. 建目录与文件 `.claude/skills/build-check/SKILL.md`：

```markdown
---
name: build-check
description: 构建并运行 myos 内核，检查 UART 输出。用户说"构建"或"跑一下"时使用。
---
运行 `make && make run`，观察串口输出并汇报。
```

2. 存好后在任何会话输入 `/build-check` 即可调用；`description` 写得越具体，任务匹配时自动加载越准

- 技能可声明 `allowed-tools` 白名单，限定使用时可以调用的工具
- 内置技能示例：`simplify`（审查改动并修复低效代码）、`loop`（循环执行）、`update-config`（改 settings.json）、`use-artifacts`（生成交付物 HTML 报告）
- 项目级技能放 `.claude/skills/`，用户级放 `~/.claude/skills/`

## 26. 子代理模型调优

派发子代理时可以**单独指定模型**，避免好钢用在刀刃上。

**怎么操作**：

> "这个代码库调研**用 haiku 跑**" / "这次的架构分析**用 opus 跑**"

会话级切换直接 `/model`。

```bash
agent: "model: haiku"   # 快速调研用 haiku，省 token
agent: "model: opus"    # 复杂推理 / 代码生成用 opus
```

建议搭配：代码库探索用 haiku、复杂重构与验证用 sonnet/opus、纯调研用 haiku。配合后台执行与验证契约，可以搭出一条"调研 → 实现 → 验证"的流水线。

**上手顺序建议**：先试零配置类（子代理 → 验证 → 任务清单 → 团队），再试建文件类（工作流 → 自定义技能 → 钩子）。这六项基本覆盖日常 90% 场景：**调研（Explore + haiku）→ 实现（子代理并行 + 任务清单）→ 验证（verification）→ 复用（工作流 / 技能）**。
