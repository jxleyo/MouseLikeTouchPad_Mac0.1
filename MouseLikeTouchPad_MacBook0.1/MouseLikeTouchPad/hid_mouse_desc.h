#pragma once

#define REPORTID_MOUSE  2

/////
const unsigned char MouseReportDescriptor[] = {
	///
	0x05, 0x01, // USAGE_PAGE(Generic Desktop)
	0x09, 0x02, //   USAGE(Mouse)
	0xA1, 0x01, //   COLLECTION(APPlication)
	0x09, 0x01, //   USAGE(Pointer)
	0xA1, 0x00, //     COLLECTION(Physical)
	0x85, REPORTID_MOUSE, //     ReportID(Mouse ReportID)
	0x05, 0x09, //     USAGE_PAGE(Button)
	0x19, 0x01, //     USAGE_MINIMUM(button 1)   Button ������ λ 0 ����� λ1 �Ҽ��� λ2 �м�
	0x29, 0x03, //     USAGE_MAXMUM(button 3)  //0x03����������갴������
	0x15, 0x00, //     LOGICAL_MINIMUM(0)
	0x25, 0x01, //     LOGICAL_MAXIMUM(1)
	0x95, 0x03, //     REPORT_COUNT(3)  //0x03��갴������
	0x75, 0x01, //     REPORT_SIZE(1)
	0x81, 0x02, //     INPUT(Data,Var,Abs)
	0x95, 0x01, //     REPORT_COUNT(1)
	0x75, 0x05, //     REPORT_SIZE(5)  //��Ҫ������ٸ�bitʹ�ü�����갴��������3��bitλ��1���ֽ�8bit
	0x81, 0x03, //     INPUT(Data,Var, Abs)
	0x05, 0x01, //     USAGE_PAGE(Generic Desktop)
	0x09, 0x30, //     USAGE(X)       X�ƶ�
	0x09, 0x31, //     USAGE(Y)       Y�ƶ�
	0x09, 0x38, //     USAGE(Wheel)   ��ֱ����
	0x15, 0x81, //     LOGICAL_MINIMUM(-127)
	0x25, 0x7F, //     LOGICAL_MAXIMUM(127)
	0x75, 0x08, //     REPORT_SIZE(8)
	0x95, 0x03, //     REPORT_COUNT(3)
	0x81, 0x06, //     INPUT(Data,Var, Rel) //X,Y,��ֱ�������������� ���ֵ

	//�±�ˮƽ����
	0x05, 0x0C, //     USAGE_PAGE (Consumer Devices)
	0x0A, 0x38, 0x02, // USAGE(AC Pan)
	0x15, 0x81, //       LOGICAL_MINIMUM(-127)
	0x25, 0x7F, //       LOGICAL_MAXIMUM(127)
	0x75, 0x08, //       REPORT_SIZE(8)
	0x95, 0x01, //       REPORT_COUNT(1)
	0x81, 0x06, //       INPUT(data,Var, Rel) //ˮƽ���֣����ֵ
	0xC0,       //       End Connection(PhySical)
	0xC0,       //     End Connection

	/////////////

};

CONST HID_DESCRIPTOR DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0100, // hid spec release
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(MouseReportDescriptor) }  // total length of report descriptor
};


///���״̬����,��Ӧ��HID���ϱߵı���
#pragma pack(1)
struct mouse_report_t
{
	BYTE    report_id;
	BYTE    button; //0x000000mrl 8bitֵ,mrl�ֱ��Ӧm�м�r�Ҽ���l�����״̬��
	CHAR    dx;
	CHAR    dy;
	CHAR    v_wheel; // ��ֱ
	CHAR    h_wheel; // ˮƽ
};
#pragma pack()

