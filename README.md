# IOT_Project_HDG
**IOT 수업 프로젝트**

축사의 온도를 확인하여 화재를 감지 / 가축의 체온, 걸음 수, 앞 다리의 각도를 측정하여 가축의 발정을 감지 / 축사 주변의 날씨 data를 실시간으로 전송받아 비와 눈이 내리는 것을 사용자에게 알려주는 시스템 


**진행할 작업**

1. mpu6050으로 만보기 만들어서 소 걸음 수 확인하기, mpu6050 센서값에 따라 행동 식별하기
2. DNT11로 주변의 온도, 습도가 비정상적으로 높거나, 낮은경우 사용자에게 화재 경보를 알림
3. API를 이용하여 축사가 있는 지역의 날씨 데이터를 가져와서 눈,비가 예상될때 사용자에게 알려주기


----------------------
**mpu6050을 이용한 만보기 제작**

mpu6050의 위치는 소의 앞다리, 사람이 소를 정면에서 바라볼 때 mpu6050의 led가 보이도록 부착 할 예정 

mpu6050에 사용되는 가속도 센서는 매 순간의 data는 확실하지 않으나 평균적인 값이 정확하고
자이로 센서는 매 순간의 값이 정확하나 평균적인 값은 부정확한 특징을 갖고 있기에
두 data를 각자 사용해서는 정확한 각도를 얻기 어렵습니다. 

때문에 두 data를 합쳐서 이상적인 각도를 구하는 상보필터를 이용하여 각도를 구하였습니다.

출처: https://m.blog.naver.com/PostView.naver?blogId=roboholic84&logNo=220401407348&proxyReferer=https:%2F%2Fwww.google.com%2F

상보필터 식) ![image](https://user-images.githubusercontent.com/59642490/119835059-744b4e00-bf3b-11eb-97a2-0b5048709244.png)

구현)
![image](https://user-images.githubusercontent.com/59642490/119836006-54685a00-bf3c-11eb-85a5-72697db9edd8.png)

atan2는 값을 라디안값으로 출력하기에 이를 deg값으로 변환해주어야 합니다. 그렇기에 * 180 / PI를 이용하여 변환해 주었습니다.

자이로센서 값의 출력 범위는 ±250°/sec 인데 출력 값은 ±16383입니다. 만약 mpu-6050이 측정한 각속도가 250°/sec라면 16383이라는 값을 출력합니다.
125°/sec일 경우는 약 8191을 출력할 것이구요. 이 출력된 값을 각속도로 변환해주는 과정이 /131.입니다.


결과)
-180 ~ 0까지, 0 ~ 180까지 측정이 가능했습니다.

![image](https://user-images.githubusercontent.com/59642490/119837534-a52c8280-bf3d-11eb-8774-7d5134a78b0e.png)![image](https://user-images.githubusercontent.com/59642490/119837629-bd9c9d00-bf3d-11eb-85c4-aa83caee95f6.png)

걸음을 인식)
시작은 가축이 가만히 서있는 상태에서 앞다리를 앞으로 움직이는 순간의 각도, x, z축 가속도 값의 변화량을 이용하여 걸음을 인식하였습니다.

400ms 마다 전에 저장한 각도, x, z 가속도 값과 비교하여 일정 값만큼 차이가 나면서, 새롭게 들어온 data가 걸음을 걷기위해 앞발을 들어올린 경우의 각도, x, z 가속도 값인 경우일때 걸음을 측정!

위의 조건을 만족하는 경우 걸음 수를 추가!

![image](https://user-images.githubusercontent.com/59642490/119852916-b039df80-bf4a-11eb-9548-f4f7b3aff258.png)



결과)
![image](https://user-images.githubusercontent.com/59642490/119852652-72d55200-bf4a-11eb-9383-b2eff2a0af1e.png)![image](https://user-images.githubusercontent.com/59642490/119852791-91d3e400-bf4a-11eb-95c4-e35e0c14caa6.png)

기승행위 구별하기)
수컷이 기승행위를 시도할 때 앞 다리의 각도, x, z축 가속도 값을 확인하여 어느정도 높이에 다리가 올라간 경우를 기승 자세라고 정하였습니다.


![image](https://user-images.githubusercontent.com/59642490/120113554-f646ab80-c1b5-11eb-9784-cb8a50bfe676.png)



기승 위라고 판단되는 자세가 3번 count되는 경우. 즉, 1.2초간 기승 자세가 인식된다면, 사용자에게 발정기가 의심된다는 메시지를 보냅니다.

--------------------------------------------------------------
**축사의 온도, 습도를 측정하여 이상시 알려주기**

DHT11을 사용하기위해 기존에 작성한 함수를 이용함 (페이즈1 ~ 페이즈4)


![image](https://user-images.githubusercontent.com/59642490/120113629-46be0900-c1b6-11eb-98f0-97049c24340a.png)


이때 축사 내부의 온도와 습도가 너무 높거나 낮은 경우에 IFTTT로 사용자의 핸드폰으로 알림이 가게끔 설계함.
12초를 주기로 하여 알림이 가게끔 count조건을 아용하여 작성함


![image](https://user-images.githubusercontent.com/59642490/120113680-838a0000-c1b6-11eb-8cdf-152ac61fb0a9.png)


결과)


![image](https://user-images.githubusercontent.com/59642490/120113898-9b15b880-c1b7-11eb-9f7b-5d518c4a34fb.png)




--------------------------------------------------------------
**날씨 data 가져와서 비교하기**

openweathermap.api를 사용하여 원하는 지역의 날씨를 0.4초마다 불러와서 실시간으로 비, 눈이 내리는 사실을 전달!


![image](https://user-images.githubusercontent.com/59642490/120076944-ed38da00-c0e2-11eb-977b-fd70e3f38c85.png)


이때, 눈 혹은 비가 내리는 경우 40초에 한 번씩 사용자의 핸드폰으로 알람이 가도록 IFTTT를 사용!
millis함수를 이용하여 0.4초의 딜레이를 만들었고, 100번 반복된 경우에만 알람이 간다!


![image](https://user-images.githubusercontent.com/59642490/120076973-12c5e380-c0e3-11eb-9e53-cb86e190a341.png)
![image](https://user-images.githubusercontent.com/59642490/120076997-283b0d80-c0e3-11eb-8fa2-6cc41f80f65e.png)
![image](https://user-images.githubusercontent.com/59642490/120077069-87008700-c0e3-11eb-8765-fad4a3c6118d.png)




