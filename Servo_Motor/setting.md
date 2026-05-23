SG90 선 색상STM32F103RB 핀설명

빨간색 (VCC)5V모터  : 전원 (반드시 5V 사용)

갈색/검은색 (GND)GND :  그라운드

주황색/노란색 (PWM)PA6 (TIM3_CH1) : 각도 제어용 PWM 신호 출력

왼쪽 메뉴에서 Timers > TIM3를 클릭합니다.
Clock Source를 Internal Clock으로 설정합니다.
Channel1을 PWM Generation CH1으로 설정합니다. 
(이때 우측 칩 뷰어에서 PA6 핀이 초록색으로 활성화되는지 확인합니다.)

2.타이머 파라미터 설정:50Hz(20ms) 주기 만들기.
Configuration > Parameter Settings에서 아래와 같이 입력합니다. 
(시스템 클럭 72MHz 기준)Prescaler (PSC): 72 - 1 (1MHz로 분주, 1 카운트 = 1µs)Counter Period (ARR): 20000 - 1 (20000µs = 20ms 주기 완성)
