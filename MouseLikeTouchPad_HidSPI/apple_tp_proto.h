/// �����ʺ� macbook pro 2017 13��� multi-touch bar.  Apple SPI Device ������������ 2016/5/26�� �汾 6.1.6500.0
/// Apple SPI Device ���������������������ݽṹ������ʵ�ʲ��Ի�ȡ����Ϊ�������ݽṹ�����������Ժ�� Apple SPI Device�汾���ܱ�Apple��˾�޸ġ� 

#pragma once

typedef unsigned short __le16;


#define TP_HEADER_SIZE    46
#define TP_FINGER_SIZE    30

// 46 length 
struct SPI_TRACKPAD_PACKET
{
	UINT8 PacketType;  // unknown type  =2
	UINT8 ClickOccurred;  // ��ס�˴����壬 ���ܼ�����ס������ 1
	UINT8 Reserved0[5];
	UINT8 IsFinger;  // ����������ָ 1�����뿪˲�䣬���� 0
	UINT8 Reserved1[16];
	UINT8 FingerDataLength;  // ��ָ�����ܳ��ȣ� ��ָ����*30
	UINT8 Reserved2[5];
	UINT8 NumOfFingers;  //��ָ����
	UINT8 ClickOccurred2;  // ͬ�ϱߵ�ClickOccurred
	UINT8 State1;  // ��ָ���ϱߺ����� 0x10�� ��ָ�뿪˲��������� 1����� 0x80��0x90��������뿪�󣬻������ 0x00
	UINT8 State2;  // ��ָ���ϱ� 0x20���뿪˲�� �� 0
	UINT8 State3;  // ƽʱ0�� ClickedΪ 0x10
	UINT8 Padding;  // ʼ�� 0
	UINT8 Reserved3[10];
};
//// 46 length 
//struct tp_protocol
//{
//	u8                  type;      // unknown type  =2
//	u8                  clicked;   // ��ס�˴����壬 ���ܼ�����ס������ 1
//	u8                  unknown1[5]; //
//	u8                  is_finger;   // ����������ָ 1�����뿪˲�䣬���� 0
//	u8                  unknown2[8]; // 
//	u8                  unknown3[8]; // δ֪���̶� 00-01-07-97-02-00-06-00
//	u8                  finger_data_length; // ��ָ�����ܳ��ȣ� ��ָ����*30
//	u8                  unknown4[5]; //
//	u8                  finger_number; //��ָ����
//	u8                  Clicked; // ͬ�ϱߵ�clicked
//	u8                  state;   // ��ָ���ϱߺ����� 0x10�� ��ָ�뿪˲��������� 1����� 0x80��0x90��������뿪�󣬻������ 0x00
//	u8                  state2;  // ��ָ���ϱ� 0x20���뿪˲�� �� 0
//	u8                  state3;  // ƽʱ0�� ClickedΪ 0x10
//	u8                  zero;    // ʼ�� 0
//	u8                  unknown5[10]; /////
//};


///// 30 length
struct SPI_TRACKPAD_FINGER
{
	SHORT OriginalX;  //����ʱ�ĳ�ʼ���꣬���º�������ֲ��䣬���Ǿ�������֤ʵ�������ʼ����OriginalX��OriginalY��ʱ׼ȷ��ʱ��׼���������������ᷢ���仯û�вο��������Բ����øò�����׷�ٴ�����
	SHORT OriginalY;
	SHORT X;          //��ǰ����ָ����
	SHORT Y;
	SHORT HorizontalAccel;
	SHORT VerticalAccel;
	SHORT ToolMajor;  //��ָ�Ӵ���Բ�泤��
	SHORT ToolMinor;  //��ָ�Ӵ���Բ����
	SHORT Orientation;  //��ָ�Ӵ���Բ��Ƕȷ���
	SHORT TouchMajor;  //
	SHORT TouchMinor;  //
	SHORT Rsvd1;
	SHORT Rsvd2;
	SHORT Pressure;  //����ѹ��
	SHORT Rsvd3;
};
/////// 30 length
//struct tp_finger
//{
//	short             org_x; //���º�������ֲ��䣬
//	short             org_y; //
//	short             x;     //������ָ�ƶ��ı䣬
//	short             y;     //
//	__le16            unknown[11];
//};


typedef struct _SPI_TRACKPAD_INFO {
	USHORT VendorId;
	USHORT ProductId;
	SHORT XMin;
	SHORT XMax;
	SHORT YMin;
	SHORT YMax;
} SPI_TRACKPAD_INFO, * PSPI_TRACKPAD_INFO;


static const SPI_TRACKPAD_INFO SpiTrackpadConfigTable[] =
{
	/* MacBookPro11,1 / MacBookPro12,1 */
	{ 0x05ac, 0x0272, -4750, 5280, -150, 6730 },
	{ 0x05ac, 0x0273, -4750, 5280, -150, 6730 },
	/* MacBook9 */
	{ 0x05ac, 0x0275, -5087, 5579, -128, 6089 },
	/* MacBookPro14,1 / MacBookPro14,2 */
	{ 0x05ac, 0x0276, -6243, 6749, -170, 7685 },
	{ 0x05ac, 0x0277, -6243, 6749, -170, 7685 },
	/* MacBookPro14,3 */
	{ 0x05ac, 0x0278, -7456, 7976, -163, 9283 },
	/* MacBook10 */
	{ 0x05ac, 0x0279, -5087, 5579, -128, 6089 },
	/* MacBookAir7,2 fallback */
	{ 0x05ac, 0x0290, -5087, 5579, -128, 6089 },
	{ 0x05ac, 0x0291, -5087, 5579, -128, 6089 },
	/* Terminator */
	{ 0 }
};


