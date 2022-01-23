#include <reg52.h>
#include "intrins.h"
///////////////////////////////////////////////////
//                 程序功能介绍
//
//	1.开机输入密码
//	2.进入选择页面
//	3.k00音乐song k01温度hot k02时钟date
//	4.k6 退出当前程序进入选择页面
//
///////////////////////////////////////////////////

///////////////////////////////////////////////////
//配置部分开始
//////////////////////////////////////////////////
///////////////////////////////////////////////////
//配置部分开始
//////////////////////////////////////////////////
#define DIG P0              //数码管管脚定义
#define LSE P2
sbit SPEAKER = P1 ^ 4;      //蜂鸣器管脚定义
sbit CE = P1 ^ 2;	        //RET  ds1302时钟管脚定义
sbit SCIO = P1 ^ 1;        //IO
sbit SCLK = P1 ^ 0;        //SCLK
sbit DSPORT = P1 ^ 3;      //ds18b02温度传感器管脚定义
sbit k6 = P1 ^ 7;         //按键管脚定义
unsigned char code passwordDec[] = "20010929";   //8位字符密码，支持字符 0-f
//设置ds1302的时间 格式s/m/h/d/m/y十进制。比如现在设置的是
unsigned char init_time[] = {0x00, 0x34, 0x01, 0x30, 0x11, 0x21, 0x20};
//是否初始化ds1302的时间, 1 初始化，否则不初始化。所以第一次设置为1烧入并进入时间模块后，改为0然后再次烧入
unsigned char isNeedInitTime = 0;
///////////////////////////////////////////////////
//配置部分结束
///////////////////////////////////////////////////
///////////////////////////////////////////////////
//配置部分结束
///////////////////////////////////////////////////

unsigned char chooseApp = -1;	  //当前选择的应用
unsigned char isDs1302AlreadyInit = 0;
unsigned char isSongAlreadyInit = 0;
unsigned char code smgCode[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71};  //0-f共阴极数码管七段码
unsigned char DisplayData[8];  //要显示的数码管数据
unsigned char m,n; //定义4个八度 每八度12分音律 共48音律
unsigned char code T[49][2]= {{0,0}, //定义音律49个二维数组
    {0xF9,0x1F},{0xF9,0x82},{0xF9,0xDF},{0xFA,0x37},{0xFA,0x8A},{0xFA,0xD8},{0xFB,0x23},{0xFB,0x68},{0xFB,0xAA},{0xFB,0xE9},{0xFC,0x24},{0xFC,0x5B},
    {0xFC,0x8F},{0xFC,0xC1},{0xFC,0xEF},{0xFD,0x1B},{0xFD,0x45},{0xFD,0x6C},{0xFD,0x91},{0xFD,0xB4},{0xFD,0xD5},{0xFD,0xF4},{0xFE,0x12},{0xFE,0x2D},
    {0xFE,0x48},{0xFE,0x60},{0xFE,0x78},{0xFE,0x86},{0xFE,0xA3},{0xFE,0xB6},{0xFE,0xC9},{0xFE,0xDA},{0xFF,0xEB},{0xFE,0xFA},{0xFF,0x09},{0xFF,0x17},
    {0xFF,0x24},{0xFF,0x30},{0xFF,0x3C},{0xFF,0x47},{0xFF,0x51},{0xFF,0x5B},{0xFF,0x64},{0xFF,0x6D},{0xFF,0x75},{0xFF,0x7D},{0xFF,0x84},{0xFF,0x8B}
};
unsigned char code music[][2]= {{0,4}, //定义曲谱数组，前数为音律，后数为音符节拍 ，要换歌改变简谱对应的音律号即可
    // {0,1},{1,1},{3,1},{5,1},{6,1},{8,1},{10,1},{12,1},{13,1},//演示超低音八度 123457671
    // {0,1},{13,1},{15,1},{17,1},{18,1},{20,1},{22,1},{24,1},{25,1},//演示低音八度12345671
    // {0,1},{25,1},{27,1},{29,1},{30,1},{32,1},{34,1},{36,1},{37,1},//演示中音八度12345671
    // {0,1},{37,1},{39,1},{41,1},{42,1},{44,1},{46,1},{48,2},//演示高音八度 1234567
    {0,4},{24,4},{24,4},{21,4},{19,4},{21,4},{14,8},{19,4},{21,4},{24,4},{21,4},{19,16},//记录菊花台简谱歌词：0553236 23532 天青色等烟雨 而我在等你
    {0,4},{24,4},{24,4},{21,4},{19,4},{21,4},{12,8},{19,4},{21,4},{24,4},{19,4},{17,16},//简谱歌词：0553235 23521 炊烟袅袅升起 隔江千万里
    {0,4},{17,4},{19,4},{21,4},{24,4},{26,4},{24,4},{22,4},{24,4},{21,4},{21,4},{19,4},{19,16},//简谱歌词：01235654 53322 在平地书刻你房间上的飘影
    {0,4},{17,4},{19,4},{17,4},{17,4},{19,4},{17,4},{19,4},{19,4},{21,8},{24,4},{21,4},{21,12},//简谱歌词：就当我为遇见你伏笔
    {0,4},{24,4},{24,4},{21,4},{19,4},{21,4},{14,8},{19,4},{21,4},{24,4},{21,4},{19,16}, //简谱歌词：0553236 23532 天青色等烟雨 而我在等你
    {0,4},{24,4},{24,4},{21,4},{19,4},{21,4},{12,8},{19,4},{21,4},{24,4},{19,4},{17,16}, //简谱歌词：0553235 23521 月色被打捞起 掩盖了结局
    {0,4},{17,4},{19,4},{21,4},{24,4},{26,4},{24,4},{22,4},{24,4},{21,4},{21,4},{19,4},{19,12},//简谱歌词：0123 5654 5332 25 322 11 如传世的青花瓷在独自美丽
    {12,4},{21,8},{19,8},{19,4},{17,20}, //简谱歌词：你眼带笑意
    {0xFF,0xFF}
}; //歌曲结尾标识

void delay(unsigned int i)
{
	while (i--)
		;
}
//////////////////////////
//
//
// ds1302部分开始
//
//
/////////////////////////
#define nop() _nop_()

#define UP() {SCLK = 0;nop();SCLK = 1;nop();} //上升沿  ,使用宏定义函数时最后一定家分号
#define DOWN() {SCLK = 1;nop();SCLK = 0;nop();} //下降沿

void ds1302_writebyte(unsigned char byte);//写一个字节; 
void ds1302_writedata(unsigned char addr,unsigned char data_);//给某地址写数据,data是c51内部的关键字，表示将变量定义在数据存储区，故此处用data_;
unsigned char ds1302_readbyte();//读一个字节
unsigned char ds1302_readdata(unsigned char addr);//读取某寄存器数据     ;
void init_ds1302();
void ds1302_readtime();

void ds1302_writebyte(unsigned char byte){
    unsigned char i;
    unsigned int t = 0x01;
    for(i=0;i<8;i++){
        SCIO = byte & t;        
        t<<=1;
        DOWN();       //下降沿完成一个位的操作
    }
    SCIO = 1;//确保释放io引脚
}
void ds1302_writedata(unsigned char addr,unsigned char data_){
    CE = 0;        nop();    
    SCLK = 0;    nop();    
    CE = 1;        nop();    //使能片选信号
    ds1302_writebyte((addr<<1)|0x80);    //方便后面写入
    ds1302_writebyte(data_);
    CE = 0;        nop();//传送数据结束

}
unsigned char ds1302_readbyte(){
    unsigned char i;
    unsigned char data_ = 0;
    unsigned int t = 0x01;     
    for(i=0;i<7;i++){    
        if(SCIO){

            data_ = data_ | t;    //低位在前，逐位读取
        }                
        t<<=1;
        DOWN();
    }
     return data_;
}
unsigned char ds1302_readdata(unsigned char addr){
    unsigned char data_ = 0;
    CE = 0;     nop();
    SCLK = 0;  nop();
    CE = 1;      nop();
    ds1302_writebyte((addr<<1)|0x81);
    data_ = ds1302_readbyte();
    CE = 0;       nop();
    SCLK = 1;  nop();
    SCIO = 0;  nop();
    SCIO = 1;  nop();

    return data_;
}
void init_ds1302(){

     unsigned char i;
     CE = 0;   //初始化引脚
     SCLK = 0; 
     i  = ds1302_readdata(0x00);  //读取秒寄存器,秒在最低位
     if((i & 0x80 != 0)){

         ds1302_writedata(7,0x00); //撤销写保护，允许写入数据
        for(i = 0;i<7;i++){

            ds1302_writedata(i,init_time[i]);
        }
     }    
}
void ds1302_readtime(){       //读取时间
      unsigned char i;
      for(i = 0;i<7;i++){

             init_time[i] = ds1302_readdata(i);
      }
}
void display()         //将时间数据转化为对应的七段码
{
	ds1302_readtime();
	DisplayData[7] = smgCode[init_time[0] & 0x0f];
	DisplayData[6] = smgCode[init_time[0] >> 4];
	DisplayData[5] = 0X40;                            // 字符 ‘-’ 的七段码
	DisplayData[4] = smgCode[init_time[1] & 0x0f];
	DisplayData[3] = smgCode[init_time[1] >> 4];
	DisplayData[2] = 0X40;
	DisplayData[1] = smgCode[init_time[2] & 0x0f];
	DisplayData[0] = smgCode[init_time[2] >> 4];
}
void time0_init()
{
	TMOD = 0X02;
	TH0 = 0X9C;
	TL0 = 0X9C;
	ET0 = 1;
	EA = 1;
	TR0 = 1;
}

unsigned char Num = 0;
void InterriptDigDisplayAndSong() interrupt 1
{
	if (isSongAlreadyInit)  //因为共用了计数器0，所以增加这个变量来判断当前是显示还是播放中断函数 interrupt 1;
	{
		SPEAKER = !SPEAKER; //蜂鸣器翻转发声
		TH0 = T[m][0];
		TL0 = T[m][1];
		//音律延时周期次数码表赋给定时寄存器作为计数初始值,每TH0TL0个机器周期触发蜂鸣器端口翻转，演奏出不同音符
	}
	else
	{
		DIG = 0;
		switch (Num)
		{
		case (7):
			LSE = 0xfe;
			break;
		case (6):
			LSE = 0xfd;
			break;
		case (5):
			LSE = 0xfb;
			break;
		case (4):
			LSE = 0xf7;
			break;
		case (3):
			LSE = 0xef;
			break;
		case (2):
			LSE = 0xdf;
			break;
		case (1):
			LSE = 0xbf;
			break;
		case (0):
			LSE = 0x7f;
			break;
		}
		DIG = DisplayData[Num];
		Num++;
		if (Num > 7)
			Num = 0;
	}
	if (k6 == 0)		//检测到 按键k6 被按下，停止计数器，返回菜单选择
	{
		TR0 = 0;
		isDs1302AlreadyInit = 0;
		isSongAlreadyInit = 0;
		chooseApp = -1;
		SPEAKER=0;
	}
}
void ds1302api(void)
{
	if (!isDs1302AlreadyInit)  
	{
		time0_init();  
		isDs1302AlreadyInit = 1;
	}
	TR0 = 1;       //启动计时器
	display();
}
//////////////////////////
//
//
// ds1302部分结束
//
//
/////////////////////////

//////////////////////////
//
//
// song部分开始
//
//
/////////////////////////
void songdelay(unsigned char p) //延时函数 无符号字符型变量
{
	unsigned char i, j;					  //定义无符号字符型变量J和I
	for (; p > 0; p--)			  // 此处P值即主函数的n值节拍个数
		for (i = 181; i > 0; i--) //延时181*181个机器周期约35毫秒，即一个1/16节拍
			for (j = 181; j > 0; j--)
				;
}

unsigned char i = 0;	 //定义无符号字符型变量i,初始值为0
void songapi()
{
	if (!isSongAlreadyInit)      //初始化
	{
		TMOD = 0x01;
		EA = 1;
		ET0 = 1; //开启T0定时16位方式，总中断开启，开启T0外部中断请求
		isSongAlreadyInit=1;
	}
	
    while(1) //开始曲谱演奏，循环无限重复
    {
		m = music[i][0]; //将音律号赋值给m
		n = music[i][1]; //将节拍号赋值给n
		if (m == 0x00)	 //如果音律号为0，
		{
			TR0 = 0;
			songdelay(n);
			i++;
		}					//关闭计时器，延迟n拍，将循环数I加1 ，准备读下一个音符
		else if (m == 0xFF) //否则如果音律数为FF
		{
			TR0 = 0;
			songdelay(30);
			i = 0;
		}							   //开启节拍延时30个1/16节拍,歌曲停顿2秒,将循环数I置0
		else if (m == music[i + 1][0]) // 否则如果把下一个音律号数给变量m
		{
			TR0 = 1;
			songdelay(n);
			TR0 = 0;
			i++;
		}	 //定时器0打开延迟n拍，关闭定时器T0，读下一个音符，循环数加1读下一个音律
		else //音符若不为零
		{
			TR0 = 1;
			songdelay(n);
			i++;
		} //打开定时器，延时n个1/16拍，循环数I加1 ,准备演奏下一个音符

		if (k6 == 0 || isSongAlreadyInit==0)   //检测到 按键k6 被按下，停止计数器，返回菜单选择 
		{
			TR0 = 0;
			isSongAlreadyInit = 0;
			chooseApp = -1;
			SPEAKER=0;
			break;
		}
	}
}
//////////////////////////
//
//
// song部分结束
//
//
/////////////////////////

//////////////////////////
//
//
// ds18b02部分开始
//
//
/////////////////////////
unsigned char Ds18b20Init();
void Ds18b20WriteByte(unsigned char com);
unsigned char Ds18b20ReadByte();
void Ds18b20ChangTemp();
void Ds18b20ReadTempCom();
int Ds18b20ReadTemp();

unsigned char Ds18b20Init()
{
	unsigned char i;
	DSPORT = 0;
	i = 70;
	while (i--)
		;
	DSPORT = 1;
	i = 0;
	while (DSPORT)
	{
		delay(50);
		i++;
		if (i > 5)
		{
			return 0;
		}
	}
	return 1;
}
void Ds18b20WriteByte(unsigned char dat)
{
	unsigned char i, j;
	for (j = 0; j < 8; j++)
	{
		DSPORT = 0;
		i++;
		DSPORT = dat & 0x01;
		i = 6;
		while (i--)
			;
		DSPORT = 1;
		dat >>= 1;
	}
}
unsigned char Ds18b20ReadByte()
{
	unsigned char byte, bi;
	unsigned char i, j;
	for (j = 8; j > 0; j--)
	{
		DSPORT = 0;
		i++;
		DSPORT = 1;
		i++;
		i++;
		bi = DSPORT;
		byte = (byte >> 1) | (bi << 7);
		i = 4;
		while (i--)
			;
	}
	return byte;
}
void Ds18b20ChangTemp()
{
	Ds18b20Init();
	delay(50);
	Ds18b20WriteByte(0xcc);
	Ds18b20WriteByte(0x44);
}
void Ds18b20ReadTempCom()
{
	Ds18b20Init();
	delay(50);
	Ds18b20WriteByte(0xcc);
	Ds18b20WriteByte(0xbe);
}
int Ds18b20ReadTemp()
{
	int temp = 0;
	unsigned char tmh, tml;
	Ds18b20ChangTemp();
	Ds18b20ReadTempCom();
	tml = Ds18b20ReadByte();
	tmh = Ds18b20ReadByte();
	temp = tmh;
	temp <<= 8;
	temp |= tml;
	return temp;
}
void datapros(int temp)         //转换获取到的温度数据
{
	float tp;
	if (temp < 0)
	{
		DisplayData[0] = 0x40;
		temp = temp - 1;
		temp = ~temp;
		tp = temp;
		temp = tp * 0.0625 * 100 + 0.5;
	}
	else
	{
		DisplayData[0] = 0x00;
		tp = temp;
		temp = tp * 0.0625 * 100 + 0.5;
	}
	DisplayData[0] = smgCode[temp % 10000 / 1000];
	DisplayData[1] = smgCode[temp % 1000 / 100] | 0x80;
	DisplayData[2] = smgCode[temp % 100 / 10];
	DisplayData[3] = smgCode[temp % 10 / 1];
	DisplayData[4] = 0x58;
}

void DigDisplay()
{
	unsigned char i;
	for (i = 0; i < 5; i++)
	{
		switch (i)
		{
		case (4):
			P2 = 0xf7;
			break;
		case (3):
			P2 = 0xef;
			break;
		case (2):
			P2 = 0xdf;
			break;
		case (1):
			P2 = 0xbf;
			break;
		case (0):
			P2 = 0x7f;
			break;
		}
		P0 = DisplayData[i];
		delay(150);
		P0 = 0x00;

		if (k6 == 0)            //检测到 按键k6 被按下，停止计数器，返回菜单选择 
			chooseApp = -1;
	}
}
void ds18b02api()
{
	datapros(Ds18b20ReadTemp());
	DigDisplay();
}
//////////////////////////
//
//
// ds18b02部分结束
//
//
/////////////////////////

//////////////////////////
//
//
// 密码验证部分开始
//
//
/////////////////////////
unsigned char passwordbit = 0;	  //当前输入密码的位数
unsigned char waitnumber[8];     //当前已输入的密码
unsigned char passwordnumber[8];  //将密码转化为对应的七段码

//将十进制密码转换成 数码管段码
void getPasswordnumberFromPassworddec()
{
	unsigned char i = 0;
	for (; i < 8; i++)
	{
		passwordnumber[i] = smgCode[passwordDec[i] - 48];
	}
}
// 仍在输入密码时显示
void displaywait()
{
	unsigned char i, k;
	for (k = 0; k < 20; k++)
	{
		for (i = 0; i < 8; i++)
		{
			switch (i) //位选，选择点亮的数码管，
			{
			case (7):
				LSE = 0xfe;
				break;
			case (6):
				LSE = 0xfd;
				break;
			case (5):
				LSE = 0xfb;
				break;
			case (4):
				LSE = 0xf7;
				break;
			case (3):
				LSE = 0xef;
				break;
			case (2):
				LSE = 0xdf;
				break;
			case (1):
				LSE = 0xbf;
				break;
			case (0):
				LSE = 0x7f;
				break;
			}

			if (i == passwordbit)
				P0 = waitnumber[i] | 0x80; //增加一个小数点显示当前输入的位
			else
				P0 = waitnumber[i]; //发送数据

			delay(100); //间隔一段时间扫描
			P0 = 0x00;	//消隐
		}
	}
}
// 密码正确提示语
void displaypass()
{
	unsigned char code passcode[] = {0x73, 0x77, 0x6d, 0x6d}; // pass 的七段码
	unsigned char i,k;  //8 bit limit 256
	for (k = 0; k < 254; k++)
	{
		for (i = 2; i < 6; i++)
		{
			switch (i) //位选，选择点亮的数码管，
			{
			case (5):
				LSE = 0xfb;
				break;
			case (4):
				LSE = 0xf7;
				break;
			case (3):
				LSE = 0xef;
				break;
			case (2):
				LSE = 0xdf;
				break;
			}
			P0 = passcode[i - 2]; //发送数据
			delay(100);			  //间隔一段时间扫描
			P0 = 0x00;			  //消隐

			SPEAKER = ~SPEAKER;
		}
	}
	SPEAKER = 0;
}
// 获取键值
unsigned char keyscan()
{
	unsigned char num = -1;
	P2 = 0xf0;
	if (P2 != 0xf0) //检测是否有键按下
	{
		delay(50);		//延时消抖
		if (P2 != 0xf0) //再次检测是否有键按下
		{
			P2 = 0xf0;	//对应IO口拉高，以便检测
			switch (P2) //先确定行
			{
			case 0x70:
				num = 3;
				break; //第一行
			case 0xb0:
				num = 2;
				break; //第二行
			case 0xd0:
				num = 1;
				break; //第三行
			case 0xe0:
				num = 0;
				break; //第四行
			}
			P2 = 0x0f;	//对应IO口拉高，以便检测
			switch (P2) //再确定列
			{
			case 0x07:
				num = num + 12;
				break; //第一列
			case 0x0b:
				num = num + 8;
				break; //第二列
			case 0x0d:
				num = num + 4;
				break; //第三列
			case 0x0e:
				num = num + 0;
				break; //第四列
			}
		}
	}
	return num; //结合行列扫描的值得出按键对应的数值，并返回
}

//判断密码是否正确
char isRightPassword()
{
	unsigned char i = 0;
	for (; i < 8; i++)
	{
		if (waitnumber[i] != passwordnumber[i])
		{
			return 0;
		}
	}
	return 1;
}
//////////////////////////
//
//
// 密码验证部分结束
//
//
/////////////////////////

//////////////////////////
//
//
// main部分开始
//
//
/////////////////////////
unsigned char code songcode[] = {0x3f, 0x40, 0x6d, 0x3f, 0x37, 0x6f}; // 1-song  七段码
unsigned char code hotcode[] = {0x06, 0x40, 0x74, 0x5c, 0x78, 0x00};  // 2-hot  七段码
unsigned char code datecode[] = {0x5b, 0x40, 0x5e, 0x77, 0x78, 0x79}; // 3-date  七段码

void displayChoose()     //显示菜单
{
	unsigned char i,j;
	unsigned int k;
	unsigned char keyscanNum;  //扫描得到的键值
	for(j=0; j<3; j++)          //设置要显示的应用名
		for (k = 0; k < 500; k++)
		{
			for (i = 1; i < 7; i++)
			{
				switch (i) //位选，选择点亮的数码管，
				{
				case (6):
					LSE = 0xfd;break;
				case (5):
					LSE = 0xfb;break;
				case (4):
					LSE = 0xf7;break;
				case (3):
					LSE = 0xef;break;
				case (2):
					LSE = 0xdf;break;
				case (1):
					LSE = 0xbf;break;
				}
				switch (j)
				{
				case 0:
					P0 = songcode[i - 1];break;
				case 1:
					P0 = hotcode[i - 1];break;
				case 2:
					P0 = datecode[i - 1];break;
				}
				delay(100); //间隔一段时间扫描
				P0 = 0x00;	//消隐
			}
			keyscanNum = keyscan();
			if (keyscanNum == 0 || keyscanNum == 1 || keyscanNum == 2) //进入应用
			{
				chooseApp = keyscanNum;
				return;
			}
		}
}

void main()
{
	SPEAKER = 0;  //初始化
	k6 = 1;     
	getPasswordnumberFromPassworddec();  //转换密码
	if(isNeedInitTime)          //是否初始化时间
		init_ds1302(); 

	while (1)
	{
		if (isRightPassword())      //判断是否已经输入正确的密码
		{
			displaypass();
			while (1)
			{
				switch (chooseApp)      //展示菜单
				{
				case -1:
					displayChoose();
					break;
				case 0:
					songapi();
					break;
				case 1:
					ds18b02api();
					break;
				case 2:
					ds1302api();
					break;
				}
			}
		}
		else            //没有，等待输入正确的密码
		{
			unsigned char keynum = keyscan();
			if (keynum != -1)       //是否有按键按下 
			{
				waitnumber[passwordbit] = smgCode[keynum];   //有，检查是否已经输入正确的密码
				(passwordbit >= 7) ? (passwordbit = 0) : (passwordbit++);   //循环输入
			}
			displaywait();
		}
	}
}
//////////////////////////
//
//
// main部分结束
//
//
/////////////////////////