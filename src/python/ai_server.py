import os
import json
import base64
from flask import Flask, request, jsonify
from dotenv import load_dotenv
from openai import OpenAI

# 配置环境
script_dir = os.path.dirname(os.path.abspath(__file__))
dotenv_path = os.path.join(script_dir, '.env')
load_dotenv(dotenv_path=dotenv_path)

api_key = os.environ.get("DEEPSEEK_API_KEY")
if not api_key:
    raise ValueError("未找到DEEPSEEK_API_KEY环境变量配置")

client = OpenAI(api_key=api_key, base_url="https://api.deepseek.com/v1")

# 创建Flask应用
app = Flask(__name__)

def process_message(json_string):
    """处理消息的核心函数"""
    try:
        data = json.loads(json_string)
        persona = data.get("persona", "默认角色")
        persona_desc = data.get("personaDesc", "你是一个AI助手")
        user_message = data.get("message", "")

        print(f"用户消息: {persona} - {user_message[:50]}...")

        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=[
                {"role": "system", "content": persona_desc},
                {"role": "user", "content": user_message},
            ],
            stream=False
        )
        ai_response = response.choices[0].message.content

        # 返回包含响应的 JSON
        return json.dumps({"response": ai_response})

    except json.JSONDecodeError:
        return json.dumps({"error": "无效的JSON输入"})
    except Exception as e:
        print(f"错误: {str(e)}")
        return json.dumps({"error": f"处理消息时出错: {str(e)}"})

@app.route('/process', methods=['POST'])
def handle_process():
    """处理POST请求"""
    try:
        # 获取base64编码的消息
        base64_msg = request.form.get('message', '')
        if not base64_msg:
            return jsonify({"error": "未提供消息"}), 400
        
        # 解码base64消息
        json_string = base64.b64decode(base64_msg).decode('utf-8')
        
        # 处理消息
        result = process_message(json_string)
        
        return result, 200, {'Content-Type': 'application/json'}
    
    except Exception as e:
        print(f"错误: {str(e)}")
        return jsonify({"error": f"处理请求时出错: {str(e)}"}), 500

if __name__ == '__main__':
    # 在本地端口5001上运行服务
    print("AI服务已启动，监听端口5001...")
    app.run(host='127.0.0.1', port=5001) 