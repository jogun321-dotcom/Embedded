// ---------------- 핀 설정 ----------------
const int sw1 = 2;
const int sw2 = 3;
const int sw3 = 4;

const int ledR = 5;
const int ledG = 6;
const int ledB = 7;

// BCD 입력 핀 (74LS47 연결: A, B, C, D)
const int fndPins[4] = {8, 9, 10, 11}; 

// FND 자리 선택 핀 (Common Anode 기준, HIGH일 때 켜짐)
const int digit1 = 13; // 십의 자리
const int digit2 = 12; // 일의 자리

// ---------------- 상태 변수 ----------------
int currentState = 0; // 0: 초기화시퀀스, -1: 초기화완료(대기), 1: 동작1, 2: 동작2
int currentNumber = 0;

// 스위치 눌렀다 떼는 시점(Edge) 감지를 위한 이전 상태 저장
bool lastSw1 = HIGH;
bool lastSw2 = HIGH;

void setup() {
  pinMode(sw1, INPUT_PULLUP);
  pinMode(sw2, INPUT_PULLUP);
  pinMode(sw3, INPUT_PULLUP);

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  for (int i = 0; i < 4; i++) {
    pinMode(fndPins[i], OUTPUT);
  }
  pinMode(digit1, OUTPUT);
  pinMode(digit2, OUTPUT);
}

void loop() {
  if (currentState == 0) {
    runInit();
  } else if (currentState == -1) { // 초기화 완료 후 대기 상태
    int ev = waitForEvent(0, 100, false); // 00 표시 유지
    if (ev == 3) currentState = 0;
    else if (ev == 1) currentState = 1;
    else if (ev == 2) currentState = 2;
  } else if (currentState == 1) {
    runAction1();
  } else if (currentState == 2) {
    runAction2();
  }
}

// ---------------- 편의 함수: RGB LED 제어 ----------------
void setRGB(int r, int g, int b) {
  digitalWrite(ledR, r);
  digitalWrite(ledG, g);
  digitalWrite(ledB, b);
}

// ---------------- 핵심 함수: 시간 대기 + FND 출력 + 스위치 감지 ----------------
// durationMs 만큼 시간을 보내며 FND를 갱신합니다. 스위치가 눌리면 즉시 해당 번호(1,2,3)를 반환합니다.
int waitForEvent(int num, unsigned long durationMs, bool ignoreInput) {
  unsigned long start = millis();
  int triggeredEvent = 0;
  
  while (millis() - start < durationMs) {
    // 1. FND 멀티플렉싱 출력
    if (num >= 0) {
      int tens = num / 10;
      int ones = num % 10;
      
      // 십의 자리 출력
      for (int i = 0; i < 4; i++) digitalWrite(fndPins[i], (tens >> i) & 1);
      digitalWrite(digit1, HIGH); 
      digitalWrite(digit2, LOW);
      delay(2);
      
      // 일의 자리 출력
      for (int i = 0; i < 4; i++) digitalWrite(fndPins[i], (ones >> i) & 1);
      digitalWrite(digit1, LOW);
      digitalWrite(digit2, HIGH);
      delay(2);
      
      // 잔상 방지
      digitalWrite(digit1, LOW);
      digitalWrite(digit2, LOW);
    } else {
      // num이 음수면 FND 소등
      digitalWrite(digit1, LOW);
      digitalWrite(digit2, LOW);
      delay(4);
    }

    // 2. 스위치 입력 감지 (무시 모드가 아닐 때만)
    if (!ignoreInput && triggeredEvent == 0) {
      // SW3: 누르는 즉시 감지 (LOW)
      if (digitalRead(sw3) == LOW) {
        return 3; 
      }
      
      // SW1: 눌렀다 떼는 순간 감지 (LOW -> HIGH)
      bool currSw1 = digitalRead(sw1);
      if (lastSw1 == LOW && currSw1 == HIGH) {
        lastSw1 = currSw1;
        return 1;
      }
      lastSw1 = currSw1;
      
      // SW2: 눌렀다 떼는 순간 감지 (LOW -> HIGH)
      bool currSw2 = digitalRead(sw2);
      if (lastSw2 == LOW && currSw2 == HIGH) {
        lastSw2 = currSw2;
        return 2;
      }
      lastSw2 = currSw2;
    }
  }
  return 0; // 스위치 입력 없이 지정된 시간이 지남
}

// ---------------- 초기화 동작 ----------------
void runInit() {
  // 1초 단위로 4초간 진행 (FND 88 점멸과 RGB 순차 점등이 동시에 일어남)
  setRGB(HIGH, LOW, LOW); // R 점등
  waitForEvent(88, 1000, true); 
  
  setRGB(LOW, HIGH, LOW); // G 점등
  waitForEvent(-1, 1000, true); // FND 소등
  
  setRGB(LOW, LOW, HIGH); // B 점등
  waitForEvent(88, 1000, true);
  
  setRGB(LOW, LOW, LOW); // RGB 소등
  waitForEvent(-1, 1000, true);

  // 완료 후 상태 유지
  currentState = -1; // 초기화 완료 대기 상태로 변경
  setRGB(HIGH, LOW, LOW); // R 점등 유지
}

// ---------------- 동작 1 ----------------
void runAction1() {
  // 진입 동작 (FND 88 2초 유지, RGB G 0.5초 -> R 0.5초 -> 이후 1초 소등)
  setRGB(LOW, HIGH, LOW);
  if (handleAction1Event(waitForEvent(88, 500, false), 88)) return;
  
  setRGB(HIGH, LOW, LOW);
  if (handleAction1Event(waitForEvent(88, 500, false), 88)) return;
  
  setRGB(LOW, LOW, LOW);
  if (handleAction1Event(waitForEvent(88, 1000, false), 88)) return;

  // 메인 반복 루프 (00~99 증가)
  currentNumber = 0;
  while (true) {
    setRGB(LOW, LOW, HIGH); // B 점등
    if (handleAction1Event(waitForEvent(currentNumber, 500, false), currentNumber)) return;
    
    currentNumber = (currentNumber + 1) % 100; // 99 다음은 00
    
    setRGB(LOW, LOW, LOW); // B 소등
    if (handleAction1Event(waitForEvent(currentNumber, 500, false), currentNumber)) return;
    
    currentNumber = (currentNumber + 1) % 100;
  }
}

// 동작 1 중 발생하는 이벤트 처리기
bool handleAction1Event(int ev, int num) {
  if (ev == 3) { currentState = 0; return true; } // 초기화로 이동
  if (ev == 2) { currentState = 2; return true; } // 동작 2로 즉시 전환
  if (ev == 1) { runAction3(num); return false; } // 동작 3(일시정지) 수행 후 계속 진행
  return false;
}

// ---------------- 동작 2 ----------------
void runAction2() {
  // 진입 동작
  setRGB(LOW, LOW, HIGH); // B 점등
  if (handleAction2Event(waitForEvent(88, 500, false), 88)) return;
  
  setRGB(HIGH, LOW, LOW); // R 점등
  if (handleAction2Event(waitForEvent(88, 500, false), 88)) return;
  
  setRGB(LOW, LOW, LOW);
  if (handleAction2Event(waitForEvent(88, 1000, false), 88)) return;

  // 메인 반복 루프 (99~00 감소)
  currentNumber = 99;
  while (true) {
    setRGB(LOW, HIGH, LOW); // G 점등
    if (handleAction2Event(waitForEvent(currentNumber, 500, false), currentNumber)) return;
    
    currentNumber = (currentNumber == 0) ? 99 : currentNumber - 1; // 0 다음은 99
    
    setRGB(LOW, LOW, LOW); // G 소등
    if (handleAction2Event(waitForEvent(currentNumber, 500, false), currentNumber)) return;
    
    currentNumber = (currentNumber == 0) ? 99 : currentNumber - 1;
  }
}

// 동작 2 중 발생하는 이벤트 처리기
bool handleAction2Event(int ev, int num) {
  if (ev == 3) { currentState = 0; return true; } // 초기화로 이동
  if (ev == 1) { currentState = 1; return true; } // 동작 1로 즉시 전환
  if (ev == 2) { runAction3(num); return false; } // 동작 3(일시정지) 수행 후 계속 진행
  return false;
}

// ---------------- 동작 3 (일시정지) ----------------
void runAction3(int num) {
  // 2초 점등, 2초 소등 2회 반복 (이때 스위치 입력은 모두 무시)
  for (int i = 0; i < 2; i++) {
    setRGB(HIGH, LOW, LOW); // R 점등
    waitForEvent(num, 2000, true);
    
    setRGB(LOW, LOW, LOW); // RGB 소등
    waitForEvent(-1, 2000, true); // FND 소등
  }
  // 함수가 종료되면 호출했던 곳(동작1 또는 2)으로 자연스럽게 돌아가 이전 동작을 이어서 수행함.
}