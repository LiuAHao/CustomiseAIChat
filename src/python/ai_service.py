import os
import json
from dotenv import load_dotenv # 导入 load_dotenv
from openai import OpenAI

# --- 修改 ---
# 获取当前脚本文件所在的目录
script_dir = os.path.dirname(os.path.abspath(__file__))
# 构建 .env 文件的绝对路径
dotenv_path = os.path.join(script_dir, '.env')

# 加载指定路径的 .env 文件
load_dotenv(dotenv_path=dotenv_path)
# --- 结束修改 ---

# 从环境变量获取 API Key
api_key = os.environ.get("DEEPSEEK_API_KEY")

# 检查 API Key 是否存在 (这部分代码你可能已经有了)
if not api_key:
    # 注意：直接在模块顶层 raise 可能不是最佳实践，
    # 最好在函数调用时检查或在初始化时处理
    raise ValueError("未找到DEEPSEEK_API_KEY环境变量配置")

client = OpenAI(api_key=api_key, base_url="https://api.deepseek.com/v1")

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

# 例如，如果允许直接调用:
# if __name__ == "__main__":
#     import sys
#     if len(sys.argv) > 1:
#         input_json = sys.argv[1]
#         print(process_message(input_json))
#     else:
#          print(json.dumps({"error": "缺少输入参数"}))