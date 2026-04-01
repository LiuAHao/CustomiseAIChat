import { useState } from 'react'

type AuthPageProps = {
  onLogin: () => void
}

export function AuthPage({ onLogin }: AuthPageProps) {
  const [isRegister, setIsRegister] = useState(false)

  return (
    <div className="auth-screen">
      <div className="auth-card">
        <div className="auth-logo">A</div>
        <h1>{isRegister ? '创建新账号' : '欢迎回来'}</h1>
        <p>Agent Management Platform</p>

        <form
          className="auth-form"
          onSubmit={(event) => {
            event.preventDefault()
            onLogin()
          }}
        >
          <input placeholder="邮箱地址" type="email" />
          <input placeholder="登录密码" type="password" />
          <button type="submit">{isRegister ? '注册' : '登录'}</button>
        </form>

        <button className="auth-switch" onClick={() => setIsRegister((value) => !value)} type="button">
          {isRegister ? '已有账号？去登录' : '没有账号？立即注册'}
        </button>
      </div>
    </div>
  )
}
