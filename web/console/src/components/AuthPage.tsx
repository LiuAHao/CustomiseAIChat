import { useState } from 'react'

type AuthPageProps = {
  onLogin: (email: string, password: string) => Promise<void>
}

export function AuthPage({ onLogin }: AuthPageProps) {
  const [email, setEmail] = useState('admin@example.com')
  const [password, setPassword] = useState('ChangeMe123!')
  const [pending, setPending] = useState(false)
  const [error, setError] = useState('')

  return (
    <div className="auth-screen">
      <div className="auth-card">
        <div className="auth-logo">A</div>
        <h1>登录平台</h1>
        <p>统一管理平台资源，按需装配 Agent 能力。</p>

        <form
          className="auth-form"
          onSubmit={async (event) => {
            event.preventDefault()
            setPending(true)
            setError('')
            try {
              await onLogin(email, password)
            } catch (currentError) {
              setError(currentError instanceof Error ? currentError.message : '登录失败')
            } finally {
              setPending(false)
            }
          }}
        >
          <input onChange={(event) => setEmail(event.target.value)} placeholder="邮箱地址" type="email" value={email} />
          <input onChange={(event) => setPassword(event.target.value)} placeholder="登录密码" type="password" value={password} />
          <button disabled={pending} type="submit">
            {pending ? '登录中...' : '登录'}
          </button>
        </form>

        {error ? <p className="auth-error">{error}</p> : null}
      </div>
    </div>
  )
}
