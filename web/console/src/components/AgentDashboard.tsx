import { useEffect, useMemo, useState, type ReactNode } from 'react'
import {
  executeTask,
  fetchMessages,
  fetchSessions,
  installSkill,
  installTool,
  uninstallSkill,
  uninstallTool,
  type AgentItem,
  type CatalogResponse,
  type CreateAgentPayload,
  type CreateModelPayload,
  type CreateSkillPayload,
  type CreateToolPayload,
  type MessageItem,
  type ModelItem,
  type SessionItem,
  type SkillItem,
  type ToolItem,
  type UserInfo,
} from '../api'
import type { ViewKey } from '../App'
import { AppIcon, type AppIconKey } from './AppIcon'

type DashboardProps = {
  activeAgent: AgentItem | null
  agents: AgentItem[]
  currentView: ViewKey
  error: string
  loading: boolean
  models: ModelItem[]
  onCreateAgent: (payload: CreateAgentPayload) => Promise<void>
  onCreateModel: (payload: CreateModelPayload) => Promise<void>
  onCreateSkill: (payload: CreateSkillPayload) => Promise<void>
  onCreateTool: (payload: CreateToolPayload) => Promise<void>
  onRefresh: () => Promise<void>
  skillCatalog: CatalogResponse<SkillItem>
  token: string
  toolCatalog: CatalogResponse<ToolItem>
  user: UserInfo | null
}

export function AgentDashboard(props: DashboardProps) {
  switch (props.currentView) {
    case 'skills':
      return <SkillsPage {...props} />
    case 'mcp':
      return <McpPage {...props} />
    case 'models':
      return <ModelsPage {...props} />
    case 'settings':
      return <SettingsPage user={props.user} />
    case 'agent-chat':
      return <AgentChatPage {...props} />
    default:
      return <CreateAgentPage {...props} />
  }
}

function CreateAgentPage(props: DashboardProps) {
  const { error, loading, models, onCreateAgent, skillCatalog, toolCatalog } = props
  const [name, setName] = useState('')
  const [description, setDescription] = useState('')
  const [systemPrompt, setSystemPrompt] = useState('')
  const [model, setModel] = useState('')
  const [selectedSkills, setSelectedSkills] = useState<string[]>([])
  const [selectedTools, setSelectedTools] = useState<string[]>([])
  const [saving, setSaving] = useState(false)
  const [message, setMessage] = useState('')

  useEffect(() => {
    if (!model && models.length > 0) {
      setModel(models.find((item) => item.is_default)?.model_key ?? models[0]?.model_key ?? '')
    }
  }, [model, models])

  async function submit() {
    if (!name.trim()) return setMessage('请先填写 Agent 名称。')
    if (!model.trim()) return setMessage('请先选择模型。')
    setSaving(true)
    setMessage('')
    try {
      await onCreateAgent({
        name: name.trim(),
        description: description.trim(),
        system_prompt: systemPrompt.trim(),
        model: model.trim(),
        skill_policy: selectedSkills,
        tool_policy: selectedTools,
        runtime_policy: 'default',
      })
      setName('')
      setDescription('')
      setSystemPrompt('')
      setSelectedSkills([])
      setSelectedTools([])
      setMessage('Agent 已创建。')
    } catch (currentError) {
      setMessage(currentError instanceof Error ? currentError.message : '创建 Agent 失败')
    } finally {
      setSaving(false)
    }
  }

  return (
    <div className="page-shell">
      <PageHero icon="create-agent" title="新建 Agent" subtitle="在创建时绑定模型、Skill 与 MCP，后续由 Agent 按配置调用能力。" />
      <div className="editor-panel">
        <Section title="基础信息" hint={loading ? '正在读取资源...' : '只展示后端已支持的真实字段。'}>
          <div className="form-grid form-grid-two">
            <label className="field">
              <span>Agent 名称</span>
              <input value={name} onChange={(e) => setName(e.target.value)} placeholder="例如：行业情报 Agent" type="text" />
            </label>
            <label className="field">
              <span>模型</span>
              <select value={model} onChange={(e) => setModel(e.target.value)}>
                <option value="">请选择模型</option>
                {models.map((item) => (
                  <option key={item.id} value={item.model_key}>
                    {item.name} / {item.model_key}
                  </option>
                ))}
              </select>
            </label>
          </div>
          <div className="form-grid form-grid-two">
            <label className="field">
              <span>Agent 描述</span>
              <textarea value={description} onChange={(e) => setDescription(e.target.value)} placeholder="说明这个 Agent 的职责和边界。" rows={4} />
            </label>
            <label className="field">
              <span>System Prompt</span>
              <textarea value={systemPrompt} onChange={(e) => setSystemPrompt(e.target.value)} placeholder="可选，对应后端 system_prompt 字段。" rows={4} />
            </label>
          </div>
        </Section>

        <Section title="能力装配" hint="这里只显示“我的 Skill”和“我的 MCP”，平台资源请先安装到个人列表。">
          <div className="form-grid form-grid-two">
            <SelectionPanel
              emptyText="当前没有可用 Skill，可先到“技能与应用”页面安装平台 Skill 或创建个人 Skill。"
              items={skillCatalog.my_items}
              selected={selectedSkills}
              title="我的 Skill"
              onToggle={(id) => toggleSelection(id, selectedSkills, setSelectedSkills)}
              renderMeta={(item) => item.description || item.entry || '暂无描述'}
            />
            <SelectionPanel
              emptyText="当前没有可用 MCP，可先到“MCP 配置”页面安装平台 MCP 或创建个人 MCP。"
              items={toolCatalog.my_items}
              selected={selectedTools}
              title="我的 MCP"
              onToggle={(id) => toggleSelection(id, selectedTools, setSelectedTools)}
              renderMeta={(item) => item.description || '暂无描述'}
            />
          </div>
        </Section>

        {error ? <div className="notice error">{error}</div> : null}
        {message ? <div className={`notice ${message.includes('已创建') ? 'success' : 'error'}`}>{message}</div> : null}
        <div className="panel-actions">
          <button className="primary-button" disabled={saving} onClick={submit} type="button">
            {saving ? '创建中...' : '创建 Agent'}
          </button>
        </div>
      </div>
    </div>
  )
}

function SkillsPage(props: DashboardProps) {
  const { error, onCreateSkill, onRefresh, skillCatalog, token } = props
  const [name, setName] = useState('')
  const [slug, setSlug] = useState('')
  const [description, setDescription] = useState('')
  const [version, setVersion] = useState('0.1.0')
  const [entry, setEntry] = useState('')
  const [scope, setScope] = useState<'platform' | 'personal'>('personal')
  const [saving, setSaving] = useState(false)
  const [message, setMessage] = useState('')
  const myIDs = useMemo(() => new Set(skillCatalog.my_items.map((item) => item.id)), [skillCatalog.my_items])

  async function submit() {
    if (!name.trim()) return setMessage('请先填写 Skill 名称。')
    setSaving(true)
    setMessage('')
    try {
      await onCreateSkill({ name: name.trim(), slug: slug.trim(), scope, description: description.trim(), version: version.trim() || '0.1.0', entry: entry.trim(), schema: {}, config: {} })
      setName('')
      setSlug('')
      setDescription('')
      setVersion('0.1.0')
      setEntry('')
      setScope('personal')
      setMessage('Skill 已创建。')
    } catch (currentError) {
      setMessage(currentError instanceof Error ? currentError.message : '创建 Skill 失败')
    } finally {
      setSaving(false)
    }
  }

  async function handleInstall(item: SkillItem, installed: boolean) {
    setMessage('')
    try {
      if (installed) await uninstallSkill(token, item.id)
      else await installSkill(token, item.id)
      await onRefresh()
    } catch (currentError) {
      setMessage(currentError instanceof Error ? currentError.message : '更新 Skill 失败')
    }
  }

  return (
    <div className="page-shell">
      <PageHero icon="skills" title="技能与应用" subtitle="平台统一提供可复用 Skill，用户也可维护自己的 Skill，并装配到 Agent。" />
      <div className="editor-panel">
        <CatalogSection
          title="平台 Skill"
          items={skillCatalog.platform_items}
          emptyTitle="当前还没有平台 Skill"
          emptyText="平台 Skill 创建后会出现在这里，用户可以安装到“我的 Skill”。"
          renderItem={(item) => (
            <CatalogItem
              title={item.name}
              secondary={`${item.version || '未标记版本'}${item.entry ? ` · ${item.entry}` : ''}`}
              actionLabel={myIDs.has(item.id) ? '移除' : '安装'}
              onAction={() => handleInstall(item, myIDs.has(item.id))}
            >
              {item.description || '暂无描述'}
            </CatalogItem>
          )}
        />
        <CatalogSection
          title="我的 Skill"
          items={skillCatalog.my_items}
          emptyTitle="当前还没有我的 Skill"
          emptyText="“我的 Skill”包含你自己创建的 Skill，以及你从平台安装下来的 Skill。"
          renderItem={(item) => (
            <CatalogItem title={item.name} secondary={item.scope === 'platform' ? '已安装的平台 Skill' : `私有 Skill${item.entry ? ` · ${item.entry}` : ''}`}>
              {item.description || '暂无描述'}
            </CatalogItem>
          )}
        />
        <ResourceForm
          error={error}
          fields={
            <>
              <div className="form-grid form-grid-three">
                <label className="field">
                  <span>Skill 名称</span>
                  <input value={name} onChange={(e) => setName(e.target.value)} placeholder="例如：行业研究" type="text" />
                </label>
                <label className="field">
                  <span>Slug</span>
                  <input value={slug} onChange={(e) => setSlug(e.target.value)} placeholder="例如：industry-research" type="text" />
                </label>
                <label className="field">
                  <span>发布范围</span>
                  <select value={scope} onChange={(e) => setScope(e.target.value as 'platform' | 'personal')}>
                    <option value="personal">我的 Skill</option>
                    <option value="platform">平台 Skill</option>
                  </select>
                </label>
              </div>
              <div className="form-grid form-grid-three">
                <label className="field">
                  <span>版本</span>
                  <input value={version} onChange={(e) => setVersion(e.target.value)} placeholder="例如：0.1.0" type="text" />
                </label>
                <label className="field form-grid-span-2">
                  <span>入口标识</span>
                  <input value={entry} onChange={(e) => setEntry(e.target.value)} placeholder="例如：builtin/research" type="text" />
                </label>
              </div>
              <label className="field">
                <span>描述</span>
                <textarea value={description} onChange={(e) => setDescription(e.target.value)} placeholder="描述这个 Skill 的能力边界。" rows={4} />
              </label>
            </>
          }
          hint="仅提交后端已支持字段。若创建平台 Skill，当前账号需要具备平台管理权限。"
          message={message}
          saving={saving}
          title="创建 Skill"
          onSubmit={submit}
          submitText="创建 Skill"
        />
      </div>
    </div>
  )
}

function McpPage(props: DashboardProps) {
  const { error, onCreateTool, onRefresh, token, toolCatalog } = props
  const [name, setName] = useState('')
  const [description, setDescription] = useState('')
  const [scope, setScope] = useState<'platform' | 'personal'>('personal')
  const [requiresApproval, setRequiresApproval] = useState(false)
  const [schemaText, setSchemaText] = useState('{}')
  const [configText, setConfigText] = useState('{}')
  const [saving, setSaving] = useState(false)
  const [message, setMessage] = useState('')
  const myIDs = useMemo(() => new Set(toolCatalog.my_items.map((item) => item.id)), [toolCatalog.my_items])

  async function submit() {
    if (!name.trim()) return setMessage('请先填写 MCP 名称。')
    setSaving(true)
    setMessage('')
    try {
      await onCreateTool({ name: name.trim(), scope, description: description.trim(), schema: JSON.parse(schemaText), config: JSON.parse(configText), requires_approval: requiresApproval })
      setName('')
      setDescription('')
      setScope('personal')
      setRequiresApproval(false)
      setSchemaText('{}')
      setConfigText('{}')
      setMessage('MCP 已创建。')
    } catch (currentError) {
      setMessage(currentError instanceof Error ? currentError.message : '创建 MCP 失败')
    } finally {
      setSaving(false)
    }
  }

  async function handleInstall(item: ToolItem, installed: boolean) {
    setMessage('')
    try {
      if (installed) await uninstallTool(token, item.id)
      else await installTool(token, item.id)
      await onRefresh()
    } catch (currentError) {
      setMessage(currentError instanceof Error ? currentError.message : '更新 MCP 失败')
    }
  }

  return (
    <div className="page-shell">
      <PageHero icon="mcp" title="MCP 配置" subtitle="平台统一提供 MCP，用户可选择安装，也可维护自己的私有 MCP。" />
      <div className="editor-panel">
        <CatalogSection
          title="平台 MCP"
          items={toolCatalog.platform_items}
          emptyTitle="当前还没有平台 MCP"
          emptyText="平台 MCP 创建后会出现在这里，用户可以安装到“我的 MCP”。"
          renderItem={(item) => (
            <CatalogItem
              title={item.name}
              secondary={item.requires_approval ? '需要审批' : '直接调用'}
              actionLabel={myIDs.has(item.id) ? '移除' : '安装'}
              onAction={() => handleInstall(item, myIDs.has(item.id))}
            >
              {item.description || '暂无描述'}
            </CatalogItem>
          )}
        />
        <CatalogSection
          title="我的 MCP"
          items={toolCatalog.my_items}
          emptyTitle="当前还没有我的 MCP"
          emptyText="“我的 MCP”包含你自己创建的 MCP，以及你从平台安装下来的 MCP。"
          renderItem={(item) => (
            <CatalogItem title={item.name} secondary={item.scope === 'platform' ? '已安装的平台 MCP' : item.requires_approval ? '私有 MCP · 需要审批' : '私有 MCP'}>
              {item.description || '暂无描述'}
            </CatalogItem>
          )}
        />
        <ResourceForm
          error={error}
          fields={
            <>
              <div className="form-grid form-grid-three">
                <label className="field">
                  <span>MCP 名称</span>
                  <input value={name} onChange={(e) => setName(e.target.value)} placeholder="例如：browser-search" type="text" />
                </label>
                <label className="field">
                  <span>发布范围</span>
                  <select value={scope} onChange={(e) => setScope(e.target.value as 'platform' | 'personal')}>
                    <option value="personal">我的 MCP</option>
                    <option value="platform">平台 MCP</option>
                  </select>
                </label>
                <label className="toggle-field">
                  <span>调用审批</span>
                  <div className="toggle-inline">
                    <input checked={requiresApproval} onChange={(e) => setRequiresApproval(e.target.checked)} type="checkbox" />
                    <strong>{requiresApproval ? '需要审批' : '直接调用'}</strong>
                  </div>
                </label>
              </div>
              <label className="field">
                <span>描述</span>
                <textarea value={description} onChange={(e) => setDescription(e.target.value)} placeholder="描述这个 MCP 暴露的能力和边界。" rows={3} />
              </label>
              <div className="form-grid form-grid-two">
                <label className="field">
                  <span>Schema JSON</span>
                  <textarea value={schemaText} onChange={(e) => setSchemaText(e.target.value)} rows={8} />
                </label>
                <label className="field">
                  <span>Config JSON</span>
                  <textarea value={configText} onChange={(e) => setConfigText(e.target.value)} rows={8} />
                </label>
              </div>
            </>
          }
          hint="只暴露后端已有的 name、description、scope、schema、config、requires_approval 字段。"
          message={message}
          saving={saving}
          title="创建 MCP"
          onSubmit={submit}
          submitText="创建 MCP"
        />
      </div>
    </div>
  )
}

function ModelsPage(props: DashboardProps) {
  const { error, models, onCreateModel } = props
  const [name, setName] = useState('')
  const [provider, setProvider] = useState('')
  const [modelKey, setModelKey] = useState('')
  const [description, setDescription] = useState('')
  const [contextWindow, setContextWindow] = useState('')
  const [maxOutputTokens, setMaxOutputTokens] = useState('')
  const [capabilities, setCapabilities] = useState('')
  const [isDefault, setIsDefault] = useState(false)
  const [saving, setSaving] = useState(false)
  const [message, setMessage] = useState('')

  async function submit() {
    if (!name.trim() || !modelKey.trim()) return setMessage('请先填写模型名称和 Model Key。')
    setSaving(true)
    setMessage('')
    try {
      await onCreateModel({
        name: name.trim(),
        provider: provider.trim(),
        model_key: modelKey.trim(),
        description: description.trim(),
        context_window: Number(contextWindow) || 0,
        max_output_tokens: Number(maxOutputTokens) || 0,
        capabilities: capabilities.split(',').map((item) => item.trim()).filter(Boolean),
        is_default: isDefault,
      })
      setName('')
      setProvider('')
      setModelKey('')
      setDescription('')
      setContextWindow('')
      setMaxOutputTokens('')
      setCapabilities('')
      setIsDefault(false)
      setMessage('模型已创建。')
    } catch (currentError) {
      setMessage(currentError instanceof Error ? currentError.message : '创建模型失败')
    } finally {
      setSaving(false)
    }
  }

  return (
    <div className="page-shell">
      <PageHero icon="models" title="模型配置" subtitle="这里展示平台当前已配置的模型，并支持继续补充新的模型定义。" />
      <div className="editor-panel">
        <CatalogSection
          title="已配置模型"
          items={models}
          emptyTitle="当前还没有模型"
          emptyText="请先创建至少一个模型，然后在新建 Agent 或对话页中选择。"
          renderItem={(item) => (
            <CatalogItem title={item.name} secondary={`${item.provider || '未填写 Provider'} · ${item.model_key}${item.is_default ? ' · 默认模型' : ''}`}>
              {item.description || '暂无描述'}
            </CatalogItem>
          )}
        />
        <ResourceForm
          error={error}
          fields={
            <>
              <div className="form-grid form-grid-three">
                <label className="field">
                  <span>模型名称</span>
                  <input value={name} onChange={(e) => setName(e.target.value)} placeholder="例如：GPT-4.1" type="text" />
                </label>
                <label className="field">
                  <span>Provider</span>
                  <input value={provider} onChange={(e) => setProvider(e.target.value)} placeholder="例如：openai" type="text" />
                </label>
                <label className="field">
                  <span>Model Key</span>
                  <input value={modelKey} onChange={(e) => setModelKey(e.target.value)} placeholder="例如：gpt-4.1" type="text" />
                </label>
              </div>
              <label className="field">
                <span>描述</span>
                <textarea value={description} onChange={(e) => setDescription(e.target.value)} placeholder="描述这个模型适合的任务。" rows={3} />
              </label>
              <div className="form-grid form-grid-three">
                <label className="field">
                  <span>上下文窗口</span>
                  <input value={contextWindow} onChange={(e) => setContextWindow(e.target.value)} placeholder="例如：128000" type="number" />
                </label>
                <label className="field">
                  <span>最大输出 Token</span>
                  <input value={maxOutputTokens} onChange={(e) => setMaxOutputTokens(e.target.value)} placeholder="例如：4096" type="number" />
                </label>
                <label className="toggle-field">
                  <span>默认模型</span>
                  <div className="toggle-inline">
                    <input checked={isDefault} onChange={(e) => setIsDefault(e.target.checked)} type="checkbox" />
                    <strong>{isDefault ? '设为默认' : '普通模型'}</strong>
                  </div>
                </label>
              </div>
              <label className="field">
                <span>Capabilities</span>
                <input value={capabilities} onChange={(e) => setCapabilities(e.target.value)} placeholder="例如：chat,reasoning,vision" type="text" />
              </label>
            </>
          }
          hint="只填写后端已支持的模型字段，不扩展额外的路由或计费概念。"
          message={message}
          saving={saving}
          title="创建模型"
          onSubmit={submit}
          submitText="创建模型"
        />
      </div>
    </div>
  )
}

function SettingsPage({ user }: { user: UserInfo | null }) {
  return (
    <div className="page-shell">
      <PageHero icon="settings" title="系统设置" subtitle="这里只保留当前平台已存在的真实信息，不引入额外概念。" />
      <div className="editor-panel">
        <Section title="当前账号" hint="登录信息来自 /api/v1/me。">
          <div className="form-grid form-grid-three">
            <div className="field readonly">
              <span>姓名</span>
              <input readOnly type="text" value={user?.name ?? '未登录'} />
            </div>
            <div className="field readonly">
              <span>邮箱</span>
              <input readOnly type="text" value={user?.email ?? '未登录'} />
            </div>
            <div className="field readonly">
              <span>用户 ID</span>
              <input readOnly type="text" value={user?.id ?? '未登录'} />
            </div>
          </div>
        </Section>
      </div>
    </div>
  )
}

function AgentChatPage(props: DashboardProps) {
  const { activeAgent, error, models, token } = props
  const [sessions, setSessions] = useState<SessionItem[]>([])
  const [activeSessionId, setActiveSessionId] = useState('')
  const [messages, setMessages] = useState<MessageItem[]>([])
  const [model, setModel] = useState('')
  const [reasoning, setReasoning] = useState('standard')
  const [prompt, setPrompt] = useState('')
  const [sending, setSending] = useState(false)
  const [message, setMessage] = useState('')

  useEffect(() => {
    setModel(activeAgent?.model ?? models.find((item) => item.is_default)?.model_key ?? models[0]?.model_key ?? '')
  }, [activeAgent?.id, activeAgent?.model, models])

  useEffect(() => {
    if (!activeAgent) {
      setSessions([])
      setActiveSessionId('')
      setMessages([])
      return
    }
    let cancelled = false
    async function loadSessions() {
      try {
        const items = await fetchSessions(token, activeAgent.id)
        if (cancelled) return
        setSessions(items)
        setActiveSessionId((current) => (current && items.some((item) => item.id === current) ? current : items[0]?.id ?? ''))
      } catch (currentError) {
        if (!cancelled) setMessage(currentError instanceof Error ? currentError.message : '读取会话失败')
      }
    }
    loadSessions()
    return () => {
      cancelled = true
    }
  }, [activeAgent, token])

  useEffect(() => {
    if (!activeSessionId) {
      setMessages([])
      return
    }
    let cancelled = false
    async function loadMessages() {
      try {
        const items = await fetchMessages(token, activeSessionId)
        if (!cancelled) setMessages(items)
      } catch (currentError) {
        if (!cancelled) setMessage(currentError instanceof Error ? currentError.message : '读取消息失败')
      }
    }
    loadMessages()
    return () => {
      cancelled = true
    }
  }, [activeSessionId, token])

  async function submit() {
    if (!activeAgent) return setMessage('请先选择一个 Agent。')
    if (!prompt.trim()) return setMessage('请输入内容。')
    setSending(true)
    setMessage('')
    const currentPrompt = prompt.trim()
    try {
      const task = await executeTask(token, {
        agent_id: activeAgent.id,
        session_id: activeSessionId || undefined,
        session_title: currentPrompt.slice(0, 32),
        model: model || undefined,
        reasoning,
        prompt: currentPrompt,
        async: false,
      })
      const nextSessions = await fetchSessions(token, activeAgent.id)
      setSessions(nextSessions)
      setActiveSessionId(task.session_id)
      if (task.session_id) {
        setMessages(await fetchMessages(token, task.session_id))
      }
      setPrompt('')
    } catch (currentError) {
      setMessage(currentError instanceof Error ? currentError.message : '发送消息失败')
    } finally {
      setSending(false)
    }
  }

  if (!activeAgent) {
    return (
      <div className="page-shell">
        <PageHero icon="chat" title="选择一个 Agent" subtitle="左侧选择 Agent 后，这里会进入它的聊天页面。" />
      </div>
    )
  }

  const emptyState = sessions.length === 0 && messages.length === 0

  return (
    <div className="page-shell chat-page-shell">
      <div className="chat-shell chat-shell-full">
        {emptyState ? (
          <div className="chat-empty-state">
            <PageHero icon="person" title={activeAgent.name} subtitle={activeAgent.description || '还没有聊天记录，可以直接开始对话。'} />
          </div>
        ) : (
          <div className="chat-header">
            <div>
              <h1>{activeAgent.name}</h1>
              <p>{activeAgent.description || '当前 Agent 已加载，可继续对话。'}</p>
            </div>
            {sessions.length > 0 ? (
              <div className="session-strip">
                {sessions.map((item) => (
                  <button key={item.id} className={`session-chip ${activeSessionId === item.id ? 'active' : ''}`} onClick={() => setActiveSessionId(item.id)} type="button">
                    {item.title || '新对话'}
                  </button>
                ))}
              </div>
            ) : null}
          </div>
        )}

        {messages.length > 0 ? (
          <div className="message-list message-list-full">
            {messages.map((item) => (
              <div key={item.id} className={`message-bubble ${item.role === 'user' ? 'user' : 'assistant'}`}>
                <div className="message-role">{item.role === 'user' ? '你' : activeAgent.name}</div>
                <div className="message-content">{item.content}</div>
              </div>
            ))}
          </div>
        ) : null}

        <div className="composer-shell composer-shell-docked">
          <textarea className="composer-input" value={prompt} onChange={(e) => setPrompt(e.target.value)} placeholder={`向 ${activeAgent.name} 发送消息，Agent 会按已绑定的 Skill 与 MCP 进行调用。`} rows={4} />
          <div className="composer-toolbar">
            <div className="composer-group">
              <label className="toolbar-select">
                <span>模型</span>
                <select value={model} onChange={(e) => setModel(e.target.value)}>
                  <option value="">默认模型</option>
                  {models.map((item) => (
                    <option key={item.id} value={item.model_key}>
                      {item.name}
                    </option>
                  ))}
                </select>
              </label>
              <label className="toolbar-select">
                <span>思考深度</span>
                <select value={reasoning} onChange={(e) => setReasoning(e.target.value)}>
                  <option value="standard">标准</option>
                  <option value="deep">深度思考</option>
                </select>
              </label>
            </div>
            <button className="send-button" disabled={sending} onClick={submit} type="button">
              {sending ? '发送中...' : '发送'}
            </button>
          </div>
        </div>

        {error ? <div className="notice error">{error}</div> : null}
        {message ? <div className="notice error">{message}</div> : null}
      </div>
    </div>
  )
}

function PageHero(props: { icon: AppIconKey; title: string; subtitle: string }) {
  return (
    <div className="page-hero">
      <div className="hero-mark">
        <AppIcon kind={props.icon} />
      </div>
      <h1>{props.title}</h1>
      <p>{props.subtitle}</p>
    </div>
  )
}

function Section(props: { title: string; hint: string; children: ReactNode }) {
  return (
    <div className="panel-section">
      <div className="panel-section-head">
        <h3>{props.title}</h3>
        <span>{props.hint}</span>
      </div>
      {props.children}
    </div>
  )
}

function ResourceForm(props: { title: string; hint: string; fields: ReactNode; message: string; error: string; saving: boolean; submitText: string; onSubmit: () => void }) {
  return (
    <Section title={props.title} hint={props.hint}>
      {props.fields}
      {props.error ? <div className="notice error">{props.error}</div> : null}
      {props.message ? <div className={`notice ${props.message.includes('已创建') ? 'success' : 'error'}`}>{props.message}</div> : null}
      <div className="panel-actions">
        <button className="primary-button" disabled={props.saving} onClick={props.onSubmit} type="button">
          {props.saving ? '提交中...' : props.submitText}
        </button>
      </div>
    </Section>
  )
}

function CatalogSection<T>(props: { title: string; items: T[]; emptyTitle: string; emptyText: string; renderItem: (item: T) => ReactNode }) {
  return (
    <Section title={props.title} hint="保持简洁，只展示真实资源。">
      <div className="list-panel">
        {props.items.length > 0 ? props.items.map((item, index) => <div key={index}>{props.renderItem(item)}</div>) : <div className="empty-panel"><strong>{props.emptyTitle}</strong><p>{props.emptyText}</p></div>}
      </div>
    </Section>
  )
}

function CatalogItem(props: { title: string; secondary: string; children: ReactNode; actionLabel?: string; onAction?: () => void }) {
  return (
    <div className="list-item">
      <div>
        <strong>{props.title}</strong>
        <p>{props.children}</p>
        <div className="list-secondary">{props.secondary}</div>
      </div>
      {props.actionLabel && props.onAction ? <button className="secondary-button" onClick={props.onAction} type="button">{props.actionLabel}</button> : null}
    </div>
  )
}

function SelectionPanel<T extends { id: string; name: string }>(props: {
  title: string
  items: T[]
  selected: string[]
  emptyText: string
  onToggle: (id: string) => void
  renderMeta: (item: T) => string
}) {
  return (
    <div className="field">
      <span>{props.title}</span>
      <div className="check-list">
        {props.items.length > 0 ? (
          props.items.map((item) => (
            <label key={item.id} className="check-item">
              <input checked={props.selected.includes(item.id)} onChange={() => props.onToggle(item.id)} type="checkbox" />
              <div>
                <strong>{item.name}</strong>
                <small>{props.renderMeta(item)}</small>
              </div>
            </label>
          ))
        ) : (
          <div className="inline-empty">{props.emptyText}</div>
        )}
      </div>
    </div>
  )
}

function toggleSelection(value: string, items: string[], setter: (items: string[]) => void) {
  setter(items.includes(value) ? items.filter((item) => item !== value) : [...items, value])
}
