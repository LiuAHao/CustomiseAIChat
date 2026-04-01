import type { ViewKey } from '../App'

type DashboardProps = {
  activeAgentId: string
  currentView: ViewKey
}

const agentContent = {
  invest: {
    title: '开始构建',
    workspace: 'go-agent-platform',
    subtitle: '围绕投研、多 agent 编排与工具接入开始你的下一次执行。',
    cards: [
      { icon: 'mcp', title: '绑定 MCP 服务', desc: '连接搜索、数据库、文件系统与浏览器能力。' },
      { icon: 'skill', title: '配置新技能', desc: '为 Agent 装载技能并设定调用边界。' },
      { icon: 'deploy', title: '发布运行环境', desc: '将 Agent 封装成 API 服务或后台任务。' },
    ],
  },
  refactor: {
    title: '重构控制台',
    workspace: 'web/console',
    subtitle: '当前 Agent 聚焦 UI 重建、信息架构收束和桌面控制台交互。',
    cards: [
      { icon: 'code', title: '拆分组件结构', desc: '按 Sidebar、Dashboard、Auth 页面组织文件。' },
      { icon: 'skill', title: '对齐技能入口', desc: '让技能页与 Agent 主流程保持同级。' },
      { icon: 'deploy', title: '构建验证', desc: '输出可运行页面并验证打包流程。' },
    ],
  },
  connector: {
    title: '连接工具域',
    workspace: 'internal/mcp',
    subtitle: '聚焦 MCP server 配置、健康探针和外部资源可见性。',
    cards: [
      { icon: 'mcp', title: '配置服务注册', desc: '新增和编辑 MCP 服务元数据与状态。' },
      { icon: 'code', title: '管理权限', desc: '区分默认权限、高权审批和只读访问。' },
      { icon: 'deploy', title: '监控连接', desc: '实时查看心跳、失败率和降级策略。' },
    ],
  },
  quant: {
    title: '启动策略实验',
    workspace: 'quant-lab',
    subtitle: '从策略假设、回测执行到报告生成形成闭环。',
    cards: [
      { icon: 'code', title: '创建回测任务', desc: '指定数据区间、手续费和风控约束。' },
      { icon: 'skill', title: '挂载研究技能', desc: '注入因子解释、结果总结和报告撰写。' },
      { icon: 'deploy', title: '生成实验流水', desc: '把实验记录回写到项目知识库。' },
    ],
  },
  research: {
    title: '开启行业研究',
    workspace: 'research-hub',
    subtitle: '从问题定义、信息采集到多源汇总，形成长期研究 Agent。',
    cards: [
      { icon: 'mcp', title: '接入信息源', desc: '挂载网页搜索、文档读取和数据库查询。' },
      { icon: 'skill', title: '定义研究技能', desc: '拆成搜集、筛选、总结、校验四层。' },
      { icon: 'deploy', title: '生成自动化', desc: '设为定时巡检与日报输出。' },
    ],
  },
} as const

export function AgentDashboard({ activeAgentId, currentView }: DashboardProps) {
  if (currentView === 'skills') {
    return <FeatureWorkspace title="技能与应用" subtitle="管理技能包、调用链路和作用范围，让 agent 能力以可治理方式装载。" type="skill" />
  }

  if (currentView === 'mcp') {
    return <FeatureWorkspace title="MCP 配置" subtitle="连接外部服务、查看健康度，并管理默认权限与降级策略。" type="mcp" />
  }

  if (currentView === 'models') {
    return <FeatureWorkspace title="模型配置" subtitle="配置默认模型、推理档位、回退链路与不同 agent 的模型分配策略。" type="models" />
  }

  if (currentView === 'settings') {
    return <FeatureWorkspace title="系统设置" subtitle="统一管理语言、工作区偏好、通知方式和平台默认行为。" type="settings" />
  }

  const content = agentContent[activeAgentId as keyof typeof agentContent] ?? agentContent.invest

  return (
    <div className="dashboard-shell">
      <div className="dashboard-center">
        <div className="hero-mark">{renderIcon('spark')}</div>
        <h1>{content.title}</h1>
        <p className="dashboard-intro">{content.subtitle}</p>
      </div>

      <div className="quick-card-strip">
        {content.cards.map((card) => (
          <button key={card.title} className="quick-card" type="button">
            <div className={`quick-card-icon ${card.icon}`}>{renderIcon(card.icon)}</div>
            <strong>{card.title}</strong>
            <span>{card.desc}</span>
          </button>
        ))}
      </div>

      <div className="composer-shell">
        <div className="composer-input-area">
          <textarea
            defaultValue=""
            placeholder="询问 Agent 关于任何事，@ 添加文件，/ 调用命令，$ 触发技能，# 选择 MCP 连接器"
          />
        </div>

        <div className="composer-footer">
          <div className="composer-left">
            <button className="composer-tool" type="button">
              <span className="composer-tool-icon">＋</span>
            </button>
            <button className="composer-select" type="button">
              <span className="composer-select-icon">{renderIcon('bolt')}</span>
              <span>GPT-5.4</span>
              <span>▾</span>
            </button>
            <button className="composer-select" type="button">
              <span>中</span>
              <span>▾</span>
            </button>
          </div>

          <div className="composer-right">
            <button className="composer-voice" type="button">
              {renderIcon('mic')}
            </button>
            <button className="composer-send" type="button">
              ↑
            </button>
          </div>
        </div>
      </div>

      <div className="dashboard-statusbar">
        <div className="status-item">
          <span>{renderIcon('screen')}</span>
          <span>本地</span>
          <span>▾</span>
        </div>
        <div className="status-item">
          <span>{renderIcon('shield')}</span>
          <span>默认权限</span>
          <span>▾</span>
        </div>
      </div>
    </div>
  )
}

type FeatureWorkspaceProps = {
  title: string
  subtitle: string
  type: 'skill' | 'mcp' | 'models' | 'settings'
}

function FeatureWorkspace({ title, subtitle, type }: FeatureWorkspaceProps) {
  const summaryCards =
    type === 'skill'
      ? [
          ['已装载技能', '18', '按 agent 角色自动继承'],
          ['运行中技能链', '6', '规划、检索、执行、审计'],
          ['待发布变更', '3', '有新版本等待灰度'],
        ]
      : type === 'mcp'
        ? [
          ['在线连接', '9', '全部可被 agent 调用'],
          ['待授权服务', '2', 'GitHub 与企业知识库'],
          ['平均延迟', '184ms', '近一小时健康探针'],
        ]
        : type === 'models'
          ? [
            ['默认主模型', 'GPT-5.4', '面向复杂规划与构建'],
            ['回退模型', 'GPT-5.4-mini', '成本优先的执行档'],
            ['推理档位', 'medium', '平台默认推理深度'],
          ]
          : [
            ['界面语言', '中文', '注释与文案默认中文'],
            ['默认权限', '工作区读写', '高风险命令仍需审批'],
            ['通知通道', '站内提醒', '失败和审批会同步提示'],
          ]

  const detailCards =
    type === 'skill'
      ? [
          ['技能包编排', '为不同 agent 绑定不同技能集合，支持启停、继承和版本说明。'],
          ['触发规则', '定义 `$技能名`、自动匹配和按任务路由的触发方式。'],
          ['发布与灰度', '先在测试 agent 上启用，再逐步扩散到生产 agent。'],
        ]
      : type === 'mcp'
        ? [
          ['服务注册', '配置 MCP 名称、地址、说明和能力标签，统一纳管。'],
          ['权限与配额', '为不同 agent 指定可调用范围，并限制高成本工具的并发。'],
          ['健康巡检', '记录延迟、失败率和最近错误，便于快速定位接入问题。'],
        ]
        : type === 'models'
          ? [
            ['模型路由', '按任务类型选择模型，例如规划、编码、审核各用不同主模型。'],
            ['成本控制', '可设置 token 上限、自动回退模型和超预算提醒。'],
            ['Agent 覆盖', '给特定 agent 单独配置模型与推理档位，不影响平台默认值。'],
          ]
          : [
            ['平台偏好', '配置语言、默认工作区、默认分支和命令输入习惯。'],
            ['审批策略', '定义哪些命令直接执行，哪些命令必须人工确认。'],
            ['通知设置', '控制站内提醒、告警提示和执行完成后的通知方式。'],
          ]

  const formRows =
    type === 'skill'
      ? [
          ['默认技能集', '规划技能 + 前端构建技能 + 安全审计技能'],
          ['触发方式', '手动触发 + 任务路由自动触发'],
          ['最近变更', '前端构建技能 v1.3 已发布到测试环境'],
        ]
      : type === 'mcp'
        ? [
          ['默认连接策略', '优先本地工具，失败后回退远程服务'],
          ['高权限审批', 'GitHub 写操作、浏览器下载、文件删除需要审批'],
          ['最近巡检', 'Playwright 延迟偏高，已自动切换到降载模式'],
        ]
        : type === 'models'
          ? [
            ['平台默认模型', 'GPT-5.4'],
            ['回退模型', 'GPT-5.4-mini'],
            ['默认推理档位', 'medium'],
          ]
          : [
            ['默认语言', '中文'],
            ['默认工作区', 'go-agent-platform'],
            ['默认权限策略', '读写工作区，敏感命令审批'],
          ]

  return (
    <div className="capability-panel">
      <div className="capability-hero">
        <div className="hero-mark">{renderIcon(type === 'skill' ? 'skill' : type === 'mcp' ? 'mcp' : type === 'models' ? 'model' : 'settings')}</div>
        <h1>{title}</h1>
        <p>{subtitle}</p>
      </div>

      <div className="feature-summary-grid">
        {summaryCards.map(([label, value, hint]) => (
          <div key={label} className="feature-stat-card">
            <span>{label}</span>
            <strong>{value}</strong>
            <small>{hint}</small>
          </div>
        ))}
      </div>

      <div className="feature-detail-grid">
        {detailCards.map(([titleText, desc]) => (
          <div key={titleText} className="capability-card">
            <strong>{titleText}</strong>
            <span>{desc}</span>
          </div>
        ))}
      </div>

      <div className="feature-config-panel">
        <div className="feature-config-head">
          <div>
            <h3>当前配置</h3>
            <p>保留现有视觉风格，只增强信息密度和可操作性。</p>
          </div>
          <button type="button">保存配置</button>
        </div>

        <div className="feature-config-grid">
          {formRows.map(([label, value]) => (
            <label key={label} className="feature-field">
              <span>{label}</span>
              <input defaultValue={value} type="text" />
            </label>
          ))}
        </div>
      </div>
    </div>
  )
}

function renderIcon(kind: string) {
  if (kind === 'cube') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M12 3 5 7v10l7 4 7-4V7l-7-4Z" />
        <path d="m5 7 7 4 7-4" />
        <path d="M12 11v10" />
      </svg>
    )
  }

  if (kind === 'scope') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <circle cx="12" cy="12" r="6.5" />
        <path d="M12 3v4M12 17v4M3 12h4M17 12h4" />
      </svg>
    )
  }

  if (kind === 'terminal') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <rect x="4" y="5" width="16" height="14" rx="3" />
        <path d="m8 10 2 2-2 2M12.5 14H16" />
      </svg>
    )
  }

  if (kind === 'clipboard') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <rect x="7" y="5" width="10" height="14" rx="2.5" />
        <path d="M9 5.5h6V4a1 1 0 0 0-1-1h-4a1 1 0 0 0-1 1v1.5Z" />
      </svg>
    )
  }

  if (kind === 'spark') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M9.4 4.2c.9-1.6 3.3-1.6 4.2 0l.6 1.1a2.8 2.8 0 0 0 1.7 1.3l1.2.3c1.8.5 2.5 2.7 1.4 4.1l-.8 1a2.8 2.8 0 0 0-.5 2.1l.2 1.2c.3 1.8-1.5 3.1-3.2 2.4l-1.1-.5a2.8 2.8 0 0 0-2.2 0l-1.1.5c-1.7.7-3.5-.6-3.2-2.4l.2-1.2a2.8 2.8 0 0 0-.5-2.1l-.8-1c-1.1-1.4-.4-3.6 1.4-4.1l1.2-.3a2.8 2.8 0 0 0 1.7-1.3l.6-1.1Z" />
        <path d="m10 9 2 3-2 3M14 15h1.6" />
      </svg>
    )
  }

  if (kind === 'mcp') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <rect x="4" y="6" width="6" height="12" rx="2" />
        <rect x="14" y="6" width="6" height="12" rx="2" />
        <path d="M10 12h4" />
      </svg>
    )
  }

  if (kind === 'model') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <rect x="5" y="6" width="14" height="12" rx="3" />
        <path d="M9 3v3M15 3v3M9 18v3M15 18v3M3 10h2M3 14h2M19 10h2M19 14h2" />
        <path d="M9.5 10.5h5M9.5 13.5h5" />
      </svg>
    )
  }

  if (kind === 'skill') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M13 3 5 14h5l-1 7 8-11h-5l1-7Z" />
      </svg>
    )
  }

  if (kind === 'deploy') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M12 4v10" />
        <path d="m8 8 4-4 4 4" />
        <rect x="4" y="14" width="16" height="6" rx="2.5" />
      </svg>
    )
  }

  if (kind === 'code') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="m9 8-4 4 4 4M15 8l4 4-4 4M13 5l-2 14" />
      </svg>
    )
  }

  if (kind === 'bolt') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M13 2 6 13h5l-1 9 8-12h-5l0-8Z" />
      </svg>
    )
  }

  if (kind === 'mic') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <rect x="9" y="4" width="6" height="10" rx="3" />
        <path d="M7 11a5 5 0 1 0 10 0M12 16v4M9 20h6" />
      </svg>
    )
  }

  if (kind === 'screen') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <rect x="4" y="5" width="16" height="11" rx="2.5" />
        <path d="M9 19h6" />
      </svg>
    )
  }

  if (kind === 'shield') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M12 3 5 6v5c0 5 3.2 8.4 7 10 3.8-1.6 7-5 7-10V6l-7-3Z" />
      </svg>
    )
  }

  if (kind === 'branch') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <circle cx="7" cy="6" r="2.5" />
        <circle cx="17" cy="18" r="2.5" />
        <path d="M7 8.5v6a3.5 3.5 0 0 0 3.5 3.5H14" />
        <path d="M14 6h3" />
      </svg>
    )
  }

  if (kind === 'settings') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M10.5 3h3l.7 2.2 2.2.9 2.1-1 2.1 2.1-1 2.1.9 2.2 2.2.7v3l-2.2.7-.9 2.2 1 2.1-2.1 2.1-2.1-1-2.2.9-.7 2.2h-3l-.7-2.2-2.2-.9-2.1 1-2.1-2.1 1-2.1-.9-2.2L3 13.5v-3l2.2-.7.9-2.2-1-2.1 2.1-2.1 2.1 1 2.2-.9.7-2.2Z" />
        <circle cx="12" cy="12" r="3.2" />
      </svg>
    )
  }

  return <span />
}
