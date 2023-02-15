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
//���̴������
unsigned char keycode;
unsigned char key_press_num=0;
uint GapValue,GapValue1;

//�����ʶ
volatile bit FlagTest = 0;		//��ʱ���Ա�־��ÿ0.5����λ��������0
volatile bit FlagKeyPress = 0;  //�м����±�־�����������0
//У׼����
//��Ϊ��ͬ�Ĵ������������߲��Ǻ�һ�£���ˣ�ÿһ����������Ҫ�������������������ʹ����ֵ��׼ȷ��
//�����ֲ��Գ���������ƫ��ʱ�����Ӹ���ֵ��
//������Գ���������ƫСʱ����С����ֵ��
//��ֵ����ΪС��
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

volatile bit ClearWeighFlag = 0; //�����������־λ�����0Ư

/******************�����ݱ��浽��Ƭ���ڲ�eeprom��******************/
void write_eeprom()
{
	SectorErase(0x2000);
	GapValue1=GapValue&0x00ff;
	byte_write(0x2000, GapValue1);
	GapValue1=(GapValue&0xff00)>>8;
	byte_write(0x2001, GapValue1);
	byte_write(0x2060, a_a);	
}

/******************�����ݴӵ�Ƭ���ڲ�eeprom�ж�����*****************/
void read_eeprom()
{
	GapValue   = byte_read(0x2001);
	GapValue   = (GapValue<<8)|byte_read(0x2000);
	a_a      = byte_read(0x2060);
}

/**************�����Լ�eeprom��ʼ��*****************/
void init_eeprom() 
{
	read_eeprom();		//�ȶ�
	if(a_a != 1)		//�µĵ�Ƭ����ʼ��Ƭ������eeprom
	{
		GapValue  = 3500;
		a_a = 1;
		write_eeprom();	   //��������
	}	
}

//��ʾ��������λkg����λ��������λС��
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

//��ʱ��0��ʼ��
void Timer0_Init()
{
	ET0 = 1;        //����ʱ��0�ж�
	TMOD = 1;       //��ʱ��������ʽѡ��
	TL0 = 0xb0;     
	TH0 = 0x3c;     //��ʱ�������ֵ
	TR0 = 1;        //������ʱ��
}

//��ʱ��0�ж�
void Timer0_ISR (void) interrupt 1 using 0
{
	uchar Counter;
	TL0 = 0xb0;
	TH0 = 0x3c;     //��ʱ�������ֵ

	//ÿ0.5����ˢ������
    Counter ++;
    if (Counter >= 10)
    {
       FlagTest = 1;
	   Counter = 0;
    }
}


//������Ӧ���򣬲����Ǽ�ֵ
//���ؼ�ֵ��
//         7          8    9      10(��0)
//         4          5    6      11(ɾ��)
//         1          2    3      12(δ����)
//         14(δ����) 0    15(.)  13(ȷ���۸�)

void KeyPress()
{
	if(ROW1==0)   //ȥƤ��
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
	
	if(ROW2==0)	   //��
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


	if(ROW3==0)  //��
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
			write_eeprom();			   //�������
		}
	}

}
//****************************************************
//������
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

	init_eeprom();  //��ʼ��ʼ�����������
	Init_LCD1602();									//��ʼ��LCD1602
   EA = 0;
   Timer0_Init();
   //��ʼ����ɣ����ж�
   EA = 1;
	
//	Get_Maopi();
	LCD1602_write_com(0x80);						//ָ������
   LCD1602_write_word(" Welcome To Use ");	//  
   LCD1602_write_com(0x80+0x40);						//ָ������
   LCD1602_write_word("Electronic Scale");
//   Delay_ms(2000);
   Get_Maopi();
   LCD1602_write_com(0x80);						//ָ������
   LCD1602_write_word("The Weight:     ");
   LCD1602_write_com(0x80+0x40);				//ָ������
   LCD1602_write_word("         0.000kg");
//	Get_Maopi();				//��ëƤ����

	while(1)
	{
//ÿ0.5�����һ��
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
//����
//****************************************************
void Get_Weight()
{
	Weight_Shiwu = HX711_Read();
	Weight_Shiwu = Weight_Shiwu - Weight_Maopi;		//��ȡ����
	
	Weight_Shiwu = (unsigned int)((float)(Weight_Shiwu*10)/GapValue)-qupi; 	//����ʵ���ʵ������																
	if(Weight_Shiwu >= 10000)		//���ر���
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
//��ȡëƤ����
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
//MS��ʱ����(12M�����²���)
//****************************************************
void Delay_ms(unsigned int n)
{
	unsigned int  i,j;
	for(i=0;i<n;i++)
		for(j=0;j<121;j++);
}