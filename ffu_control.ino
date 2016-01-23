//Arduino+PMS5003雾霾传感器+四路继电器

//空气质量传感器返回的串口数据大小端是反的
//需要转一下大小端
#ifndef ntohs
#define ntohs(x) ( ((x)<<8) | (((x)>>8)&0xFF) )
#endif

//GPIO针脚定义
//defines the gpio pins
#define FFU_POWER_PIN 11
#define FFU_HIGH_PIN  10
#define FFU_MID_PIN   9
#define FFU_LOW_PIN   8

//FFU状态位定义，对应着四路继电器开关
//define the ffu status bits
//电源
#define FFU_POWER 8
//高速
#define FFU_HIGH  4
//中速
#define FFU_MID   2
//低速
#define FFU_LOW   1

//ffu状态包含上面的4位：高，中，低，停
//the ffu status includes 4bits: power, high, mid, low
//1 1 0 0 FFU_POWER + FFU_HIGH
#define FFU_MODE_HIGH 12
//1 0 1 0 FFU_POWER + FFU_MID
#define FFU_MODE_MID  10
//1 0 0 1 FFU_POWER + FFU_LOW
#define FFU_MODE_LOW  9
//0 0 0 1 FFU_LOW
#define FFU_MODE_OFF  1

//定义开关状态，淘宝上买的继电器有高低电平触发的区别。
#define ON LOW
#define OFF HIGH

//参考PMS5003文档，定义空气质量结构
struct AQ {
  char head[2];
  unsigned short len;
  unsigned short cfpm1;
  unsigned short cfpm2_5;
  unsigned short cfpm10;
  unsigned short pm1;
  unsigned short pm2_5;
  unsigned short pm10;
  unsigned short dst0_3;
  unsigned short dst0_5;
  unsigned short dst1;
  unsigned short dst2_5;
  unsigned short dst5;
  unsigned short dst10;
  unsigned short unknown;
  unsigned short checksum;
};

void setup()
{
  pinMode(FFU_POWER_PIN, OUTPUT);
  pinMode(FFU_HIGH_PIN,  OUTPUT);
  pinMode(FFU_MID_PIN,   OUTPUT);
  pinMode(FFU_LOW_PIN,   OUTPUT);
  digitalWrite(FFU_POWER_PIN, OFF);  
  digitalWrite(FFU_HIGH_PIN,  OFF);  
  digitalWrite(FFU_MID_PIN,   OFF);  
  digitalWrite(FFU_LOW_PIN,   OFF);
  Serial.begin(9600); // PMS5003使用9600n8串口连接
}

//读一个字节
char readc()
{
  while(!Serial.available())
  {
    delay(10);
  }
  return Serial.read();
}

//读空气质量数据
AQ * getAQ()
{
  unsigned short i, checksum;
  byte buf[32];
  AQ *aq;
  
  while (true) {
    buf[0] = readc();
    if(buf[0]=='B')
    {
      buf[1] = readc();
      if(buf[1]=='M'){
      
        for(i=2;i<32;i++){
          buf[i] = readc();
        }
        aq = (AQ *)buf;
        //必须做校验，数据出错的概率还是挺大的。
        checksum=0;
        for(i=0;i<30;i++){
          checksum += buf[i];
        }
        if(ntohs(aq->checksum) == checksum)
        {
          return aq;
        }
      }
    }
  }
}

//根据传入的状态值设定FFU的工作状态
void set_ffu(int ffu_status)
{
  digitalWrite(FFU_POWER_PIN, ffu_status & FFU_POWER ? ON: OFF);  
  digitalWrite(FFU_HIGH_PIN,  ffu_status & FFU_HIGH  ? ON: OFF);  
  digitalWrite(FFU_MID_PIN,   ffu_status & FFU_MID   ? ON: OFF);  
  digitalWrite(FFU_LOW_PIN,   ffu_status & FFU_LOW   ? ON: OFF);
}
//记录上一个状态值，10~15, 50~60, 100~120临界区间使用上一个状态值，避免到50,100等临界点转速频繁抖动。
short last_ffu_status = FFU_MODE_OFF;
void loop()
{
  bool b;
  short pm1, pm2_5, pm10, ffu_status;
  AQ *aq;
  aq = getAQ();
  pm1 = ntohs(aq->pm1);
  pm2_5 = ntohs(aq->pm2_5);
  pm10 = ntohs(aq->pm10);
  if(pm2_5<10){
      ffu_status = FFU_MODE_OFF;
  } else if (pm2_5<15){
      ffu_status = last_ffu_status;
  } else if (pm2_5<50){
      ffu_status = FFU_MODE_LOW;
  } else if (pm2_5<60){
      ffu_status = last_ffu_status;
  } else if (pm2_5<100){
      ffu_status = FFU_MODE_MID;
  } else if (pm2_5<120){
      ffu_status = last_ffu_status;
  } else {
      ffu_status = FFU_MODE_HIGH;
  }
  Serial.print("pm1:");
  Serial.println(pm1);
  Serial.print("pm2.5:");
  Serial.println(pm2_5);
  Serial.print("pm10:");
  Serial.println(pm10);
  Serial.println(ffu_status);
  Serial.println(last_ffu_status);
  set_ffu(ffu_status);
  last_ffu_status = ffu_status;
  delay(1000);
}
