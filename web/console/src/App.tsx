import { useMemo, useState } from 'react'
import { login } from './api'

const workspaceModules = [
  { name: 'Agent 编排', hint: '智能体管理与执行', icon: '◆' },
  { name: 'MCP 接入', hint: '外部工具协议接入', icon: '◇' },
  { name: '技能中心', hint: '工具注册与治理', icon: '○' },
  { name: 'RAG 知识库', hint: '文档检索增强', icon: '□' },
]

const commandChips = ['创建 Agent', '接入工具', '导入文档', '开始对话']

const cards = [
  {
    tone: 'sand',
    title: 'Agent 工作台',
    kicker: '编排',
    body: '管理多 Agent、版本与会话，统一任务入口。',
  },
  {
    tone: 'mist',
    title: 'MCP 连接器',
    kicker: '接入',
    body: '连接外部工具服务，扩展 Agent 能力边界。',
  },
  {
    tone: 'sage',
    title: '技能中心',
    kicker: '治理',
    body: '注册和管理工具技能，规范调用边界。',
  },
  {
    tone: 'paper',
    title: 'RAG 知识库',
    kicker: '增强',
    body: '文档上传与检索，为对话提供知识支撑。',
  },
]

const historyItems = ['工作区概览', 'Agent 配置', '知识库管理']

export function App() {
  const [email, setEmail] = useState('admin@example.com')
  const [password, setPassword] = useState('ChangeMe123!')
  const [token, setToken] = useState('')
  const [error, setError] = useState('')
  const [command, setCommand] = useState('')

  const loginState = useMemo(() => {
    if (error) return `登录失败：${error}`
    if (token) return '已连接默认管理员'
    return '当前为预览模式'
  }, [error, token])

  async function submit() {
    try {
      const result = await login(email, password)
      setToken(result.token)
      setError('')
    } catch (err) {
      setToken('')
      setError(err instanceof Error ? err.message : '未知错误')
    }
  }

  return (
    <div className="shell">
      <div className="ambient ambient-a" />
      <div className="ambient ambient-b" />

      <aside className="sidebar">
        <div className="brand">
          <div className="brand-mark">KP</div>
          <div>
            <div className="brand-title">扣子工场</div>
            <div className="brand-subtitle">Go Multi-Agent Platform</div>
          </div>
        </div>

        <button className="primary-nav-button">+ 新建工作区</button>

        <div className="sidebar-section">
          <div className="sidebar-label">平台模块</div>
          <div className="module-list">
            {workspaceModules.map((item) => (
              <button key={item.name} className="module-item" type="button">
                <span className="module-icon">{item.icon}</span>
                <span>
                  <strong>{item.name}</strong>
                  <small>{item.hint}</small>
                </span>
              </button>
            ))}
          </div>
        </div>

        <div className="sidebar-section">
          <div className="sidebar-label">最近运行</div>
          <div className="history-list">
            {historyItems.map((item) => (
              <div key={item} className="history-item">
                {item}
              </div>
            ))}
          </div>
        </div>

        <div className="sidebar-footer">
          <button className="account-card" type="button">
            <div className="account-avatar">管</div>
            <div className="account-meta">
              <span>当前账号</span>
              <strong>admin@example.com</strong>
            </div>
            <div className="account-action">进入</div>
          </button>
        </div>
      </aside>

      <main className="main">
        <header className="topbar">
          <div className="topbar-actions">
            <span className="status-dot" />
            <span>{loginState}</span>
          </div>
        </header>

        <section className="hero">
          <h1>Agent 应用平台</h1>
          <p className="hero-copy">多模型 · 工具调用 · 知识增强 · 异步任务</p>
        </section>

        <section className="composer-card">
          <div className="composer-head">
            <div className="composer-title">开始对话</div>
            <div className="login-area">
              {!token ? (
                <>
                  <input 
                    className="login-input" 
                    value={email} 
                    onChange={(event) => setEmail(event.target.value)} 
                    placeholder="邮箱" 
                  />
                  <input 
                    className="login-input" 
                    value={password} 
                    onChange={(event) => setPassword(event.target.value)} 
                    placeholder="密码" 
                    type="password" 
                  />
                  <button className="primary-button" onClick={submit} type="button">
                    登录
                  </button>
                </>
              ) : (
                <span className="login-status">已登录</span>
              )}
            </div>
          </div>

          <textarea
            className="composer-input"
            value={command}
            onChange={(event) => setCommand(event.target.value)}
            placeholder="输入指令或问题，例如：创建一个数据分析 Agent"
          />

          <div className="composer-actions">
            <div className="chip-row">
              {commandChips.map((chip) => (
                <button key={chip} className="chip" onClick={() => setCommand(chip)} type="button">
                  {chip}
                </button>
              ))}
            </div>
          </div>

          {error ? <div className="error-text">{error}</div> : null}
        </section>

        <section className="card-grid">
          {cards.map((card) => (
            <article key={card.title} className={`feature-card tone-${card.tone}`}>
              <span className="feature-kicker">{card.kicker}</span>
              <h3>{card.title}</h3>
              <p>{card.body}</p>
            </article>
          ))}
        </section>
      </main>
    </div>
  )
}
