import { useEffect, useMemo, useState } from 'react'
import {
  createAgent,
  createModel,
  createSkill,
  createTool,
  fetchAgents,
  fetchMe,
  fetchModels,
  fetchSkills,
  fetchTools,
  login,
  type AgentItem,
  type CatalogResponse,
  type CreateAgentPayload,
  type CreateModelPayload,
  type CreateSkillPayload,
  type CreateToolPayload,
  type ModelItem,
  type SkillItem,
  type ToolItem,
  type UserInfo,
} from './api'
import { AgentDashboard } from './components/AgentDashboard'
import { AuthPage } from './components/AuthPage'
import { Sidebar } from './components/Sidebar'

export type ViewKey = 'create-agent' | 'agent-chat' | 'skills' | 'mcp' | 'models' | 'settings'

const TOKEN_KEY = 'go-agent-platform-token'
const DEV_EMAIL = 'admin@example.com'
const DEV_PASSWORD = 'ChangeMe123!'

export function App() {
  const [token, setToken] = useState<string | null>(() => window.localStorage.getItem(TOKEN_KEY))
  const [booting, setBooting] = useState(true)
  const [currentView, setCurrentView] = useState<ViewKey>('create-agent')
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false)
  const [user, setUser] = useState<UserInfo | null>(null)
  const [agents, setAgents] = useState<AgentItem[]>([])
  const [skillCatalog, setSkillCatalog] = useState<CatalogResponse<SkillItem>>({ platform_items: [], my_items: [] })
  const [toolCatalog, setToolCatalog] = useState<CatalogResponse<ToolItem>>({ platform_items: [], my_items: [] })
  const [models, setModels] = useState<ModelItem[]>([])
  const [activeAgentId, setActiveAgentId] = useState('')
  const [loading, setLoading] = useState(false)
  const [loadError, setLoadError] = useState('')

  useEffect(() => {
    let cancelled = false

    async function bootstrap() {
      if (token) {
        setBooting(false)
        return
      }

      try {
        const result = await login(DEV_EMAIL, DEV_PASSWORD)
        if (cancelled) return
        window.localStorage.setItem(TOKEN_KEY, result.token)
        setToken(result.token)
      } catch {
      } finally {
        if (!cancelled) {
          setBooting(false)
        }
      }
    }

    bootstrap()
    return () => {
      cancelled = true
    }
  }, [token])

  useEffect(() => {
    if (!token) return

    let cancelled = false

    async function loadAll() {
      setLoading(true)
      try {
        const [me, agentItems, skills, tools, modelItems] = await Promise.all([
          fetchMe(token),
          fetchAgents(token),
          fetchSkills(token),
          fetchTools(token),
          fetchModels(token),
        ])
        if (cancelled) return
        setUser(me.user)
        setAgents(agentItems)
        setSkillCatalog(skills)
        setToolCatalog(tools)
        setModels(modelItems)
        setActiveAgentId((current) => current || agentItems[0]?.id || '')
        setLoadError('')
      } catch (error) {
        if (cancelled) return
        setLoadError(error instanceof Error ? error.message : '读取平台数据失败')
      } finally {
        if (!cancelled) {
          setLoading(false)
        }
      }
    }

    loadAll()
    return () => {
      cancelled = true
    }
  }, [token])

  const activeAgent = useMemo(
    () => agents.find((item) => item.id === activeAgentId) ?? null,
    [activeAgentId, agents],
  )

  async function refreshAll() {
    if (!token) return
    const [agentItems, skills, tools, modelItems] = await Promise.all([
      fetchAgents(token),
      fetchSkills(token),
      fetchTools(token),
      fetchModels(token),
    ])
    setAgents(agentItems)
    setSkillCatalog(skills)
    setToolCatalog(tools)
    setModels(modelItems)
    setActiveAgentId((current) => {
      if (current && agentItems.some((item) => item.id === current)) return current
      return agentItems[0]?.id ?? ''
    })
  }

  async function handleLogin(email: string, password: string) {
    const result = await login(email, password)
    window.localStorage.setItem(TOKEN_KEY, result.token)
    setToken(result.token)
  }

  function handleLogout() {
    window.localStorage.removeItem(TOKEN_KEY)
    setToken(null)
    setUser(null)
    setAgents([])
    setSkillCatalog({ platform_items: [], my_items: [] })
    setToolCatalog({ platform_items: [], my_items: [] })
    setModels([])
    setActiveAgentId('')
    setCurrentView('create-agent')
  }

  async function handleCreateAgent(payload: CreateAgentPayload) {
    if (!token) throw new Error('未登录')
    const created = await createAgent(token, payload)
    await refreshAll()
    setActiveAgentId(created.id)
    setCurrentView('agent-chat')
  }

  async function handleCreateSkill(payload: CreateSkillPayload) {
    if (!token) throw new Error('未登录')
    await createSkill(token, payload)
    await refreshAll()
  }

  async function handleCreateTool(payload: CreateToolPayload) {
    if (!token) throw new Error('未登录')
    await createTool(token, payload)
    await refreshAll()
  }

  async function handleCreateModel(payload: CreateModelPayload) {
    if (!token) throw new Error('未登录')
    await createModel(token, payload)
    await refreshAll()
  }

  function handleSelectAgent(agentID: string) {
    setActiveAgentId(agentID)
    setCurrentView('agent-chat')
  }

  if (booting) {
    return <div className="boot-screen">正在连接平台...</div>
  }

  if (!token) {
    return <AuthPage onLogin={handleLogin} />
  }

  return (
    <div className={`desktop-shell ${sidebarCollapsed ? 'sidebar-collapsed' : ''}`}>
      <Sidebar
        activeAgentId={activeAgentId}
        agents={agents}
        collapsed={sidebarCollapsed}
        currentView={currentView}
        onChangeAgent={handleSelectAgent}
        onChangeView={setCurrentView}
        onLogout={handleLogout}
        onToggleCollapsed={() => setSidebarCollapsed((value) => !value)}
        user={user}
      />

      <main className="desktop-main">
        <header className="desktop-menubar" />
        <section className="desktop-content">
          <AgentDashboard
            activeAgent={activeAgent}
            agents={agents}
            currentView={currentView}
            error={loadError}
            loading={loading}
            models={models}
            onCreateAgent={handleCreateAgent}
            onCreateModel={handleCreateModel}
            onCreateSkill={handleCreateSkill}
            onCreateTool={handleCreateTool}
            onRefresh={refreshAll}
            skillCatalog={skillCatalog}
            token={token}
            toolCatalog={toolCatalog}
            user={user}
          />
        </section>
      </main>
    </div>
  )
}
