// --- THÔNG TIN BLYNK ---
#define BLYNK_TEMPLATE_ID "TMPL60TG7FblT"
#define BLYNK_TEMPLATE_NAME "ThanhTuan148"
#define BLYNK_AUTH_TOKEN "SQ9cgTUWESGDNaYHxZbuT6VKmKnkneSO"

// --- THƯ VIỆN ---
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

// --- CẤU HÌNH PHẦN CỨNG & WIFI---
char ssid[] = "Wokwi-GUEST"; char pass[] = "";
#define NUM_SERVOS 6
int servoPins[NUM_SERVOS] = {13, 12, 14, 27, 26, 25};
#define MODE_BUTTON_PIN 15         // Nút Đỏ
#define SELECT_BUTTON_PIN 18  // Nút Xanh Dương
#define ACTION_BUTTON_PIN 4        // Nút Xanh Lá
#define SERVO_ANGLE_CLOSED 0
#define SERVO_ANGLE_OPEN 90

// --- KHỞI TẠO CÁC ĐỐI TƯỢNG ---
Servo myServos[NUM_SERVOS];
BlynkTimer timer;

// Enum đơn giản cho 2 chế độ
enum OperationMode { MANUAL, AUTO_LDR };
OperationMode currentMode = MANUAL; // Bắt đầu ở chế độ Thủ công

// Biến trạng thái
bool doorStates[NUM_SERVOS] = {false};
int selectedTarget = 6; // 6 = Tất cả


// ---------------- CÁC HÀM ĐIỀU KHIỂN CƠ BẢN ----------------
void setDoorState(int doorIndex, bool open) {
  if (doorIndex < 0 || doorIndex >= NUM_SERVOS) return;
  // Chỉ thực hiện hành động nếu trạng thái thay đổi
  if (doorStates[doorIndex] != open) {
    myServos[doorIndex].write(open ? SERVO_ANGLE_OPEN : SERVO_ANGLE_CLOSED);
    doorStates[doorIndex] = open;
    Serial.printf("  -> Kết quả: Cửa %d đã %s\n", doorIndex + 1, open ? "Mở" : "Đóng");
    Blynk.virtualWrite(V10 + doorIndex, open); // Cập nhật trạng thái lên Blynk
  }
}

void setAllDoorsState(bool open) {
  // Chỉ thực hiện hành động nếu trạng thái thay đổi
  if (doorStates[0] != open) {
    Serial.printf("  -> Kết quả: %s TẤT CẢ cửa.\n", open ? "Mở" : "Đóng");
    for (int i = 0; i < NUM_SERVOS; i++) {
      myServos[i].write(open ? SERVO_ANGLE_OPEN : SERVO_ANGLE_CLOSED);
      doorStates[i] = open;
      Blynk.virtualWrite(V10 + i, open); // Cập nhật trạng thái lên Blynk
    }
  }
}

// ---------------- HÀM ĐIỀU KHIỂN CHÍNH ----------------
void mainController() {
  if (currentMode != AUTO_LDR) {
    return;
  }
  
  int ldrValue = analogRead(34);
  bool shouldBeOpen = ldrValue > 2000;

  if (selectedTarget == 6) {
    setAllDoorsState(shouldBeOpen);
  } else {
    setDoorState(selectedTarget, shouldBeOpen);
  }
}


// ----------- HÀM KIỂM TRA NÚT NHẤN-----------
void checkHardwareButtons() {
  // Nút Đỏ: Chuyển chế độ
  if (digitalRead(MODE_BUTTON_PIN) == LOW) {
    delay(250);
    currentMode = (currentMode == MANUAL) ? AUTO_LDR : MANUAL;
    if (currentMode == MANUAL) {
      Serial.println("\n### CHẾ ĐỘ: THỦ CÔNG ###");
      Blynk.virtualWrite(V1, 1);
    } else {
      Serial.println("\n### CHẾ ĐỘ: TỰ ĐỘNG - ÁNH SÁNG ###");
      Blynk.virtualWrite(V1, 0);
    }
  }

  // Nút Xanh Dương: Chọn cửa (luôn hoạt động)
  if (digitalRead(SELECT_BUTTON_PIN) == LOW) {
    delay(250);
    selectedTarget = (selectedTarget + 1) % (NUM_SERVOS + 1);
    const char* targetName = (selectedTarget == 6) ? "TẤT CẢ CÁC CỬA" : ("Cửa " + String(selectedTarget + 1)).c_str();
    if (currentMode == MANUAL) {
        Serial.printf(">> Điều khiển thủ công: %s\n", targetName);
    } else {
        Serial.printf(">> Đối tượng tự động: %s\n", targetName);
    }
  }

  // Nút Xanh Lá: Hành động (chỉ hoạt động ở chế độ thủ công)
  if (currentMode == MANUAL && digitalRead(ACTION_BUTTON_PIN) == LOW) {
    delay(250);

    // In ra đối tượng đang được điều khiển trước khi hành động
    if (selectedTarget == 6) {
        Serial.print("Hành động trên: [TẤT CẢ CÁC CỬA]");
        setAllDoorsState(!doorStates[0]);
    } else {
        Serial.printf("Hành động trên: [Cửa %d]", selectedTarget + 1);
        setDoorState(selectedTarget, !doorStates[selectedTarget]);
    }
  }
}


// ---------------- CÁC HÀM BLYNK ----------------
BLYNK_CONNECTED() {
  Blynk.syncAll();
  Blynk.virtualWrite(V1, currentMode == MANUAL);
}

BLYNK_WRITE(V1) {
  bool isBlynkManual = param.asInt();
  if (isBlynkManual && currentMode != MANUAL) {
    currentMode = MANUAL;
    Serial.println("\n### CHẾ ĐỘ: THỦ CÔNG (từ Blynk) ###");
  } else if (!isBlynkManual && currentMode != AUTO_LDR) {
    currentMode = AUTO_LDR;
    Serial.println("\n### CHẾ ĐỘ: TỰ ĐỘNG - ÁNH SÁNG (từ Blynk) ###");
  }
}

BLYNK_WRITE(V2) {
  if (currentMode == MANUAL) {
    setAllDoorsState(param.asInt());
  }
}

BLYNK_WRITE_DEFAULT() {
  if (currentMode == MANUAL) {
    int pin = request.pin;
    if (pin >= 10 && pin < 10 + NUM_SERVOS) {
      setDoorState(pin - 10, param.asInt());
    }
  }
}

void sendSensorData() {
    Blynk.virtualWrite(V0, analogRead(34));
}

// ----------- HÀM SETUP VÀ LOOP -----------
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" Connected!");

  for (int i = 0; i < NUM_SERVOS; i++) myServos[i].attach(servoPins[i]);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ACTION_BUTTON_PIN, INPUT_PULLUP);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);

  timer.setInterval(2000L, sendSensorData);
  timer.setInterval(250L, checkHardwareButtons);
  timer.setInterval(500L, mainController);
}

void loop() {
  Blynk.run();
  timer.run();
}