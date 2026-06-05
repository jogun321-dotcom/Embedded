// ---------------- 핀 설정 ----------------
const int sw1 = 2;
const int sw2 = 3;
const int sw3 = 4;

const int ledR = 5;
const int ledG = 6;
const int ledB = 7;

// BCD 입력 핀 (74LS47 연결)
const int fndPins[4] = {8, 9, 10, 11}; 

// FND 자리 선택 핀 (공통 애노드 제어)
const int digit1 = 12; // 십의 자리
const int digit2 = 13; // 일의 자리

// ---------------- 상태 변수 ----------------
// 0: 초기화 중, -1: 대기(초기화 상태), 1: 동작1, 2: 동작2, 3: 동작3
int currentState = 0; 
int currentNumber = 50;

// 스위치 길게 누름(1초)을 측정하기 위한 타이머 변수
unsigned long sw1PressTime = 0;
unsigned long sw2PressTime = 0;

void setup() {
  pinMode(sw1, INPUT_PULLUP);
  pinMode(sw2, INPUT_PULLUP);
  pinMode(sw3, INPUT_PULLUP);

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  for (int i = 0; i < 4; i++) pinMode(fndPins[i], OUTPUT);
  pinMode(digit1, OUTPUT);
  pinMode(digit2, OUTPUT);
}

void loop() {
  // 현재 상태에 따라 적절한 함수를 무한 반복 실행
  if (currentState == 0) runInit();
  else if (currentState == -1) runIdle();
  else if (currentState == 1) runAction1();
  else if (currentState == 2) runAction2();
  else if (currentState == 3) runAction3();
}

// ---------------- FND & LED 제어 핵심 함수 ----------------
// 딜레이 없이 아주 짧은 시간(약 4ms) 동안 FND를 번갈아 켜주는 함수
void refreshFND(int num) {
  if (num < 0) { // 음수면 FND 소등
    digitalWrite(digit1, LOW);
    digitalWrite(digit2, LOW);
    delayMicroseconds(4000); 
    return;
  }
  
  int tens = num / 10;
  int ones = num % 10;
  
  // 십의 자리 출력
  for(int i=0; i<4; i++) digitalWrite(fndPins[i], (tens >> i) & 1);
  digitalWrite(digit1, HIGH); digitalWrite(digit2, LOW);
  delayMicroseconds(2000);
  
  // 일의 자리 출력
  for(int i=0; i<4; i++) digitalWrite(fndPins[i], (ones >> i) & 1);
  digitalWrite(digit1, LOW); digitalWrite(digit2, HIGH);
  delayMicroseconds(2000);
  
  // 잔상 방지
  digitalWrite(digit1, LOW); digitalWrite(digit2, LOW);
}

// RGB LED 색상을 쉽게 지정하는 함수
void setRGB(int r, int g, int b) {
  digitalWrite(ledR, r);
  digitalWrite(ledG, g);
  digitalWrite(ledB, b);
}

// 정해진 시간(ms) 동안 FND와 LED를 유지하며 시간을 보내는 함수 (스위치 무시)
void waitWithFND(int num, unsigned long ms, int r, int g, int b) {
  setRGB(r, g, b);
  unsigned long start = millis();
  while(millis() - start < ms) {
    refreshFND(num);
  }
}

// ---------------- 초기화 동작 (상태 0) ----------------
void runInit() {
  // FND 88/소등, RGB 녹/청 순차 점멸 (1초 간격 2회 반복)
  waitWithFND(88, 1000, LOW, HIGH, LOW); // FND 88, 녹색
  waitWithFND(-1, 1000, LOW, LOW, HIGH); // FND 소등, 청색
  waitWithFND(88, 1000, LOW, HIGH, LOW); 
  waitWithFND(-1, 1000, LOW, LOW, HIGH); 

  // 완료 후 초기화 상태(대기) 유지
  currentNumber = 50;
  setRGB(HIGH, LOW, LOW); // 적색
  currentState = -1; // 대기 상태로 전환
}

// ---------------- 대기 상태 (초기화 완료 후 상태 -1) ----------------
void runIdle() {
  refreshFND(currentNumber); // 숫자 유지

  // SW3 누르면 즉시 초기화
  if (digitalRead(sw3) == LOW) {
    currentState = 0; 
    sw1PressTime = 0; sw2PressTime = 0;
    return;
  }

  // SW1 1초 이상 길게 누름 감지
  if (digitalRead(sw1) == LOW) {
    if (sw1PressTime == 0) sw1PressTime = millis(); // 처음 눌린 시간 기록
    else if (millis() - sw1PressTime >= 1000) {     // 1초 경과 확인
      currentState = 1;
      sw1PressTime = 0;
    }
  } else {
    sw1PressTime = 0; // 스위치 떼면 타이머 초기화
  }

  // SW2 1초 이상 길게 누름 감지
  if (digitalRead(sw2) == LOW) {
    if (sw2PressTime == 0) sw2PressTime = millis();
    else if (millis() - sw2PressTime >= 1000) {
      currentState = 2;
      sw2PressTime = 0;
    }
  } else {
    sw2PressTime = 0;
  }
}

// ---------------- 동작 1 (상태 1) ----------------
void runAction1() {
  unsigned long lastTick = millis();
  bool ledOn = true;
  setRGB(LOW, LOW, HIGH); // B(청색) 점등 시작

  unsigned long zeroReachTime = 0; // 00 도달 후 유지 시간 측정용

  // 누르고 있는 동안 무한 반복
  while(true) {
    refreshFND(currentNumber);

    // 1. SW1을 떼는 순간 동작 즉시 종료
    if (digitalRead(sw1) == HIGH) {
      setRGB(HIGH, LOW, LOW); // R(적색) 유지
      currentState = -1; // 대기 상태로
      return;
    }

    // 2. 숫자가 00에 도달 후 1초 추가 유지 시 동작 3으로 전환
    if (currentNumber == 0) {
      if (zeroReachTime == 0) zeroReachTime = millis();
      else if (millis() - zeroReachTime >= 1000) {
        currentState = 3; 
        return;
      }
    }

    // 3. 0.5초마다 숫자 감소 및 B LED 점멸
    if (millis() - lastTick >= 500) {
      lastTick = millis();
      
      if (currentNumber > 0) { // 0까지만 감소
        currentNumber--;
        zeroReachTime = 0; // 숫자가 줄어드는 중이면 타이머 리셋
      }
      
      ledOn = !ledOn;
      if (ledOn) setRGB(LOW, LOW, HIGH); // B 켜기
      else setRGB(LOW, LOW, LOW);        // 소등
    }
  }
}

// ---------------- 동작 2 (상태 2) ----------------
void runAction2() {
  unsigned long lastTick = millis();
  bool ledOn = true;
  setRGB(LOW, HIGH, LOW); // G(녹색) 점등 시작

  unsigned long ninetyNineReachTime = 0; // 99 도달 후 유지 시간 측정용

  while(true) {
    refreshFND(currentNumber);

    // 1. SW2를 떼는 순간 동작 즉시 종료
    if (digitalRead(sw2) == HIGH) {
      setRGB(HIGH, LOW, LOW); // R(적색) 유지
      currentState = -1;
      return;
    }

    // 2. 숫자가 99에 도달 후 1초 추가 유지 시 동작 3으로 전환
    if (currentNumber == 99) {
      if (ninetyNineReachTime == 0) ninetyNineReachTime = millis();
      else if (millis() - ninetyNineReachTime >= 1000) {
        currentState = 3;
        return;
      }
    }

    // 3. 0.5초마다 숫자 증가 및 G LED 점멸
    if (millis() - lastTick >= 500) {
      lastTick = millis();
      
      if (currentNumber < 99) { // 99까지만 증가
        currentNumber++;
        ninetyNineReachTime = 0;
      }
      
      ledOn = !ledOn;
      if (ledOn) setRGB(LOW, HIGH, LOW); // G 켜기
      else setRGB(LOW, LOW, LOW);        // 소등
    }
  }
}

// ---------------- 동작 3 (상태 3) ----------------
void runAction3() {
  // 스위치 입력 무시하고 지정된 시퀀스 실행
  waitWithFND(0,  1000, HIGH, LOW, LOW); // FND 00, R(적) 1초
  waitWithFND(99, 1000, LOW, HIGH, LOW); // FND 99, G(녹) 1초
  waitWithFND(0,  1000, LOW, LOW, HIGH); // FND 00, B(청) 1초
  waitWithFND(99, 1000, LOW, LOW, LOW);  // FND 99, 소등 1초

  // 수행 완료 후 [초기화 상태]로 복귀
  currentNumber = 50;
  setRGB(HIGH, LOW, LOW); // R(적색)
  currentState = -1; 
}