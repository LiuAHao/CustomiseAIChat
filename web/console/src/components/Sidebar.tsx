import type { AgentItem, UserInfo } from '../api'
import type { ViewKey } from '../App'
import { AppIcon, type AppIconKey } from './AppIcon'

type SidebarProps = {
  activeAgentId: string
  agents: AgentItem[]
  collapsed: boolean
  currentView: ViewKey
  onChangeAgent: (agentId: string) => void
  onChangeView: (view: ViewKey) => void
  onLogout: () => void
  onToggleCollapsed: () => void
  user: UserInfo | null
}

const navItems: Array<{ key: Exclude<ViewKey, 'agent-chat'>; label: string; icon: AppIconKey }> = [
  { key: 'create-agent', label: '新建 Agent', icon: 'create-agent' },
  { key: 'skills', label: '技能与应用', icon: 'skills' },
  { key: 'mcp', label: 'MCP 配置', icon: 'mcp' },
  { key: 'models', label: '模型配置', icon: 'models' },
]

export function Sidebar(props: SidebarProps) {
  const { activeAgentId, agents, collapsed, currentView, onChangeAgent, onChangeView, onLogout, onToggleCollapsed, user } = props
  const userInitial = (user?.name || user?.email || 'A').trim().slice(0, 1).toUpperCase()

  return (
    <aside className="sidebar-shell">
      <div className="sidebar-toprow">
        <div className="sidebar-topmark" aria-hidden="true">
          <div className="logo-cloud">
            <div className="cloud-shape" />
          </div>
        </div>

        <button className="sidebar-toggle" onClick={onToggleCollapsed} title={collapsed ? '展开侧栏' : '收起侧栏'} type="button">
          <span className="sidebar-action-icon">
            <AppIcon kind={collapsed ? 'expand' : 'collapse'} />
          </span>
          {!collapsed ? <span className="sidebar-action-label">收起</span> : null}
        </button>
      </div>

      <div className="sidebar-primary">
        {navItems.map((item) => (
          <button
            key={item.key}
            className={`sidebar-action ${currentView === item.key ? 'active' : ''}`}
            onClick={() => onChangeView(item.key)}
            title={item.label}
            type="button"
          >
            <span className="sidebar-action-icon">
              <AppIcon kind={item.icon} />
            </span>
            <span className="sidebar-action-label">{item.label}</span>
          </button>
        ))}
      </div>

      <div className="sidebar-section-title">Agents</div>

      <div className="agent-project-list">
        <div className="agent-card-list">
          {agents.length > 0 ? (
            agents.map((agent) => (
              <button
                key={agent.id}
                className={`agent-card ${activeAgentId === agent.id && currentView === 'agent-chat' ? 'active' : ''}`}
                onClick={() => onChangeAgent(agent.id)}
                title={agent.name}
                type="button"
              >
                <div className="agent-card-head">
                  <span className="agent-person-icon">
                    <AppIcon kind="person" />
                  </span>
                  <strong>{agent.name}</strong>
                </div>
                <span className="agent-card-note">{agent.description || '暂无描述'}</span>
              </button>
            ))
          ) : !collapsed ? (
            <div className="sidebar-empty">还没有 Agent，可以先在“新建 Agent”页面创建。</div>
          ) : null}
        </div>
      </div>

      <div className="sidebar-footer">
        <button
          className={`sidebar-action ${currentView === 'settings' ? 'active' : ''}`}
          onClick={() => onChangeView('settings')}
          title="系统设置"
          type="button"
        >
          <span className="sidebar-action-icon">
            <AppIcon kind="settings" />
          </span>
          <span className="sidebar-action-label">系统设置</span>
        </button>

        <button className="sidebar-account" onClick={onLogout} title="退出登录" type="button">
          <span className="sidebar-account-badge">{userInitial || 'A'}</span>
          <span className="sidebar-action-label">退出登录</span>
        </button>
      </div>
    </aside>
  )
}
