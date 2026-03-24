export type LoginResponse = {
  token: string
  user: { id: string; email: string; name: string }
}

const API_BASE = 'http://localhost:8081/api/v1'

export async function login(email: string, password: string): Promise<LoginResponse> {
  const response = await fetch(`${API_BASE}/auth/login`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ email, password }),
  })

  if (!response.ok) {
    throw new Error('login failed')
  }

  return response.json()
}
