#include <reg52.h>
#include <intrins.h>
#include <string.h>
#include "main.h"
#include "LCD1602.h"
#include "HX711.h"
#include "eeprom52.h"

#define uchar unsigned char
#define uint  unsigned int
	//=============================
	sbit F_close = P2^0;////Funnel switch
	sbit F_open = P2^2;////Funnel switch
  sbit Conveyor_belt = P2^1;//Conveyor_belt
	long Weight_touwei = 1000;
  short flag;
	//==============================


////=============================================
unsigned long HX711_Buffer = 0;
unsigned long Weight_Maopi = 0;
unsigned long Weight_Maopi_0 = 0;
long Weight_Shiwu = 0;
unsigned int qupi=0;
//键盘处理变量
unsigned char keycode;
unsigned char key_press_num=0;
uint GapValue,GapValue1;

//定义标识
volatile bit FlagTest = 0;		//定时测试标志，每0.5秒置位，测完清0
volatile bit FlagKeyPress = 0;  //有键按下标志，处理完毕清0
//校准参数
//因为不同的传感器特性曲线不是很一致，因此，每一个传感器需要矫正这里这个参数才能使测量值很准确。
//当发现测试出来的重量偏大时，增加该数值。
//如果测试出来的重量偏小时，减小改数值。
//该值可以为小数
//#define GapValue 349
////sbit LED=P1^1;
short LED;

////sbit ROW1=P1^0;
sbit ROW1=P3^1;
////sbit ROW2=P1^1;
sbit ROW2=P3^0;
////sbit ROW3=P1^2;
sbit ROW3=P3^2;
////sbit ROW4=P1^3;
sbit ROW4=P3^3;

volatile bit ClearWeighFlag = 0; //传感器调零标志位，清除0漂

/******************把数据保存到单片机内部eeprom中******************/
void write_eeprom()
{
	SectorErase(0x2000);
	GapValue1=GapValue&0x00ff;
	byte_write(0x2000, GapValue1);
	GapValue1=(GapValue&0xff00)>>8;
	byte_write(0x2001, GapValue1);
	byte_write(0x2060, a_a);	
}

/******************把数据从单片机内部eeprom中读出来*****************/
void read_eeprom()
{
	GapValue   = byte_read(0x2001);
	GapValue   = (GapValue<<8)|byte_read(0x2000);
	a_a      = byte_read(0x2060);
}

/**************开机自检eeprom初始化*****************/
void init_eeprom() 
{
	read_eeprom();		//先读
	if(a_a != 1)		//新的单片机初始单片机内问eeprom
	{
		GapValue  = 3500;
		a_a = 1;
		write_eeprom();	   //保存数据
	}	
}

//显示重量，单位kg，两位整数，三位小数
void Display_Weight()
{
	LCD1602_write_com(0x80+0x40+8);
	if(Weight_Shiwu/10000==0)
	LCD1602_write_data(' ');
	else
	LCD1602_write_data(Weight_Shiwu/10000 + 0x30);
	LCD1602_write_data(Weight_Shiwu%10000/1000 + 0x30);
	LCD1602_write_data('.');
	LCD1602_write_data(Weight_Shiwu%1000/100 + 0x30);
	LCD1602_write_data(Weight_Shiwu%100/10 + 0x30);
	LCD1602_write_data(Weight_Shiwu%10 + 0x30);
}

//定时器0初始化
void Timer0_Init()
{
	ET0 = 1;        //允许定时器0中断
	TMOD = 1;       //定时器工作方式选择
	TL0 = 0xb0;     
	TH0 = 0x3c;     //定时器赋予初值
	TR0 = 1;        //启动定时器
}

//定时器0中断
void Timer0_ISR (void) interrupt 1 using 0
{
	uchar Counter;
	TL0 = 0xb0;
	TH0 = 0x3c;     //定时器赋予初值

	//每0.5秒钟刷新重量
    Counter ++;
    if (Counter >= 10)
    {
       FlagTest = 1;
	   Counter = 0;
    }
}


//按键响应程序，参数是键值
//返回键值：
//         7          8    9      10(清0)
//         4          5    6      11(删除)
//         1          2    3      12(未定义)
//         14(未定义) 0    15(.)  13(确定价格)

void KeyPress()
{
	if(ROW1==0)   //去皮键
	{
		Delay_ms(5);
		if(ROW1==0)
		{
//			Get_Maopi();
			if(qupi==0)
			qupi=Weight_Shiwu;
			else
			qupi=0;
			Buzzer=0;
			Delay_ms(50);
			Buzzer=1;	
			while(ROW1==0);
		}
	}
	
	if(ROW2==0)	   //加
	{
		Delay_ms(5);
		if(ROW2==0)
		{
			while(!ROW2)
			{
				key_press_num++;
				if(key_press_num>=100)
				{
					key_press_num=0;
					while(!ROW2)
					{
						if(GapValue<10000)
						GapValue+=10;
						Buzzer=0;
						Delay_ms(10);
						Buzzer=1;
						Delay_ms(10);
						Get_Weight();
					}
				}
				Delay_ms(10);
			}
			if(key_press_num!=0)
			{
				key_press_num=0;
				if(GapValue<10000)
				GapValue++;
				Buzzer=0;
				Delay_ms(50);
				Buzzer=1;
			}
			write_eeprom();		
		}
	}


	if(ROW3==0)  //减
	{
		Delay_ms(5);
		if(ROW3==0)
		{
			while(!ROW3)
			{
				key_press_num++;
				if(key_press_num>=100)
				{
					key_press_num=0;
					while(!ROW3)
					{
						if(GapValue>1)
						GapValue-=10;
						Buzzer=0;
						Delay_ms(10);
						Buzzer=1;
						Delay_ms(10);
						Get_Weight();
					}
				}
				Delay_ms(10);
			}
			if(key_press_num!=0)
			{
				key_press_num=0;
				if(GapValue>1)
				GapValue--;
				Buzzer=0;
				Delay_ms(50);
				Buzzer=1;
			}
			write_eeprom();			   //保存数�
		}
	}

}
//****************************************************
//主函数
//****************************************************
void main()
{
////======================
  F_close=0;
	F_open = 0;
	Conveyor_belt = 0;
//	ROW4=1;
	flag=1;
////======================

	init_eeprom();  //开始初始化保存的数据
	Init_LCD1602();									//初始化LCD1602
   EA = 0;
   Timer0_Init();
   //初始化完成，开中断
   EA = 1;
	
//	Get_Maopi();
	LCD1602_write_com(0x80);						//指针设置
   LCD1602_write_word(" Welcome To Use ");	//  
   LCD1602_write_com(0x80+0x40);						//指针设置
   LCD1602_write_word("Electronic Scale");
//   Delay_ms(2000);
   Get_Maopi();
   LCD1602_write_com(0x80);						//指针设置
   LCD1602_write_word("The Weight:     ");
   LCD1602_write_com(0x80+0x40);				//指针设置
   LCD1602_write_word("         0.000kg");
//	Get_Maopi();				//称毛皮重量

	while(1)
	{
//每0.5秒称重一次
		if (FlagTest==1)
		{
			Get_Weight();
			FlagTest = 0;

		}	 
		KeyPress();

	//=============================
////	while()
////	  if(ROW2==0)
////		{
///   	   	for(j=1;j<=3;j++)
///				{
		   if(ROW4==0)
			  {
					flag=0;
          F_open=1;
			    Delay_ms(3000);
			    F_open=0;
				}
					if(Weight_Shiwu+0>=Weight_touwei)//
						{
						  if(flag==0) 
								{
							flag++;
							F_close=1;
            	Delay_ms(3000);
            	F_close=0;
							Conveyor_belt = 1;
							Delay_ms(5000);
							Conveyor_belt =0;
//                Delay_ms(3000);
//					      ROW4=0;
							}
							
						}

////=============================	
		
	}


	
	
}
//****************************************************
//称重
//****************************************************
void Get_Weight()
{
	Weight_Shiwu = HX711_Read();
	Weight_Shiwu = Weight_Shiwu - Weight_Maopi;		//获取净重
	
	Weight_Shiwu = (unsigned int)((float)(Weight_Shiwu*10)/GapValue)-qupi; 	//计算实物的实际重量																
	if(Weight_Shiwu >= 10000)		//超重报警
	{
		Buzzer = !Buzzer;	
		LED=!LED;
		LCD1602_write_com(0x80+0x40+8);
	   LCD1602_write_word("--.---");
	}
	else
	{
		if(Weight_Shiwu==0)
		LED=0;
		else if(Weight_Shiwu>0)
		LED=1; 
		Buzzer = 1;
		Display_Weight();
	}
}

//****************************************************
//获取毛皮重量
//****************************************************
void Get_Maopi()
{
	unsigned char clear;
mm:	Weight_Maopi_0 = HX711_Read();
	for(clear=0;clear<10;clear++)
	{
		Buzzer=1;
		LED=0;
		Delay_ms(100);
		LED=1;
		Delay_ms(100);	
	}
	Weight_Maopi = HX711_Read();
	if(Weight_Maopi/GapValue!=Weight_Maopi_0/GapValue)
	goto mm;
	Buzzer=0;
	Delay_ms(500);
	Buzzer=1;
} 

//****************************************************
//MS延时函数(12M晶振下测试)
//****************************************************
void Delay_ms(unsigned int n)
{
	unsigned int  i,j;
	for(i=0;i<n;i++)
		for(j=0;j<121;j++);
}
//上传到Github
