import json
from flask import Flask, request, jsonify, render_template
from openai import OpenAI
import os

app = Flask(__name__, template_folder='templates')

# DeepSeek API配置
DEEPSEEK_API_KEY = "sk-e980cdc232dc4579838f155a9df02718"
DEEPSEEK_API_URL = "https://api.deepseek.com/v1/chat/completions"

# 确保模板目录存在
os.makedirs('templates', exist_ok=True)

# 核心API接口 - 供C++服务器调用
@app.route('/api/v1/chat', methods=['POST'])
def chat_api():
    """核心API接口，保持与C++服务器的兼容性"""
    try:
        data = request.get_json()
        persona_id = data.get('persona_id')
        messages = data.get('messages', [])
        
        system_prompt = get_persona_prompt(persona_id)
        full_messages = [{"role": "system", "content": system_prompt}]
        full_messages.extend(messages)
        
        client = OpenAI(
            api_key=DEEPSEEK_API_KEY,
            base_url="https://api.deepseek.com"
        )
        
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=full_messages,
            temperature=0.7,
            stream=False
        )
        
        ai_response = response.choices[0].message.content
        
        # 确保返回的JSON使用UTF-8编码
        return jsonify({
            "success": True,
            "response": ai_response
        }), 200, {'Content-Type': 'application/json; charset=utf-8'}
    except Exception as e:
        return jsonify({
            "success": False,
            "error": str(e)
        }), 500

# 测试页面路由
@app.route('/test', methods=['GET'])
def test_page():
    """独立测试页面"""
    return render_template('chat_test.html')

# 测试页面API
@app.route('/test/api/chat', methods=['POST'])
def test_chat():
    """仅供测试页面使用的API"""
    try:
        data = request.get_json()
        # 调用核心API
        with app.test_request_context('/api/v1/chat', method='POST', json=data):
            return chat_api()
    except Exception as e:
        return jsonify({"error": str(e)}), 500

def get_persona_prompt(persona_id):
    """
    根据persona_id获取预设prompt
    这里可以连接您的数据库或从文件读取
    """
    # 示例实现 - 实际应从数据库获取
    personas = {
        "p1": "你是一个严谨的学术助手，用专业但易懂的方式回答问题...",
        "p2": "你是一个幽默的聊天伙伴，回答时要风趣...",
        "p3": "你是一个百度贴吧的暴躁老哥，回答时嘴臭且恶毒...",
    }
    return personas.get(persona_id, "你是一个乐于助人的AI助手")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)