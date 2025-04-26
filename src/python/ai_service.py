import os
import json
from dotenv import load_dotenv # 导入 load_dotenv
from openai import OpenAI

script_dir = os.path.dirname(os.path.abspath(__file__))
dotenv_path = os.path.join(script_dir, '.env')

load_dotenv(dotenv_path=dotenv_path)

api_key = os.environ.get("DEEPSEEK_API_KEY")

if not api_key:
    raise ValueError("未找到DEEPSEEK_API_KEY环境变量配置")

client = OpenAI(api_key=api_key, base_url="https://api.deepseek.com/v1")

import json
import sys # 用于打印到 stderr

def process_message(json_string):
    """直接处理消息的核心函数"""
    try:
        data = json.loads(json_string)
        persona = data.get("persona", "默认角色")
        persona_desc = data.get("personaDesc", "你是一个AI助手")
        user_message = data.get("message", "")

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
        return json.dumps({"error": "无效的 JSON 输入"})
    except Exception as e:
        # 返回包含错误的 JSON
        return json.dumps({"error": f"处理消息时出错: {str(e)}"})

