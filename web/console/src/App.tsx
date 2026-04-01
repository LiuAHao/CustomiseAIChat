import { useEffect, useState } from 'react'
import { AgentDashboard } from './components/AgentDashboard'
import { AuthPage } from './components/AuthPage'
import { Sidebar } from './components/Sidebar'

export type ViewKey = 'dashboard' | 'skills' | 'mcp' | 'models' | 'settings'

export function App() {
  const [isLoggedIn, setIsLoggedIn] = useState(true)
  const [view, setView] = useState<ViewKey>('dashboard')
  const [activeAgentId, setActiveAgentId] = useState('invest')
  const [sidebarWidth, setSidebarWidth] = useState(448)
  const [isDragging, setIsDragging] = useState(false)

  useEffect(() => {
    if (!isDragging) return

    function handleMove(event: MouseEvent) {
      const nextWidth = Math.min(620, Math.max(320, event.clientX))
      setSidebarWidth(nextWidth)
    }

    function handleUp() {
      setIsDragging(false)
    }

    window.addEventListener('mousemove', handleMove)
    window.addEventListener('mouseup', handleUp)

    return () => {
      window.removeEventListener('mousemove', handleMove)
      window.removeEventListener('mouseup', handleUp)
    }
  }, [isDragging])

  if (!isLoggedIn) {
    return <AuthPage onLogin={() => setIsLoggedIn(true)} />
  }

  return (
    <div
      className={`desktop-shell ${isDragging ? 'dragging' : ''}`}
      style={{ gridTemplateColumns: `${sidebarWidth}px 10px minmax(0, 1fr)` }}
    >
      <Sidebar
        activeAgentId={activeAgentId}
        currentView={view}
        onChangeAgent={setActiveAgentId}
        onChangeView={setView}
        onLogout={() => setIsLoggedIn(false)}
      />

      <div
        aria-label="调整侧边栏宽度"
        className="sidebar-resizer"
        onMouseDown={() => setIsDragging(true)}
        role="separator"
      />

      <main className="desktop-main">
        <header className="desktop-menubar" />

        <section className="desktop-content">
          <AgentDashboard activeAgentId={activeAgentId} currentView={view} />
        </section>
      </main>
    </div>
  )
}
