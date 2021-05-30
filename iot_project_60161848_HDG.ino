#include<ESP8266WiFi.h>
#include<PubSubClient.h>
#include<ESP8266HTTPClient.h>
#include<ArduinoJson.h>
#include<Wire.h>
#include <AnotherIFTTTWebhook.h>

#define mpu_add 0x68 //mpu6050 mpu_add

int16_t ac_x, ac_y, ac_z, gy_x, gy_y, gy_z; //acc, gyro data 
int16_t old_ac_x, old_ac_y, old_ac_z, old_gy_x, old_gy_y, old_gy_z; //acc, gyro data 
double angle = 0, deg, old_angle=0; // angle, deg data
double dgy_x; //double type acc data
char Temp[10], Acz[10], httpADD[100];
float temp;
int step_num = 0;

WiFiClient tcpClient;
PubSubClient mqttClient;
HTTPClient myClient;
DynamicJsonDocument buf(2048);//json data를 저장할 임시버퍼 생성

void setup() {
  Wire.begin(4, 5);
  Wire.beginTransmission(mpu_add);
  Wire.write(0x6B);
  Wire.write(1); //슬립모드 해제
  Wire.endTransmission(true);

  /*full scale range 16g*/
  Wire.beginTransmission(mpu_add);
  Wire.write(0x1C);
  Wire.write(0x18);
  Wire.endTransmission(true);
  
  /*full scale range 2000'/s*/
  Wire.beginTransmission(mpu_add);
  Wire.write(0x1B);
  Wire.write(0x18);
  Wire.endTransmission(true);
  
  Serial.begin(115200);
  
  WiFi.begin("SK_WiFiDCC8","1104058454");
  while(1)
  {
    if (WiFi.status()==WL_CONNECTED)
      break;
    else
      Serial.printf("wait...\r\n");
      delay(1000);  
  }
  Serial.printf("\r\nWiFi is connected!!\r\n");
}

int readDHT11(int *readTemp, int *readHumid)
{
  int dt[82] = {0,};//40bit + 1bit 이고 한 bit에 0과 1이 들어가므로 82개, 전부 0으로 채움
  
  //페이즈 1
  digitalWrite(14, 1);
  pinMode(14, OUTPUT);
  delay(1);
  digitalWrite(14, 0);
  delay(20);
  pinMode(14, INPUT_PULLUP);

  //페이즈 2~3
  while(1)//1페이즈 후 data라인이 1이 되는 것을 기다림
  {
    if(digitalRead(14) == 1)  break;
  }
  while(1)
  {
    if(digitalRead(14) == 0)  break;  
  }
  
  int count = 0;
  for(count = 0; count<41; count++)
  {
    /*count가 0일때 페이즈 2, 이후로는 페이즈 3에서 소요된 시간이 dt에 저장된다*/
    dt[count*2] = micros(); // 이전시간
    while(1)
      if(digitalRead(14) == 1)  break;
    dt[count*2] = micros() - dt[count*2];//0을 보낸 시간

    dt[count*2+1] = micros();//이전시간
    while(1)
      if(digitalRead(14) == 0)  break;
    dt[count*2+1] = micros() - dt[count*2+1];//1을 보낸 시간
  }

  //페이즈 4
  /*습도값 읽기*/
  *readHumid = 0;
  for(count = 1; count < 9; count++)
  {
    *readHumid = *readHumid << 1;//1비트씩 8번 밀어서 8비트 크기 데이터를 만듬
    if(dt[count*2+1] > 49)  
    {
      *readHumid = *readHumid + 1;
    }
    else
    {
      *readHumid = *readHumid + 0;
    }
    //Serial.printf("humid: %d\r\n", *readHumid);
  }
  /*온도값 읽기*/
  for(count = 17; count < 25; count++)
  {
    *readTemp = *readTemp << 1;//1비트씩 8번 밀어서 8비트 크기 데이터를 만듬
    if(dt[count*2+1] > 49)  
    {
      *readTemp = *readTemp + 1;
    }
    else
    {
      *readTemp = *readTemp + 0;
    }  
    //Serial.printf("temp: %d\r\n", *readTemp); 
  }

  return 1;
}

int readTemp, readHumid;
int mating = 0, webhook_count = 0, fire_count = 0;
unsigned long long MS;//64bit
#define DELAY_MS  400
void loop() {

  // put your main code here, to run repeatedly:
  /*mpu6050 data read*/
  Wire.beginTransmission(mpu_add); //get acc data 
  Wire.write(0x3B);  
  Wire.endTransmission(false);  
  Wire.requestFrom(mpu_add, 6, true);
  
  ac_x = Wire.read() << 8 | Wire.read(); 
  ac_y = Wire.read() << 8 | Wire.read();
  ac_z = Wire.read() << 8 | Wire.read();

  Wire.beginTransmission(mpu_add); //get gyro data
  Wire.write(0x43); 
  Wire.endTransmission(false);
  Wire.requestFrom(mpu_add, 6, true);
  
  gy_x = Wire.read() << 8 | Wire.read();
  gy_y = Wire.read() << 8 | Wire.read();
  gy_z = Wire.read() << 8 | Wire.read();
  
  deg = atan2(ac_x, ac_z) * 180 / PI;  //rad to deg
  
  dgy_x = gy_y / 131.;  //16-bit data to 250 deg/sec
  
  angle = (0.95 * (angle + (dgy_x * 0.001))) + (0.05 * deg); //complementary filter
    
  if(millis() - MS >= DELAY_MS)
  {
    MS = millis();
    
    /*다리를 앞으로 움직이면, 걸음 수를 카운트*/
    if((angle > -60) && (ac_x > -1700) && (ac_z > 1200))
    {
      if((angle - old_angle > 30) && (ac_x - old_ac_x > 400) && (ac_z - old_ac_z > 1100)) 
      {
        step_num =  step_num + 1;
      }
    }
    
    /*기승위 자세 인식*/
    if((angle > -40) && (ac_x > -1400) && (ac_z > 1700))
    {
        mating = mating + 1;
        if(mating == 3)
        {
          //나중에는 printf문 대신에 핸드폰으로 신호가 가도록 바꿀 예정
          send_webhook("mating_on", "0dzpHnFyxhAziHqOzRCPQ", "", "","");
          //Serial.printf("현재 발정기가 의심됩니다.\r\n");
          mating = 0;  
        }
    }

    old_angle = angle;
    old_ac_x = ac_x;
    old_ac_z = ac_z;
    /*
    Serial.printf("angle: "); Serial.println(angle);
    Serial.printf("ac_x = "); Serial.println(ac_x);
    Serial.printf("ac_y = "); Serial.println(ac_y);
    Serial.printf("ac_z = "); Serial.println(ac_z);
    Serial.printf("gy_x = "); Serial.println(gy_x);
    Serial.printf("gy_y = "); Serial.println(gy_y);
    Serial.printf("gy_z = "); Serial.println(gy_z);
    */
    Serial.printf("step_num = "); Serial.println(step_num);
    /*api로 날씨 읽어오기*/
    myClient.begin("http://api.openweathermap.org/data/2.5/weather?q=seoul&appid=a69fbbf85426b1fafba406fb029a1ce0");
    //Serial.printf("now start!!\r\n");
    if(myClient.GET() == HTTP_CODE_OK)//서버와의 연결 상태를 받아옴. HTTP_CODE_OK는 200을 의미함
    {
      String data = myClient.getString();//실제 데이터를 받아온다
      //Serial.printf("%s\r\n\r\nEND\r\n", data.c_str());  
      deserializeJson(buf, data);//json data를 해석 완료
      float temp = buf["main"]["temp"];//버퍼에서 온도 data를 변수에 저장
      int hum = buf["main"]["humidity"];
      const char* city = buf["name"];
      const char* weather = buf["weather"][0]["main"];
      Serial.printf("현재 %s의 날씨는 %s 입니다.\r\n", city, weather);
      Serial.printf("온도: %f의\r\n습도: %d\r\n", temp-273.0, hum);

      char* weather_data = (char*)weather;//send_webhook 함수에 const char*가 불가능 해서 타입을 바꿔줌
      //weather_data = "Snow";
      if(strcmp(weather_data,"Snow")==0)
      {
        Serial.print("현재 눈이 내리고 있습니다.\r\n");
        if(webhook_count == 0)
        {
          send_webhook("hdg_test", "0dzpHnFyxhAziHqOzRCPQ", weather_data, "","");
        }
        webhook_count++;
      }
      else if(strcmp(weather_data,"Rain")==0)
      {
        Serial.print("현재 비가 내리고 있습니다.\r\n");
        if(webhook_count == 0)
        {
          send_webhook("hdg_test", "0dzpHnFyxhAziHqOzRCPQ", weather_data, "","");
        }
        webhook_count++;
      }
      //Serial.printf("webhook_count= %d\r\n",webhook_count);
      if(webhook_count == 100)  webhook_count = 0;
    }
    else  
    {
      Serial.printf("ERORR\r\n");
    }

    readTemp = 0;
    readDHT11(&readTemp, &readHumid);//센서로 온도를 측정
    char temp[80], humid[80];
    snprintf(temp,sizeof(temp),"%d", readTemp);
    snprintf(humid,sizeof(humid),"%d", readHumid);//온도, 습도 두 값을 문자열로 변환
    //send_webhook("hdg_test", "0dzpHnFyxhAziHqOzRCPQ", temp, humid,"");//웹훅 함수로 이벤트 이름, api키, 값1, 값2, 값3 을 전달
    Serial.printf("Temp: %d, Humid: %d\r\n", readTemp, readHumid);
    
    
    readTemp = 51;
    readHumid = 19;
    Serial.printf("readTemp, readHumid: %d  %d\r\n", readTemp, readHumid);
    if(fire_count == 30) 
    {
      fire_count = 0;
    } 
    
    if((readTemp > 50) && (readHumid <  20) && (fire_count == 0))
    {
        //축사의 온도나 습기가 비이상적으로 높거나, 낮다면 사용자에게 화제 경보를 보냄   
        send_webhook("fire_warning", "0dzpHnFyxhAziHqOzRCPQ", temp, humid,"");
        Serial.printf("화재경보 발생\r\n");
    }
    Serial.printf("fire_count: %d\r\n",fire_count);
    fire_count++;
       
    
    Serial.printf("\r\n");
 }
}
