# 어
## 실제경로
- 파이썬은 brew로 깐거
- "/opt/homebrew/Cellar/python@3.11/3.11.4/Frameworks/Python.framework/Versions/3.11/bin"의 pip가 찐이다. 딴거는 이상함.
- 그래서 아무리 깔아도 오류가 난 것.

## 성능
- 경로를 출력하면 성능 하락이 있다 (???)
- nohup onweb -p 8000 

- 의문점
    - 비동기 처리를 이용하면 1000번 요청마다 강제로 끊기는 현상 있어.
    - 애초에