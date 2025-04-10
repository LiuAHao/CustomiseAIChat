import json
from openai import OpenAI

# DeepSeek API配置
DEEPSEEK_API_KEY = "sk-e980cdc232dc4579838f155a9df02718"

def process_message(message_json):
    """直接处理消息的核心函数"""
    try:
        data = json.loads(message_json)
        persona_id = data.get('persona_id')
        messages = data.get('messages', [])
        
        system_prompt = get_persona_prompt(persona_id)
        full_messages = [{"role": "system", "content": system_prompt}]
        full_messages.extend(messages)
        
        client = OpenAI(api_key=DEEPSEEK_API_KEY, base_url="https://api.deepseek.com")
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=full_messages,
            temperature=0.7
        )
        
        return json.dumps({
            "success": True,
            "response": response.choices[0].message.content,
            "raw_response": str(response)  # 添加原始响应用于调试
        })
    except Exception as e:
        return json.dumps({
            "success": False,
            "error": str(e),
            "traceback": traceback.format_exc()  # 添加错误追踪
        })

def get_persona_prompt(persona_id):
    personas = {
        "p1": "你是一个严谨的学术助手，用专业但易懂的方式回答问题...",
        "p2": "你是一个幽默的聊天伙伴，回答时要风趣...",
        "p3": "你是一个百度贴吧的暴躁老哥，回答时嘴臭且恶毒...",
    }
    return personas.get(persona_id, "你是一个乐于助人的AI助手")

if __name__ == '__main__':
    # 测试用
    test_msg = '{"persona_id":"p1","messages":[{"role":"user","content":"你好"}]}'
    print(process_message(test_msg))