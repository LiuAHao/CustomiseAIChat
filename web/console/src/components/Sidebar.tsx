import type { ViewKey } from '../App'

type SidebarProps = {
  activeAgentId: string
  currentView: ViewKey
  onChangeAgent: (agentId: string) => void
  onChangeView: (view: ViewKey) => void
  onLogout: () => void
}

const navItems: Array<{ key: ViewKey; label: string; icon: string }> = [
  { key: 'dashboard', label: '新建 Agent', icon: 'edit' },
  { key: 'skills', label: '技能与应用', icon: 'grid' },
  { key: 'mcp', label: 'MCP 配置', icon: 'clock' },
  { key: 'models', label: '模型配置', icon: 'sparkle' },
]

const agents = [
  { id: 'invest', name: '智能投研助手', note: '负责研报抓取、因子分析与结论摘要' },
  { id: 'refactor', name: '代码重构专家', note: '负责模块拆分、样式清理与回归验证' },
  { id: 'connector', name: 'MCP 连接器', note: '负责工具接入、权限巡检与状态同步' },
  { id: 'quant', name: '策略实验 Agent', note: '待启动' },
  { id: 'research', name: '行业情报 Agent', note: '无运行中的 agent' },
]

export function Sidebar(props: SidebarProps) {
  const { activeAgentId, currentView, onChangeAgent, onChangeView, onLogout } = props

  return (
    <aside className="sidebar-shell">
      <div className="sidebar-topmark">
        <div className="logo-cloud">
          <div className="cloud-shape" />
        </div>
      </div>

      <div className="sidebar-primary">
        {navItems.map((item) => (
          <button
            key={item.key}
            className={`sidebar-action ${currentView === item.key ? 'active' : ''}`}
            onClick={() => onChangeView(item.key)}
            type="button"
          >
            <span className="sidebar-action-icon">{renderGlyph(item.icon)}</span>
            <span>{item.label}</span>
          </button>
        ))}
      </div>

      <div className="sidebar-section-title">Agents</div>

      <div className="agent-project-list">
        <div className="agent-card-list">
          {agents.map((agent) => (
            <button
              key={agent.id}
              className={`agent-card ${activeAgentId === agent.id ? 'active' : ''}`}
              onClick={() => {
                onChangeView('dashboard')
                onChangeAgent(agent.id)
              }}
              type="button"
            >
              <div className="agent-card-head">
                <span className="agent-person-icon">{renderGlyph('person')}</span>
                <strong>{agent.name}</strong>
              </div>
              <span className="agent-card-note">{agent.note}</span>
            </button>
          ))}
        </div>
      </div>

      <div className="sidebar-footer">
        <button className={`sidebar-action ${currentView === 'settings' ? 'active' : ''}`} onClick={() => onChangeView('settings')} type="button">
          <span className="sidebar-action-icon">{renderGlyph('settings')}</span>
          <span>系统设置</span>
        </button>

        <button className="sidebar-account" onClick={onLogout} type="button">
          <span className="sidebar-account-badge">A</span>
          <span>退出登录</span>
        </button>
      </div>
    </aside>
  )
}

function renderGlyph(kind: string) {
  if (kind === 'edit') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M4 16.5V20h3.5L18 9.5 14.5 6 4 16.5Z" />
        <path d="M13.5 7L17 10.5" />
      </svg>
    )
  }

  if (kind === 'grid') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <rect x="4" y="4" width="6" height="6" rx="2" />
        <rect x="14" y="4" width="6" height="6" rx="2" />
        <rect x="4" y="14" width="6" height="6" rx="2" />
        <rect x="14" y="14" width="6" height="6" rx="2" />
      </svg>
    )
  }

  if (kind === 'clock') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <circle cx="12" cy="12" r="8" />
        <path d="M12 8v5l3 2" />
      </svg>
    )
  }

  if (kind === 'sparkle') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M12 3.5 13.8 8l4.7 1.8-4.7 1.8L12 16l-1.8-4.4L5.5 9.8 10.2 8 12 3.5Z" />
        <path d="M18.5 4.5v3M20 6h-3M6 15.5v2.5M7.3 16.8H4.8" />
      </svg>
    )
  }

  if (kind === 'settings') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M10.5 3h3l.7 2.3 2.2.9 2.1-1.1 2.1 2.1-1.1 2.1.9 2.2L21 13.5v3l-2.3.7-.9 2.2 1.1 2.1-2.1 2.1-2.1-1.1-2.2.9-.7 2.3h-3l-.7-2.3-2.2-.9-2.1 1.1-2.1-2.1 1.1-2.1-.9-2.2L3 16.5v-3l2.3-.7.9-2.2-1.1-2.1 2.1-2.1 2.1 1.1 2.2-.9.7-2.3Z" />
        <circle cx="12" cy="12" r="3.2" />
      </svg>
    )
  }

  if (kind === 'person') {
    return (
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <circle cx="12" cy="8" r="3.2" />
        <path d="M6.5 19a5.5 5.5 0 0 1 11 0" />
      </svg>
    )
  }
  return <span />
}
