#include <Arduino.h>
// Bluetooth用設定
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#include "Secret.h"
BluetoothSerial SerialBT;
// put function declarations here:
// 受信側macアドレス E0:E2:E6:B4:88:D6
// String MACadd = "44:17:93:43:7E:F2";
// uint8_t address[6]  = {0x44, 0x17, 0x93, 0x43, 0x7E, 0xF2};
#include <string>
// #include <iostream>
const int A_IN1 = 25;
const int A_IN2 = 26;
const int BLUE_LED_PIN = 14;
const int LED_FORNT_RIGHT_1PIN = 12;
const int LED_FRONT_RIGHT_2PIN = 13;
const int PBuzzer_PIN = 27;

void motor_motion(int pwm);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  String MACadd = MAC_ADDRESS_STR;
  uint8_t address[6] = {MAC_ADDRESS_BYTE[0], MAC_ADDRESS_BYTE[1], MAC_ADDRESS_BYTE[2], MAC_ADDRESS_BYTE[3], MAC_ADDRESS_BYTE[4], MAC_ADDRESS_BYTE[5]};
  SerialBT.begin("ESP32");
  Serial.println("device start");
  uint8_t macBT[6]; // macアドレス用設定
  esp_read_mac(macBT, ESP_MAC_BT);// macアドレス
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", macBT[0], macBT[1], macBT[2], macBT[3], macBT[4], macBT[5]); // macアドレス表示
  pinMode(BLUE_LED_PIN, OUTPUT); 
  digitalWrite(BLUE_LED_PIN, LOW);
  pinMode(LED_FORNT_RIGHT_1PIN, OUTPUT); 
  digitalWrite(LED_FORNT_RIGHT_1PIN, LOW);
  pinMode(LED_FRONT_RIGHT_2PIN, OUTPUT); 
  digitalWrite(LED_FRONT_RIGHT_2PIN, LOW);
}

// std::string received_data;
String received_data_to_str;
String holding_data  = "nan";
String no_data = "NoData";
// 受信用の構造体の定義
struct ReceiveData {
  int X1;
  int Y1;
  int SW1;
  int X2;
  int Y2;
  int SW2;
  int A_BTN;
  int B_BTN;
  int X_BTN;
  int Y_BTN;
};
// 受信したデータを格納するためのバッファを作成(loopの前に作成するのが適切？)
uint8_t buffer[sizeof(ReceiveData)];
// 構造体の宣言後初期値設定(値保持用)
ReceiveData HoldData = {110, 110, 1, 110, 110, 1, 1, 1, 1, 1};
ReceiveData LastData = {110, 110, 1, 110, 110, 1, 1, 1, 1, 1};
int pwm_value = 110;
const int inputMax = 255;
const int inputMin = 136;
const int outputMax = 255;
const int outputMin = 0;
int communication_count = 0;
int led_flag = 0;
int automatic_progress_mode = 0;
int automatic_mode = 0;

void loop() {
  if (SerialBT.available()) {
    // TODO 受信した時動作(モーター動作等)
    SerialBT.write(1);
    /*値を取得したとき*/
    // Bluetooth通信からデータを受信して受信データをバッファに格納してる
    int bytesReceived = SerialBT.readBytes(buffer, sizeof(buffer));
    if (bytesReceived != sizeof(ReceiveData)) {
      /*受信したデータ量が正しいかチェック、エラー処理*/
      Serial.print("Illegal data");
      Serial.println(holding_data);
    }
    else{
      // 受信データを格納するための構造体作成
      ReceiveData receiveData;
      // 受信データを構造体にコピー
      memcpy(&receiveData, buffer, sizeof(ReceiveData));
      Serial.print("X1:");
      Serial.print(receiveData.X1);
      Serial.print(" Y1:");
      Serial.print(receiveData.Y1);
      Serial.print(" SW1:");
      Serial.print(receiveData.SW1);
      Serial.print(" X2:");
      Serial.print(receiveData.X2);
      Serial.print(" Y2:");
      Serial.print(receiveData.Y2);
      Serial.print(" SW2:");
      Serial.print(receiveData.SW2);
      Serial.print(" A_BTN:");
      Serial.print(receiveData.A_BTN);
      Serial.print(" B_BTN:");
      Serial.print(receiveData.B_BTN);
      Serial.print(" X_BTN:");
      Serial.print(receiveData.X_BTN);
      Serial.print(" Y_BTN:");
      Serial.print(receiveData.Y_BTN);
      // 値途切れた時用の構造体にコピーしている
      HoldData = receiveData;
      pwm_value = receiveData.X1;
      // Aボタンの動作
      if (receiveData.A_BTN == 1 && LastData.A_BTN == 0 && led_flag == 0){
        digitalWrite(LED_FORNT_RIGHT_1PIN, HIGH);
        digitalWrite(LED_FRONT_RIGHT_2PIN, HIGH);
        led_flag = 1;
      }else if (receiveData.A_BTN == 1 && LastData.A_BTN == 0 && led_flag == 1){
        digitalWrite(LED_FORNT_RIGHT_1PIN, LOW);
        digitalWrite(LED_FRONT_RIGHT_2PIN, LOW);
        led_flag = 0;
      }
      // Bボタンの動作
      if (receiveData.B_BTN == 0){
        tone(PBuzzer_PIN,700,50) ;  // buzzer
      }
      // モーター動作モード
      if (automatic_progress_mode == 0){
        motor_motion(pwm_value);
      }else if (automatic_progress_mode == 1){
        motor_motion(75);
      }else if (automatic_progress_mode == 2){
        motor_motion(56);
      }else if (automatic_progress_mode == 3){
        motor_motion(37);
      }else if (automatic_progress_mode == 4){
        motor_motion(18);
      }else if (automatic_progress_mode == 5){
        motor_motion(0);
      }
      
      // 自動運転切り替え(X_BTN)
      if (receiveData.X_BTN == 1 && LastData.X_BTN == 0 && automatic_progress_mode < 5){
        automatic_progress_mode++;
          // if (automatic_mode == 0){
          //   automatic_progress_mode == automatic_progress_mode++;
          //   if (automatic_progress_mode == 5){
          //     automatic_mode = 1;
          //   }
          // }else if (automatic_mode == 1){
          //   automatic_progress_mode == automatic_progress_mode--;
          //   if (automatic_progress_mode == 0){
          //     automatic_mode = 0;
          //   }
          // }
      }else if (receiveData.Y_BTN == 1 && LastData.Y_BTN == 0 && automatic_progress_mode > 0){
        automatic_progress_mode--;
      }
      
      LastData = receiveData;
      digitalWrite(BLUE_LED_PIN, HIGH);
      communication_count = 0;
    }
  }
  else {
    /*値が途切れたとき*/
    communication_count++;
    if (communication_count >= 100){
      digitalWrite(BLUE_LED_PIN, LOW);
      digitalWrite(LED_FORNT_RIGHT_1PIN, LOW);
      digitalWrite(LED_FRONT_RIGHT_2PIN, LOW);
      motor_motion(110);
      HoldData.X1 = 120;
      ESP.restart();
    }
    pwm_value = HoldData.X1;
    motor_motion(pwm_value);
  }
  delay(50);
}

void motor_motion(int pwm){
  // TODO 値を受け取りモータの動作をする関数
  if (pwm >= 105 && pwm <= 135){
    // ブレーキ
    Serial.println("ブレーキ");
    analogWrite(A_IN1, pwm);
    analogWrite(A_IN2, pwm);
  }
  else if (pwm >= inputMin){
    // 前進
    // outputValue = (int)(((double)(analog_read_num - inputMin) / (inputMax - inputMin)) * (outputMax - outputMin) + outputMin);
    pwm = (int)(((double)(pwm - inputMin) / (inputMax - inputMin)) * (outputMax - outputMin) + outputMin);
    analogWrite(A_IN1, pwm);
    analogWrite(A_IN2, 0);
    Serial.print("前進:");
    Serial.println(pwm);
  }
  else if (pwm <= 106){
    // 後進
    pwm = 105 - pwm;
    float t = static_cast<float>(pwm) / 105;
    pwm = t * 255;
    analogWrite(A_IN1, 0);
    analogWrite(A_IN2, pwm);
    Serial.print("後進");
    Serial.println(pwm);
  }
}