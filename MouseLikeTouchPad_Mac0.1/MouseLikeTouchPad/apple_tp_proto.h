/// �����ʺ� macbook pro 2017 13��� multi-touch bar.  Apple SPI Device ������������ 2016/5/26�� �汾 6.1.6500.0
/// Apple SPI Device ���������������������ݽṹ������ʵ�ʲ��Ի�ȡ����Ϊ�������ݽṹ�����������Ժ�� Apple SPI Device�汾���ܱ�Apple��˾�޸ġ� 

#pragma once

typedef unsigned short __le16;
typedef unsigned char  u8;

#define STABLE_INTERVAL_MSEC         100   // ��ָ������������ȶ�ʱ���� 

#define MouseReport_INTERVAL_MSEC     4   // ��걨����ʱ��ms����Ƶ��250hzΪ��׼
#define MButton_Interval_MSEC         100   // ��������Ҽ���ָ��������ʱ��ms��

#define Jitter_Time_MSEC         50   // ���������㶶����ʱ������ֵms��
#define Jitter_Offset         10    // ���������㶶����λ����ֵ



#define TP_HEADER_SIZE    46
#define TP_FINGER_SIZE    30

//// 46 length 
struct tp_protocol
{
	u8                  type;      // unknown type  =2
	u8                  clicked;   // ��ס�˴����壬 ���ܼ�����ס������ 1
	u8                  unknown1[5]; //
	u8                  is_finger;   // ����������ָ 1�����뿪˲�䣬���� 0
	u8                  unknown2[8]; // 
	u8                  unknown3[8]; // δ֪���̶� 00-01-07-97-02-00-06-00
	u8                  finger_data_length; // ��ָ�����ܳ��ȣ� ��ָ����*30
	u8                  unknown4[5]; //
	u8                  finger_number; //��ָ����
	u8                  Clicked; // ͬ�ϱߵ�clicked
	u8                  state;   // ��ָ���ϱߺ����� 0x10�� ��ָ�뿪˲��������� 1����� 0x80��0x90��������뿪�󣬻������ 0x00
	u8                  state2;  // ��ָ���ϱ� 0x20���뿪˲�� �� 0
	u8                  state3;  // ƽʱ0�� ClickedΪ 0x10
	u8                  zero;    // ʼ�� 0
	u8                  unknown5[10]; /////
};


///// 30 length
struct tp_finger
{
	short             org_x; //���º�������ֲ��䣬
	short             org_y; //
	short             x;     //������ָ�ƶ��ı䣬
	short             y;     //
	__le16            unknown[11];
};


/////////////���������ݷ�������ģ�����

struct mouse_event_t
{
	unsigned char     button; /// 0 û�� �� 1 �����ס�� 2 �Ҽ���ס��
    short             dx;
	short             dy;
	char              v_wheel; //��ֱ����
	char              h_wheel; //ˮƽ����
};

typedef void(*MOUSEEVENTCALLBACK)(mouse_event_t* evt, void* param);

void MouseLikeTouchPad_init(MOUSEEVENTCALLBACK cbk, void* param);

void MouseLikeTouchPad_parse(u8* data, LONG length);

void MouseLikeTouchPad_parse_init();


