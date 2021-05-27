# IOT_Project_HDG
**IOT 수업 프로젝트**

주변 지역의 온도와 축사의 온도를 비교하여 화재를 감지, 가축의 체온, 걸음 수, 앞 다리의 각도를 측정하여 가축의 발정을 감지하고 사용자에게 알려주는 시스템


**사용하는 기술**
2.1 Inertial Measurement Unit
소의 활동량을 식별하기 위해 관성측정 장치로써 MPU 6050 모듈을 사용한다. 가속도 센서의 X, Y, Z축값을 합쳐 절댓값으로 환산하고 특정 값보다 크면 걸음 수를 1회 카운트하도록 설정한다. 특정 값은 여러 번의 테스트를 거쳐 산출한다. 가축의 승가 행위는 가속도와 자이로센서값을 활용하여 판별한다.

  2.2 Adafruit IO
활동량과 가축 체온변동정보를 Adafruit IO Dashboard를 이용하여 시각화해서 표시한다. 
개체번호는 입력하거나 controller bar를 이용하여 조절할 수 있도록 설계한다.

  2.3 IFTTT
활동량이 평소보다 급감하거나 가축 체온에 이상이 생겼을 때 휴대전화로 알림을 보낸다.
온도가 높으면 선풍기 세기를 제어하여 온도를 조절하고 온도가 급격하게 상승할 경우 화재로 감지하여 사용자와 119에 알림을 보낸다. 

  2.4 OpenAPI(openweathermap)
해당 지역의 날씨 데이터를 가져오기 위해서 openweathermap.api를 사용한다.


**진행할 작업**
1. mpu6050으로 만보기 만들어서 소 걸음 수 확인하기, 걸음 수에 따라 행동 식별하기
2. DNT11로 주변의 온도, 습도 데이터를 thingspeak로 전달
3. API를 이용하여 축사가 있는 지역의 날씨 데이터(온도, 습도) 가져오기

----------------
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


이후에는 뒷 걸음에 대해 구현할 예정입니다.
