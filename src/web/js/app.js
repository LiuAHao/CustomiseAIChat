/**
 * AI人设聊天应用 - 前端交互逻辑（后端集成版）
 */

const AppState = {
    currentView: 'login-view',
    user: null,
    personas: [],
    currentPersonaId: null,
    conversations: [],
    currentConvId: null,
};

const Toast = {
    show(msg, type = 'info', duration = 3000) {
        const old = document.getElementById('toast-container');
        if (old) old.remove();
        const colors = { success: 'bg-green-500', error: 'bg-red-500', info: 'bg-blue-500', warning: 'bg-yellow-500 text-gray-800' };
        const icons = { success: 'fa-circle-check', error: 'fa-circle-xmark', info: 'fa-circle-info', warning: 'fa-triangle-exclamation' };
        const div = document.createElement('div');
        div.id = 'toast-container';
        div.className = `fixed top-5 left-1/2 -translate-x-1/2 z-[999] ${colors[type] || colors.info} text-white px-5 py-3 rounded-lg shadow-lg flex items-center gap-2 text-sm animate-fade-in`;
        div.innerHTML = `<i class="fa-solid ${icons[type] || icons.info}"></i><span>${msg}</span>`;
        document.body.appendChild(div);
        setTimeout(() => div.remove(), duration);
    },
};

const ConfirmDialog = {
    _resolve: null,
    show(title, message, okLabel = '确认删除') {
        return new Promise(resolve => {
            this._resolve = resolve;
            document.getElementById('confirm-title').textContent = title;
            document.getElementById('confirm-message').textContent = message;
            document.getElementById('confirm-ok-btn').textContent = okLabel;
            document.getElementById('confirm-modal').classList.remove('hidden');
        });
    },
    _close(result) {
        document.getElementById('confirm-modal').classList.add('hidden');
        if (this._resolve) { this._resolve(result); this._resolve = null; }
    },
    init() {
        document.getElementById('confirm-ok-btn')?.addEventListener('click', () => this._close(true));
        document.getElementById('confirm-cancel-btn')?.addEventListener('click', () => this._close(false));
        document.getElementById('confirm-modal')?.addEventListener('click', e => {
            if (e.target === e.currentTarget) this._close(false);
        });
    },
};

const ViewManager = {
    switchView(viewId) {
        ['login-view', 'app-view', 'register-view', 'forgot-view'].forEach(id => {
            document.getElementById(id)?.classList.add('hidden');
        });
        const target = document.getElementById(viewId);
        if (target) {
            target.classList.remove('hidden');
            AppState.currentView = viewId;
        }
        if (viewId === 'app-view') this.scrollChatToBottom();
    },
    toggleModal(modalId) {
        const modal = document.getElementById(modalId);
        if (!modal) return;
        const isHidden = modal.classList.contains('hidden');
        if (isHidden) {
            modal.classList.remove('hidden');
            modal.querySelector('.modal-overlay')?.classList.add('animate-overlay-in');
            modal.querySelector('.modal-panel')?.classList.add('animate-modal-in');
        } else {
            modal.classList.add('hidden');
        }
    },
    scrollChatToBottom() {
        requestAnimationFrame(() => {
            const chatArea = document.getElementById('chat-messages');
            if (chatArea) chatArea.scrollTop = chatArea.scrollHeight;
        });
    },
};

const ChatManager = {
    formatTimestamp(ts) {
        if (!ts) return '';
        try {
            // 后端返回 UTC时间，加 'Z' 后浏览器自动转换为本地时间
            const tsStr = String(ts).replace(' ', 'T');
            const d = new Date(tsStr.endsWith('Z') ? tsStr : tsStr + 'Z');
            if (isNaN(d)) return ts;
            let h = d.getHours();
            const m = d.getMinutes();
            const ampm = h >= 12 ? 'PM' : 'AM';
            h = h % 12 || 12;
            return `${h}:${String(m).padStart(2, '0')} ${ampm}`;
        } catch {
            return ts;
        }
    },
    escapeHtml(str = '') {
        const map = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' };
        return String(str).replace(/[&<>"']/g, c => map[c]);
    },
    getPersonaAvatarHtml(persona) {
        if (persona?.avatar && !persona.avatar.startsWith('emoji:')) {
            return `<img src="${persona.avatar}" class="w-8 h-8 rounded-full object-cover">`;
        }
        const emoji = persona?.avatar?.startsWith('emoji:') ? persona.avatar.replace('emoji:', '') : '🤖';
        return `<span>${emoji}</span>`;
    },
    appendMessage(role, text, time) {
        const chatArea = document.getElementById('chat-messages');
        if (!chatArea) return;
        // 清除空状态占位符
        const placeholder = chatArea.querySelector('.empty-placeholder');
        if (placeholder) placeholder.remove();
        const div = document.createElement('div');
        div.className = 'animate-msg';
        if (role === 'assistant' || role === 'ai') {
            const persona = AppState.personas.find(p => p.id === AppState.currentPersonaId);
            div.innerHTML = `
                <div class="flex items-start gap-4">
                    <div class="w-8 h-8 bg-gray-100 rounded-full flex flex-shrink-0 items-center justify-center border border-gray-200 text-sm">${this.getPersonaAvatarHtml(persona)}</div>
                    <div class="flex flex-col gap-1 items-start max-w-[70%]">
                        <div class="bg-white border border-gray-100 shadow-sm p-4 rounded-2xl rounded-tl-sm text-sm text-gray-700 leading-relaxed">${text}</div>
                        <span class="text-[11px] text-gray-400 ml-1">${time || ''}</span>
                    </div>
                </div>`;
        } else {
            div.innerHTML = `
                <div class="flex items-start gap-4 flex-row-reverse">
                    <div class="flex flex-col gap-1 items-end max-w-[70%]">
                        <div class="bg-secondary text-white shadow-md shadow-secondary/20 p-4 rounded-2xl rounded-tr-sm text-sm leading-relaxed">${this.escapeHtml(text)}</div>
                        <span class="text-[11px] text-gray-400 mr-1">${time || ''}</span>
                    </div>
                </div>`;
        }
        chatArea.appendChild(div);
    },
    renderMessages(messages) {
        const chatArea = document.getElementById('chat-messages');
        if (!chatArea) return;
        chatArea.innerHTML = '';
        if (!messages || messages.length === 0) {
            chatArea.innerHTML = `<div class="empty-placeholder flex flex-col items-center justify-center h-full text-gray-400"><i class="fa-regular fa-comments text-4xl mb-3"></i><p class="text-sm">开始你们的对话吧</p></div>`;
            return;
        }
        messages.forEach(m => this.appendMessage(m.role, m.content, this.formatTimestamp(m.timestamp)));
        ViewManager.scrollChatToBottom();
    },
    showTypingIndicator() {
        const chatArea = document.getElementById('chat-messages');
        if (!chatArea) return;
        const persona = AppState.personas.find(p => p.id === AppState.currentPersonaId);
        const typingEl = document.createElement('div');
        typingEl.id = 'typing-indicator';
        typingEl.className = 'flex items-start gap-4 animate-msg';
        typingEl.innerHTML = `
            <div class="w-8 h-8 bg-gray-100 rounded-full flex flex-shrink-0 items-center justify-center border border-gray-200 text-sm">${this.getPersonaAvatarHtml(persona)}</div>
            <div class="bg-white border border-gray-100 shadow-sm px-5 py-3 rounded-2xl rounded-tl-sm">
                <span class="typing-dot"></span><span class="typing-dot"></span><span class="typing-dot"></span>
            </div>`;
        chatArea.appendChild(typingEl);
        ViewManager.scrollChatToBottom();
    },
    removeTypingIndicator() {
        document.getElementById('typing-indicator')?.remove();
    },
    async loadHistory(conversationId) {
        const res = await API.getHistory(conversationId);
        if (res.code !== 0) throw new Error(res.message || '加载历史失败');
        this.renderMessages(res.messages || []);
    },
    async sendMessage() {
        const textarea = document.getElementById('msg-input');
        const text = textarea?.value.trim();
        if (!text) return;
        if (!AppState.currentConvId) return;
        const now = new Date();
        const localTime = `${(now.getHours() % 12 || 12)}:${String(now.getMinutes()).padStart(2, '0')} ${now.getHours() >= 12 ? 'PM' : 'AM'}`;
        this.appendMessage('user', text, localTime);
        textarea.value = '';
        textarea.style.height = 'auto';
        this.showTypingIndicator();
        try {
            const res = await API.sendMessage(AppState.currentConvId, text);
            this.removeTypingIndicator();
            if (res.code === 0) {
                this.appendMessage('assistant', res.reply || '', this.formatTimestamp(res.timestamp));
                ViewManager.scrollChatToBottom();
            } else {
                Toast.show(res.message || '发送失败', 'error');
            }
        } catch (e) {
            this.removeTypingIndicator();
            Toast.show(`发送失败: ${e.message}`, 'error');
        }
    },
};

const ConversationManager = {
    /** 静默加载或自动创建该人设的会话（每个人设对应一个会话）*/
    async loadForPersona(personaId) {
        const res = await API.listConversationsByPersona(personaId);
        if (res.code !== 0) throw new Error(res.message || '加载会话失败');
        AppState.conversations = res.conversations || [];
        if (AppState.conversations.length > 0) {
            AppState.currentConvId = AppState.conversations[0].id;
            await ChatManager.loadHistory(AppState.currentConvId);
        } else {
            // 自动为该人设创建会话
            const cr = await API.createConversation(personaId, '');
            if (cr.code !== 0) throw new Error(cr.message || '创建会话失败');
            AppState.currentConvId = cr.conversationId;
            ChatManager.renderMessages([]);
            // 人设主动发起招呼
            ChatManager.showTypingIndicator();
            try {
                const gr = await API.greet(cr.conversationId);
                ChatManager.removeTypingIndicator();
                if (gr.code === 0 && gr.reply) {
                    ChatManager.appendMessage('assistant', gr.reply, ChatManager.formatTimestamp(gr.timestamp));
                    ViewManager.scrollChatToBottom();
                }
            } catch {
                ChatManager.removeTypingIndicator();
            }
        }
    },
};

const SidebarManager = {
    buildAvatarHtml(persona) {
        if (persona.avatar && !persona.avatar.startsWith('emoji:')) {
            return `<img src="${persona.avatar}" alt="" class="w-9 h-9 rounded-lg object-cover">`;
        }
        const emoji = persona.avatar?.startsWith('emoji:') ? persona.avatar.replace('emoji:', '') : '🤖';
        const bgColors = ['bg-green-100', 'bg-pink-100', 'bg-blue-100', 'bg-purple-100', 'bg-orange-100', 'bg-cyan-100', 'bg-yellow-100', 'bg-teal-100'];
        const bg = bgColors[(persona.id || 0) % bgColors.length];
        return `<div class="w-9 h-9 ${bg} rounded-lg flex items-center justify-center text-xl">${emoji}</div>`;
    },
    renderPersonaList() {
        const list = document.getElementById('persona-list');
        if (!list) return;
        list.innerHTML = '';
        AppState.personas.forEach(persona => {
            const isActive = persona.id === AppState.currentPersonaId;
            const li = document.createElement('li');
            li.dataset.personaId = persona.id;
            li.innerHTML = `
                <a href="#" class="flex items-center gap-3 p-2.5 ${isActive ? 'bg-white rounded-lg border border-gray-100 shadow-sm' : 'hover:bg-gray-100 rounded-lg'} transition relative group cursor-pointer">
                    ${isActive ? '<div class="sidebar-indicator"></div>' : ''}
                    ${this.buildAvatarHtml(persona)}
                    <div class="flex-1 min-w-0">
                        <h4 class="text-sm ${isActive ? 'font-medium text-gray-800' : 'text-gray-600 group-hover:text-gray-800'} truncate">${ChatManager.escapeHtml(persona.name)}</h4>
                        <p class="text-xs text-gray-400 truncate mt-0.5">${ChatManager.escapeHtml(persona.description || '')}</p>
                    </div>
                </a>`;
            list.appendChild(li);
        });
    },
    async loadPersonas() {
        const res = await API.listPersonas();
        if (res.code !== 0) throw new Error(res.message || '加载人设失败');
        AppState.personas = res.personas || [];
        this.renderPersonaList();
        if (AppState.personas.length > 0) {
            await this.selectPersona(AppState.personas[0].id);
        } else {
            document.getElementById('chat-persona-name').textContent = '暂无人设';
            document.getElementById('chat-persona-desc').textContent = '请先创建人设';
            document.getElementById('chat-messages').innerHTML = '';
        }
    },
    async selectPersona(personaId) {
        if (personaId === AppState.currentPersonaId) return;
        AppState.currentPersonaId = personaId;
        this.renderPersonaList();
        const persona = AppState.personas.find(p => p.id === personaId);
        document.getElementById('chat-persona-name').textContent = persona?.name || '';
        document.getElementById('chat-persona-desc').textContent = persona?.description || '';
        try {
            await ConversationManager.loadForPersona(personaId);
        } catch (e) {
            Toast.show(e.message, 'error');
        }
        document.getElementById('sidebar')?.classList.remove('open');
        document.getElementById('sidebar-overlay')?.classList.add('hidden');
    },
};

const PersonaCreator = {
    selectedAvatar: 'emoji:🤖',
    customAvatarData: null,
    editingPersonaId: null,
    resetForm() {
        document.getElementById('persona-name-input').value = '';
        document.getElementById('persona-desc-input').value = '';
        document.getElementById('desc-char-count').textContent = '0';
        this.selectedAvatar = 'emoji:🤖';
        this.customAvatarData = null;
        document.querySelectorAll('#preset-avatars .avatar-option').forEach((b, i) => {
            b.classList.toggle('selected', i === 0);
        });
        document.getElementById('custom-avatar-preview')?.classList.add('hidden');
    },
    openModal() {
        this.editingPersonaId = null;
        this.resetForm();
        document.getElementById('persona-modal-title').textContent = '创建新人设';
        document.getElementById('btn-confirm-create-persona').innerHTML = '<i class="fa-solid fa-plus text-xs"></i> 创建人设';
        ViewManager.toggleModal('create-persona-modal');
    },
    openEditModal(personaId) {
        const persona = AppState.personas.find(p => p.id === personaId);
        if (!persona) return;
        this.editingPersonaId = personaId;
        this.resetForm();
        document.getElementById('persona-name-input').value = persona.name || '';
        document.getElementById('persona-desc-input').value = persona.description || '';
        document.getElementById('desc-char-count').textContent = (persona.description || '').length;
        if (persona.avatar && persona.avatar.startsWith('emoji:')) {
            this.selectedAvatar = persona.avatar;
            document.querySelectorAll('#preset-avatars .avatar-option').forEach(b => {
                b.classList.toggle('selected', b.dataset.avatar === this.selectedAvatar);
            });
        } else if (persona.avatar) {
            this.selectedAvatar = 'custom';
            this.customAvatarData = persona.avatar;
            document.getElementById('custom-avatar-img').src = persona.avatar;
            document.getElementById('custom-avatar-preview')?.classList.remove('hidden');
            document.querySelectorAll('#preset-avatars .avatar-option').forEach(b => b.classList.remove('selected'));
        }
        document.getElementById('persona-modal-title').textContent = '编辑人设';
        document.getElementById('btn-confirm-create-persona').innerHTML = '<i class="fa-solid fa-check text-xs"></i> 保存修改';
        ViewManager.toggleModal('create-persona-modal');
    },
    closeModal() {
        ViewManager.toggleModal('create-persona-modal');
    },
    selectPreset(btn) {
        document.querySelectorAll('#preset-avatars .avatar-option').forEach(b => b.classList.remove('selected'));
        btn.classList.add('selected');
        this.selectedAvatar = btn.dataset.avatar;
        this.customAvatarData = null;
        document.getElementById('custom-avatar-preview')?.classList.add('hidden');
    },
    handleCustomUpload(input) {
        const file = input.files?.[0];
        if (!file) return;
        if (file.size > 2 * 1024 * 1024) {
            Toast.show('图片大小不能超过 2MB', 'warning');
            return;
        }
        const reader = new FileReader();
        reader.onload = e => {
            this.customAvatarData = e.target.result;
            this.selectedAvatar = 'custom';
            document.getElementById('custom-avatar-img').src = e.target.result;
            document.getElementById('custom-avatar-preview')?.classList.remove('hidden');
            document.querySelectorAll('#preset-avatars .avatar-option').forEach(b => b.classList.remove('selected'));
        };
        reader.readAsDataURL(file);
    },
    removeCustomAvatar() {
        this.customAvatarData = null;
        this.selectedAvatar = 'emoji:🤖';
        document.getElementById('custom-avatar-preview')?.classList.add('hidden');
        document.getElementById('persona-avatar-upload').value = '';
        const first = document.querySelector('#preset-avatars .avatar-option');
        if (first) first.classList.add('selected');
    },
    async generateDesc() {
        const name = document.getElementById('persona-name-input')?.value.trim();
        if (!name) {
            Toast.show('请先输入人设名称', 'warning');
            return;
        }
        const btn = document.getElementById('btn-ai-gen-desc');
        btn.innerHTML = '<i class="fa-solid fa-spinner fa-spin text-[10px]"></i><span>生成中...</span>';
        btn.disabled = true;
        try {
            const res = await API.generateDesc(name);
            if (res.code === 0 && res.description) {
                document.getElementById('persona-desc-input').value = res.description;
                document.getElementById('desc-char-count').textContent = res.description.length;
            } else {
                Toast.show(res.message || 'AI生成失败', 'error');
            }
        } catch (e) {
            Toast.show(`生成失败: ${e.message}`, 'error');
        } finally {
            btn.innerHTML = '<i class="fa-solid fa-wand-magic-sparkles text-[10px]"></i><span>AI 生成</span>';
            btn.disabled = false;
        }
    },
    async confirmCreate() {
        const name = document.getElementById('persona-name-input')?.value.trim();
        const description = document.getElementById('persona-desc-input')?.value.trim() || '';
        const systemPrompt = description; // 人设描述同时作为系统提示词
        if (!name) {
            Toast.show('人设名称不能为空', 'warning');
            return;
        }
        const avatar = (this.selectedAvatar === 'custom' && this.customAvatarData) ? this.customAvatarData : this.selectedAvatar;
        const btn = document.getElementById('btn-confirm-create-persona');
        btn.disabled = true;
        try {
            if (this.editingPersonaId !== null) {
                const res = await API.updatePersona(this.editingPersonaId, name, description, systemPrompt, avatar || '');
                if (res.code !== 0) throw new Error(res.message || '更新失败');
                Toast.show('人设已更新', 'success');
                // 如果编辑的是当前人设，将 currentPersonaId 清零以强制刷新聊天内头像
                if (this.editingPersonaId === AppState.currentPersonaId) {
                    AppState.currentPersonaId = null;
                }
            } else {
                const res = await API.createPersona(name, description, systemPrompt, avatar || '');
                if (res.code !== 0) throw new Error(res.message || '创建失败');
                Toast.show('人设创建成功', 'success');
            }
            await SidebarManager.loadPersonas();
            this.closeModal();
        } catch (e) {
            Toast.show(e.message, 'error');
        } finally {
            btn.disabled = false;
        }
    },
};

const Auth = {
    updateUserUI() {
        if (!AppState.user) return;
        const displayName = AppState.user.nickname || AppState.user.username;
        const nameEl = document.querySelector('#sidebar .p-5 .font-bold');
        if (nameEl) nameEl.textContent = displayName;
        const settingsName = document.getElementById('settings-display-name');
        if (settingsName) settingsName.textContent = displayName;
        document.getElementById('settings-email').textContent = AppState.user.username || '';
        document.getElementById('settings-nickname-input').value = displayName;
        // 恢复本地保存的用户头像
        const savedAvatar = localStorage.getItem('user_avatar');
        if (savedAvatar) {
            const sa = document.getElementById('settings-avatar');
            const sb = document.getElementById('sidebar-user-avatar');
            if (sa) sa.src = savedAvatar;
            if (sb) sb.src = savedAvatar;
        }
    },
    async initAppData() {
        this.updateUserUI();
        try {
            await SidebarManager.loadPersonas();
        } catch (e) {
            Toast.show(e.message, 'error');
        }
    },
    async doLogin(form) {
        const username = document.getElementById('login-email')?.value.trim()
            || form.querySelectorAll('input')[0]?.value.trim();
        const password = document.getElementById('login-password')?.value.trim()
            || form.querySelectorAll('input')[1]?.value.trim();
        if (!username || !password) {
            Toast.show('请输入邮箱和密码', 'warning');
            return;
        }
        const btn = form.querySelector('button[type="submit"]');
        btn.disabled = true;
        btn.innerHTML = '<i class="fa-solid fa-spinner fa-spin mr-2"></i>登录中...';
        try {
            const res = await API.login(username, password);
            if (res.code !== 0) throw new Error(res.message || '登录失败');
            AppState.user = { userId: res.userId, username, nickname: res.nickname || username };
            // 保存 session 到 localStorage
            localStorage.setItem('session_token', API.getToken());
            localStorage.setItem('session_user', JSON.stringify(AppState.user));
            // 处理"记住我"
            const rememberMe = document.getElementById('remember-me-checkbox')?.checked;
            if (rememberMe) {
                localStorage.setItem('remembered_email', username);
                localStorage.setItem('remembered_password', password);
            } else {
                localStorage.removeItem('remembered_email');
                localStorage.removeItem('remembered_password');
            }
            ViewManager.switchView('app-view');
            Toast.show('登录成功', 'success');
            await this.initAppData();
        } catch (e) {
            Toast.show(e.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '登 录';
        }
    },
    async doRegister(form) {
        const inputs = form.querySelectorAll('input');
        const nickname = inputs[0]?.value.trim();
        const username = inputs[1]?.value.trim();
        const password = inputs[2]?.value.trim();
        const confirmPwd = inputs[3]?.value.trim();
        if (!nickname || !username || !password) {
            Toast.show('请填写完整注册信息', 'warning');
            return;
        }
        if (password !== confirmPwd) {
            Toast.show('两次密码不一致', 'warning');
            return;
        }
        if (password.length < 6) {
            Toast.show('密码至少6位', 'warning');
            return;
        }
        const btn = form.querySelector('button[type="submit"]');
        btn.disabled = true;
        btn.innerHTML = '<i class="fa-solid fa-spinner fa-spin mr-2"></i>注册中...';
        try {
            const res = await API.register(username, password, nickname);
            if (res.code !== 0) throw new Error(res.message || '注册失败');
            Toast.show('注册成功，请登录', 'success');
            ViewManager.switchView('login-view');
        } catch (e) {
            Toast.show(e.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '注 册';
        }
    },
    async doLogout() {
        try { await API.logout(); } catch {}
        API.clearToken();
        localStorage.removeItem('session_token');
        localStorage.removeItem('session_user');
        AppState.user = null;
        AppState.personas = [];
        AppState.currentPersonaId = null;
        AppState.conversations = [];
        AppState.currentConvId = null;
        ViewManager.toggleModal('settings-modal');
        ViewManager.switchView('login-view');
        Toast.show('已退出登录', 'info');
    },
    async updateNickname() {
        const nickname = document.getElementById('settings-nickname-input')?.value.trim();
        if (!nickname) {
            Toast.show('昵称不能为空', 'warning');
            return;
        }
        const res = await API.updateNickname(nickname);
        if (res.code !== 0) {
            Toast.show(res.message || '更新昵称失败', 'error');
            return;
        }
        AppState.user.nickname = nickname;
        this.updateUserUI();
        Toast.show('昵称已更新', 'success');
    },
    async updatePassword() {
        const oldPwd = document.getElementById('settings-old-password')?.value.trim();
        const newPwd = document.getElementById('settings-new-password')?.value.trim();
        const confirmPwd = document.getElementById('settings-confirm-password')?.value.trim();
        if (!oldPwd || !newPwd || !confirmPwd) {
            Toast.show('请填写完整密码信息', 'warning');
            return;
        }
        if (newPwd !== confirmPwd) {
            Toast.show('两次新密码不一致', 'warning');
            return;
        }
        if (newPwd.length < 6) {
            Toast.show('新密码至少6位', 'warning');
            return;
        }
        const res = await API.updatePassword(oldPwd, newPwd);
        if (res.code !== 0) {
            Toast.show(res.message || '修改密码失败', 'error');
            return;
        }
        Toast.show('密码已更新', 'success');
        document.getElementById('settings-old-password').value = '';
        document.getElementById('settings-new-password').value = '';
        document.getElementById('settings-confirm-password').value = '';
    },
};

function togglePasswordVisibility(btn) {
    const input = btn.closest('.relative')?.querySelector('input') || btn.parentElement.querySelector('input');
    if (!input) return;
    const icon = btn.querySelector('i') || btn;
    if (input.type === 'password') {
        input.type = 'text';
        icon.classList.replace('fa-eye-slash', 'fa-eye');
    } else {
        input.type = 'password';
        icon.classList.replace('fa-eye', 'fa-eye-slash');
    }
}

function autoResizeTextarea(el) {
    el.style.height = 'auto';
    el.style.height = Math.min(el.scrollHeight, 128) + 'px';
}

document.addEventListener('DOMContentLoaded', async () => {
    API.onDisconnect(() => {
        if (AppState.currentView === 'app-view') Toast.show('连接已断开，请刷新页面', 'error', 5000);
    });

    // 前填"记住我"的账号密码
    const rememberedEmail = localStorage.getItem('remembered_email');
    const rememberedPassword = localStorage.getItem('remembered_password');
    if (rememberedEmail) {
        const emailEl = document.getElementById('login-email');
        const pwdEl = document.getElementById('login-password');
        const cbEl = document.getElementById('remember-me-checkbox');
        if (emailEl) emailEl.value = rememberedEmail;
        if (pwdEl && rememberedPassword) pwdEl.value = rememberedPassword;
        if (cbEl) cbEl.checked = true;
    }

    // 尝试恢复登录状态
    const savedToken = localStorage.getItem('session_token');
    const savedUser = localStorage.getItem('session_user');
    if (savedToken && savedUser) {
        try {
            API.setToken(savedToken);
            const res = await API.getUserInfo();
            if (res.code === 0) {
                AppState.user = JSON.parse(savedUser);
                AppState.user.nickname = res.nickname || AppState.user.username;
                // 同步更新 localStorage 中的昵称
                localStorage.setItem('session_user', JSON.stringify(AppState.user));
                ViewManager.switchView('app-view');
                await Auth.initAppData();
            } else {
                localStorage.removeItem('session_token');
                localStorage.removeItem('session_user');
                API.clearToken();
            }
        } catch {
            localStorage.removeItem('session_token');
            localStorage.removeItem('session_user');
            API.clearToken();
        }
    }

    const loginForm = document.getElementById('login-form');
    loginForm?.addEventListener('submit', e => { e.preventDefault(); Auth.doLogin(loginForm); });

    const registerForm = document.getElementById('register-form');
    registerForm?.addEventListener('submit', e => { e.preventDefault(); Auth.doRegister(registerForm); });

    document.getElementById('link-register')?.addEventListener('click', e => { e.preventDefault(); ViewManager.switchView('register-view'); });
    document.getElementById('link-to-login-from-reg')?.addEventListener('click', e => { e.preventDefault(); ViewManager.switchView('login-view'); });
    document.getElementById('link-forgot')?.addEventListener('click', e => { e.preventDefault(); ViewManager.switchView('forgot-view'); });
    document.getElementById('link-to-login-from-forgot')?.addEventListener('click', e => { e.preventDefault(); ViewManager.switchView('login-view'); });

    const forgotForm = document.getElementById('forgot-form');
    forgotForm?.addEventListener('submit', e => { e.preventDefault(); Toast.show('该功能暂未实现', 'warning'); });

    const msgInput = document.getElementById('msg-input');
    msgInput?.addEventListener('input', () => autoResizeTextarea(msgInput));
    msgInput?.addEventListener('keydown', e => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            ChatManager.sendMessage();
        }
    });
    document.getElementById('send-btn')?.addEventListener('click', () => ChatManager.sendMessage());

    document.getElementById('persona-list')?.addEventListener('click', e => {
        const li = e.target.closest('li');
        if (!li) return;
        e.preventDefault();
        const personaId = parseInt(li.dataset.personaId, 10);
        if (!Number.isNaN(personaId)) SidebarManager.selectPersona(personaId);
    });

    document.querySelectorAll('.toggle-pwd').forEach(btn => btn.addEventListener('click', () => togglePasswordVisibility(btn)));

    // 用户头像上传
    document.getElementById('avatar-upload')?.addEventListener('change', e => {
        const file = e.target.files?.[0];
        if (!file) return;
        if (file.size > 3 * 1024 * 1024) {
            Toast.show('图片大小不能超过 3MB', 'warning');
            return;
        }
        const reader = new FileReader();
        reader.onload = ev => {
            const dataUrl = ev.target.result;
            localStorage.setItem('user_avatar', dataUrl);
            const sa = document.getElementById('settings-avatar');
            const sb = document.getElementById('sidebar-user-avatar');
            if (sa) sa.src = dataUrl;
            if (sb) sb.src = dataUrl;
            Toast.show('头像已更新', 'success');
        };
        reader.readAsDataURL(file);
        e.target.value = '';
    });

    // Pro 订阅弹窗
    const openSubscribeModal = () => {
        document.getElementById('subscribe-modal')?.classList.remove('hidden');
    };
    const closeSubscribeModal = () => {
        document.getElementById('subscribe-modal')?.classList.add('hidden');
    };
    document.getElementById('tier-pro')?.addEventListener('click', openSubscribeModal);
    document.getElementById('subscribe-close-btn')?.addEventListener('click', closeSubscribeModal);
    document.getElementById('subscribe-cancel-btn')?.addEventListener('click', closeSubscribeModal);
    document.getElementById('subscribe-overlay')?.addEventListener('click', closeSubscribeModal);
    document.getElementById('subscribe-confirm-btn')?.addEventListener('click', () => {
        closeSubscribeModal();
        Toast.show('订阅功能正在建设中，敬请期待！', 'info', 4000);
    });

    document.getElementById('btn-settings')?.addEventListener('click', () => ViewManager.toggleModal('settings-modal'));
    document.querySelectorAll('[data-close-modal]').forEach(el => el.addEventListener('click', () => ViewManager.toggleModal('settings-modal')));

    document.getElementById('btn-logout')?.addEventListener('click', () => Auth.doLogout());
    document.getElementById('btn-save-nickname')?.addEventListener('click', () => Auth.updateNickname());
    document.getElementById('btn-update-password')?.addEventListener('click', () => Auth.updatePassword());

    document.getElementById('btn-create-persona')?.addEventListener('click', () => PersonaCreator.openModal());
    document.querySelectorAll('[data-close-persona-modal]').forEach(el => el.addEventListener('click', () => PersonaCreator.closeModal()));

    document.getElementById('preset-avatars')?.addEventListener('click', e => {
        const btn = e.target.closest('.avatar-option');
        if (btn) PersonaCreator.selectPreset(btn);
    });
    document.getElementById('persona-avatar-upload')?.addEventListener('change', e => PersonaCreator.handleCustomUpload(e.target));
    document.getElementById('remove-custom-avatar')?.addEventListener('click', () => PersonaCreator.removeCustomAvatar());
    document.getElementById('btn-ai-gen-desc')?.addEventListener('click', () => PersonaCreator.generateDesc());
    document.getElementById('persona-desc-input')?.addEventListener('input', e => {
        document.getElementById('desc-char-count').textContent = e.target.value.length;
    });
    document.getElementById('btn-confirm-create-persona')?.addEventListener('click', () => PersonaCreator.confirmCreate());

    document.getElementById('btn-edit-persona')?.addEventListener('click', () => {
        if (AppState.currentPersonaId !== null) PersonaCreator.openEditModal(AppState.currentPersonaId);
    });

    document.getElementById('btn-delete-persona')?.addEventListener('click', async () => {
        if (AppState.currentPersonaId === null) return;
        const persona = AppState.personas.find(p => p.id === AppState.currentPersonaId);
        const ok = await ConfirmDialog.show(
            `删除人设“${persona?.name || ''}”`,
            '删除后将无法恢复，该人设的全部会话和消息也将一并删除。'
        );
        if (!ok) return;
        const res = await API.deletePersona(AppState.currentPersonaId);
        if (res.code !== 0) {
            Toast.show(res.message || '删除失败', 'error');
            return;
        }
        Toast.show('人设已删除', 'success');
        AppState.currentPersonaId = null;
        await SidebarManager.loadPersonas();
    });

    ConfirmDialog.init();

    document.getElementById('mobile-menu-btn')?.addEventListener('click', () => {
        document.getElementById('sidebar')?.classList.toggle('open');
        document.getElementById('sidebar-overlay')?.classList.toggle('hidden');
    });
    document.getElementById('sidebar-overlay')?.addEventListener('click', () => {
        document.getElementById('sidebar')?.classList.remove('open');
        document.getElementById('sidebar-overlay')?.classList.add('hidden');
    });
});

window.switchView = id => ViewManager.switchView(id);
window.toggleModal = id => ViewManager.toggleModal(id);
