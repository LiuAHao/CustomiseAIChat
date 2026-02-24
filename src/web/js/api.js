/**
 * 前端网络层 - WebSocket API 客户端
 * 封装与后端的所有通信逻辑
 */

const API = (() => {
    // ===== 配置 =====
    const WS_URL = 'ws://127.0.0.1:8765';

    // ===== 内部状态 =====
    let ws = null;
    let token = '';
    let requestId = 0;
    const pendingRequests = new Map(); // requestId -> {resolve, reject, timer}
    let onDisconnect = null;
    let reconnectTimer = null;
    let isConnecting = false;

    // ===== WebSocket 连接管理 =====

    function connect() {
        return new Promise((resolve, reject) => {
            if (ws && ws.readyState === WebSocket.OPEN) {
                resolve();
                return;
            }
            if (isConnecting) {
                // 等待当前连接完成
                const check = setInterval(() => {
                    if (ws && ws.readyState === WebSocket.OPEN) {
                        clearInterval(check);
                        resolve();
                    }
                }, 100);
                return;
            }

            isConnecting = true;
            ws = new WebSocket(WS_URL);

            ws.onopen = () => {
                console.log('[API] WebSocket 已连接');
                isConnecting = false;
                if (reconnectTimer) {
                    clearTimeout(reconnectTimer);
                    reconnectTimer = null;
                }
                resolve();
            };

            ws.onclose = () => {
                console.log('[API] WebSocket 断开');
                isConnecting = false;
                ws = null;
                // 拒绝所有待处理请求
                pendingRequests.forEach(({ reject: rej, timer }) => {
                    clearTimeout(timer);
                    rej(new Error('连接断开'));
                });
                pendingRequests.clear();
                if (onDisconnect) onDisconnect();
            };

            ws.onerror = (err) => {
                console.error('[API] WebSocket 错误', err);
                isConnecting = false;
                reject(new Error('无法连接到服务器'));
            };

            ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    // 后端不返回 requestId，使用有序队列处理
                    // 由于是请求-响应模式，按 FIFO 顺序匹配
                    if (pendingRequests.size > 0) {
                        const firstKey = pendingRequests.keys().next().value;
                        const { resolve: res, timer } = pendingRequests.get(firstKey);
                        clearTimeout(timer);
                        pendingRequests.delete(firstKey);
                        res(data);
                    }
                } catch (e) {
                    console.error('[API] 解析响应失败', e);
                }
            };
        });
    }

    function disconnect() {
        if (ws) {
            ws.close();
            ws = null;
        }
    }

    /**
     * 发送请求并等待响应
     */
    async function request(action, params = {}, timeout = 30000) {
        await connect();
        
        return new Promise((resolve, reject) => {
            const id = ++requestId;
            const payload = { action, ...params };
            if (token) payload.token = token;

            const timer = setTimeout(() => {
                pendingRequests.delete(id);
                reject(new Error('请求超时'));
            }, timeout);

            pendingRequests.set(id, { resolve, reject, timer });
            ws.send(JSON.stringify(payload));
        });
    }

    // ===== 公共 API =====

    return {
        /** 设置断开回调 */
        onDisconnect(fn) { onDisconnect = fn; },

        /** 手动连接 */
        connect,
        disconnect,

        /** 获取/设置 token */
        getToken() { return token; },
        setToken(t) { token = t; },
        clearToken() { token = ''; },

        // ==================== 用户管理 ====================

        /** 注册 */
        async register(username, password, nickname) {
            return request('register', { username, password, nickname });
        },

        /** 登录 */
        async login(username, password) {
            const res = await request('login', { username, password });
            if (res.code === 0 && res.token) {
                token = res.token;
            }
            return res;
        },

        /** 退出登录 */
        async logout() {
            const res = await request('logout');
            token = '';
            return res;
        },

        /** 获取用户信息 */
        async getUserInfo() {
            return request('get_user_info');
        },

        /** 修改密码 */
        async updatePassword(oldPassword, newPassword) {
            return request('update_password', { oldPassword, newPassword });
        },

        /** 修改昵称 */
        async updateNickname(nickname) {
            return request('update_nickname', { nickname });
        },

        /** 注销账号 */
        async deleteAccount(password) {
            return request('delete_account', { password });
        },

        // ==================== 人设管理 ====================

        /** 创建人设 */
        async createPersona(name, description, systemPrompt, avatar) {
            return request('create_persona', { name, description, systemPrompt, avatar });
        },

        /** 删除人设 */
        async deletePersona(personaId) {
            return request('delete_persona', { personaId });
        },

        /** 获取人设列表 */
        async listPersonas() {
            return request('list_personas');
        },

        /** 获取单个人设 */
        async getPersona(personaId) {
            return request('get_persona', { personaId });
        },

        /** 更新人设 */
        async updatePersona(personaId, name, description, systemPrompt, avatar) {
            return request('update_persona', { personaId, name, description, systemPrompt, avatar });
        },

        // ==================== 会话管理 ====================

        /** 创建会话 */
        async createConversation(personaId, title) {
            return request('create_conversation', { personaId, title: title || '' });
        },

        /** 删除会话 */
        async deleteConversation(conversationId) {
            return request('delete_conversation', { conversationId });
        },

        /** 获取会话列表 */
        async listConversations(limit = 50) {
            return request('list_conversations', { limit });
        },

        /** 获取某人设下的会话列表 */
        async listConversationsByPersona(personaId) {
            return request('list_conversations_by_persona', { personaId });
        },

        /** 获取会话详情 */
        async getConversation(conversationId) {
            return request('get_conversation', { conversationId });
        },

        /** 更新会话标题 */
        async updateConversationTitle(conversationId, title) {
            return request('update_conversation_title', { conversationId, title });
        },

        // ==================== 聊天操作 ====================

        /** 发送消息（含 AI 回复） */
        async sendMessage(conversationId, message) {
            return request('send_message', { conversationId, message }, 60000);
        },

        /** 获取聊天历史 */
        async getHistory(conversationId, limit = 50) {
            return request('get_history', { conversationId, limit });
        },

        /** 清空聊天历史 */
        async clearHistory(conversationId) {
            return request('clear_history', { conversationId });
        },

        // ==================== AI 能力 ====================

        /** 新建会话后让人设主动招呼 */
        async greet(conversationId) {
            return request('greet', { conversationId }, 30000);
        },

        /** AI生成人设描述 */
        async generateDesc(name) {
            return request('generate_desc', { name }, 30000);
        },
    };
})();
