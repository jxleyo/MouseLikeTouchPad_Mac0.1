#include "MouseLikeTouchPad.h"
#include "apple_tp_proto.h"
#include<math.h>
extern "C" int _fltused = 0;


#define MAXFINGER_CNT 10

#define MouseButtonPointer_IntervalThreshold         12   // ��������Ҽ���ָ��������������ֵ��

#define Jitter_Offset         2    // ������������΢������λ����ֵ
#define Exception_Offset         2    // ����������Ư���쳣��λ����ֵ


struct MouseLikeTouchPad_state
{
	MOUSEEVENTCALLBACK mEvt_cbk;
	void* mEvt_param;
	ULONG tick_count;
};


//����׷�ٵ���ָ
static struct SPI_TRACKPAD_FINGER lastfinger[MAXFINGER_CNT];
static struct SPI_TRACKPAD_FINGER currentfinger[MAXFINGER_CNT];
static BYTE currentfinger_count; //���嵱ǰ����������
static BYTE lastfinger_count; //�����ϴδ���������

static char Mouse_Pointer_CurrentIndexID; //���嵱ǰ���ָ�봥������������������ţ�-1Ϊδ����
static char Mouse_LButton_CurrentIndexID; //���嵱ǰ��������������������������ţ�-1Ϊδ����
static char Mouse_RButton_CurrentIndexID; //���嵱ǰ����Ҽ���������������������ţ�-1Ϊδ����
static char Mouse_MButton_CurrentIndexID; //���嵱ǰ����м���������������������ţ�-1Ϊδ����
static char Mouse_Wheel_CurrentIndexID; //���嵱ǰ�����ָ����ο���ָ��������������������ţ�-1Ϊδ����

static char Mouse_Pointer_LastIndexID; //�����ϴ����ָ�봥������������������ţ�-1Ϊδ����
static char Mouse_LButton_LastIndexID; //�����ϴ���������������������������ţ�-1Ϊδ����
static char Mouse_RButton_LastIndexID; //�����ϴ�����Ҽ���������������������ţ�-1Ϊδ����
static char Mouse_MButton_LastIndexID; //�����ϴ�����м���������������������ţ�-1Ϊδ����
static char Mouse_Wheel_LastIndexID; //�����ϴ������ָ����ο���ָ��������������������ţ�-1Ϊδ����

static BOOLEAN Mouse_LButton_Status; //������ʱ������״̬��0Ϊ�ͷţ�1Ϊ����
static BOOLEAN Mouse_MButton_Status; //������ʱ����м�״̬��0Ϊ�ͷţ�1Ϊ����
static BOOLEAN Mouse_RButton_Status; //������ʱ����Ҽ�״̬��0Ϊ�ͷţ�1Ϊ����

static BOOLEAN Mouse_Wheel_mode; //����������״̬��0Ϊ����δ���1Ϊ���ּ���
static BOOLEAN Mouse_MButton_Enabled; //����м����ܿ�����־

static BYTE JitterFixStep;         // ���������㶶����������״̬
static BYTE MouseButtonPointer_IntervalCount;//ָ�����Ҳ����ָ����ʱ����ָ�붨����ʼʱ��ļ�������������걨��Ƶ�ʹ����ʱ����

static BYTE Scroll_IntervalCount; //�����������������

static mouse_event_t mEvt;//��������¼��ṹ��
static struct SPI_TRACKPAD_PACKET* PtpReport;//���崥���屨������ָ��
static struct SPI_TRACKPAD_FINGER* pFinger_data;//���崥���屨����ָ����������ָ��

//��ʱ��������ɾ�̬�ķ�ֹ�洢��ջ���
static float dx;
static float dy;
static float distance;
static float px;
static float py;
static float lx;
static float ly;
static float rx;
static float ry;
static float mx;
static float my;
static float wx;
static float wy;
static int direction_hscale;//�����������ű���
static int direction_vscale;//�����������ű���


//������ָͷ�ߴ��С
static float thumb_width;//��ָͷ���
static float thumb_height;//��ָͷ�߶�
static float thumb_scale;//��ָͷ�ߴ����ű���
static float FingerTracingMaxOffset;//����׷�ٵ��β������ʱ�������ָ���λ����
static float FingerMinDistance;//������Ч��������ָ��С����(��FingerTracingMaxOffset��ֱ�ӹ�ϵ)
static float FingerClosedThresholdDistance;//����������ָ��£ʱ����С����(��FingerTracingMaxOffset��ֱ�ӹ�ϵ)
static float FingerMaxDistance;//������Ч��������ָ������
static float TouchPad_DPI;//���崥����ֱ���
static float PointerSensitivity;//����ָ�������ȼ�ָ����ƶ������ű���


//�����൱��ȫ�ֱ�����tp
static MouseLikeTouchPad_state tp_state;
#define tp  (&tp_state)

/////////////////

VOID RegDebug(WCHAR* strValueName, PVOID dataValue, ULONG datasizeValue)//RegDebug(L"Run debug here",pBuffer,pBufferSize);//RegDebug(L"Run debug here",NULL,0x12345678);
{
	//��ʼ��ע�����
	UNICODE_STRING stringKey;
	RtlInitUnicodeString(&stringKey, L"\\Registry\\Machine\\Software\\RegDebug");

	//��ʼ��OBJECT_ATTRIBUTES�ṹ
	OBJECT_ATTRIBUTES  ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, &stringKey, OBJ_CASE_INSENSITIVE, NULL, NULL);

	//����ע�����
	HANDLE hKey;
	ULONG Des;
	NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, &Des);
	if (NT_SUCCESS(status))
	{
		if (Des == REG_CREATED_NEW_KEY)
		{
			KdPrint(("�½�ע����\n"));
		}
		else
		{
			KdPrint(("Ҫ������ע������Ѿ����ڣ�\n"));
		}
	}
	else {
		return;
	}

	//��ʼ��valueName
	UNICODE_STRING valueName;
	RtlInitUnicodeString(&valueName, strValueName);

	if (dataValue == NULL) {
		//����REG_DWORD��ֵ
		status = ZwSetValueKey(hKey, &valueName, 0, REG_DWORD, &datasizeValue, 4);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("����REG_DWORD��ֵʧ�ܣ�\n"));
		}
	}
	else {
		//����REG_BINARY��ֵ
		status = ZwSetValueKey(hKey, &valueName, 0, REG_BINARY, dataValue, datasizeValue);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("����REG_BINARY��ֵʧ�ܣ�\n"));
		}
	}
	ZwFlushKey(hKey);
	//�ر�ע�����
	ZwClose(hKey);
}



void MouseLikeTouchPad_init(MOUSEEVENTCALLBACK cbk, void* param)
{
	RtlZeroMemory(tp, sizeof(MouseLikeTouchPad_state));

	tp->tick_count = KeQueryTimeIncrement(); ///
	tp->mEvt_cbk = cbk;
	tp->mEvt_param = param;
}

static __forceinline short abs(short x)
{
	if (x < 0)return -x;
	return x;
}

void MouseLikeTouchPad_parse_init()
{
		Mouse_Pointer_CurrentIndexID = -1; //���嵱ǰ���ָ�봥������������������ţ�-1Ϊδ����
		Mouse_LButton_CurrentIndexID = -1; //���嵱ǰ��������������������������ţ�-1Ϊδ����
		Mouse_RButton_CurrentIndexID = -1; //���嵱ǰ����Ҽ���������������������ţ�-1Ϊδ����
		Mouse_MButton_CurrentIndexID = -1; //���嵱ǰ����м���������������������ţ�-1Ϊδ����
		Mouse_Wheel_CurrentIndexID = -1; //���嵱ǰ�����ָ����ο���ָ��������������������ţ�-1Ϊδ����
		
		Mouse_Pointer_LastIndexID = -1; //�����ϴ����ָ�봥������������������ţ�-1Ϊδ����
		Mouse_LButton_LastIndexID = -1; //�����ϴ���������������������������ţ�-1Ϊδ����
		Mouse_RButton_LastIndexID = -1; //�����ϴ�����Ҽ���������������������ţ�-1Ϊδ����
		Mouse_MButton_LastIndexID = -1; //�����ϴ�����м���������������������ţ�-1Ϊδ����
		Mouse_Wheel_LastIndexID = -1; //�����ϴ������ָ����ο���ָ��������������������ţ�-1Ϊδ����

		Mouse_LButton_Status = 0; //������ʱ������״̬��0Ϊ�ͷţ�1Ϊ����
		Mouse_MButton_Status = 0;//������ʱ����м�״̬��0Ϊ�ͷţ�1Ϊ����
		Mouse_RButton_Status = 0; //������ʱ����Ҽ�״̬��0Ϊ�ͷţ�1Ϊ����

		Mouse_Wheel_mode = 0;

		lastfinger_count = 0;
		currentfinger_count = 0;
		
		MouseButtonPointer_IntervalCount = 0;
		JitterFixStep = 0;

		Scroll_IntervalCount = 0;

		Mouse_MButton_Enabled = FALSE;//Ĭ�Ϲر��м�����

		//��ȡ������ֱ���
		TouchPad_DPI = 100; //��ˮƽ100��/mmΪ��׼
		//��̬������ָͷ��С����
		thumb_width = 18;//��ָͷ���,Ĭ������ָ18mm��Ϊ��׼
		thumb_scale = 1.0;//��ָͷ�ߴ����ű�����
		FingerTracingMaxOffset = 6 * TouchPad_DPI * thumb_scale;//����׷�ٵ��β������ʱ�������ָ���λ��������
		FingerMinDistance = 12 * TouchPad_DPI * thumb_scale;//������Ч��������ָ��С����(��FingerTracingMaxOffset��ֱ�ӹ�ϵ)
		FingerClosedThresholdDistance = 18 * TouchPad_DPI * thumb_scale;//����������ָ��£ʱ����С����(��FingerTracingMaxOffset��ֱ�ӹ�ϵ)
		FingerMaxDistance = FingerMinDistance * 4;//������Ч��������ָ������(FingerMinDistance*4)  
		PointerSensitivity = TouchPad_DPI / 20;
}

void MouseLikeTouchPad_parse(UINT8* data, LONG length)
{
	PtpReport = (struct SPI_TRACKPAD_PACKET*)data;
	if (length < TP_HEADER_SIZE || length < TP_HEADER_SIZE + TP_FINGER_SIZE * PtpReport->NumOfFingers || PtpReport->NumOfFingers >= MAXFINGER_CNT) return; //

	//���浱ǰ��ָ����
	if (!PtpReport->IsFinger) {//is_finger�����ж���ָȫ���뿪��������pr->state & 0x80)��pr->state �ж���Ϊ��㴥��ʱ����һ����ָ�뿪����������źţ�Ҳ������pr->finger_number�ж���Ϊ��ֵ����Ϊ0
		currentfinger_count = 0;
	}
	else {
		currentfinger_count = PtpReport->NumOfFingers;
		if (currentfinger_count > 0) {
			for (char i = 0; i < currentfinger_count; i++) {
				pFinger_data = (struct SPI_TRACKPAD_FINGER*)(data + TP_HEADER_SIZE + i * TP_FINGER_SIZE); //
				currentfinger[i] = *pFinger_data;
			}
		}
	}

	//��ʼ������¼�
	mEvt.button = 0;
	mEvt.dx = 0;
	mEvt.dy = 0;
	mEvt.h_wheel = 0;
	mEvt.v_wheel = 0;
	Mouse_LButton_Status = 0; //������ʱ������״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�
	Mouse_MButton_Status = 0; //������ʱ����м�״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�
	Mouse_RButton_Status = 0; //������ʱ����Ҽ�״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�


	//������ָ���������
	//��ʼ����ǰ�����������ţ����ٺ�δ�ٸ�ֵ�ı�ʾ��������
	Mouse_Pointer_CurrentIndexID = -1;
	Mouse_LButton_CurrentIndexID = -1;
	Mouse_RButton_CurrentIndexID = -1;
	Mouse_MButton_CurrentIndexID = -1;
	Mouse_Wheel_CurrentIndexID = -1;

	//���Ѿ����ڵ�ָ�롢������������и���
	for (char i = 0; i < currentfinger_count; i++) {
		if (i== Mouse_Pointer_CurrentIndexID  || i == Mouse_LButton_CurrentIndexID ||i == Mouse_RButton_CurrentIndexID || i == Mouse_MButton_CurrentIndexID || i == Mouse_Wheel_CurrentIndexID){//�����Ѿ�׷�ٵĴ�����,i����ֵ���Բ���Ҫ�ж�ָ�밴�������Ƿ����
			continue;
		}
		if (Mouse_Pointer_LastIndexID !=-1 && currentfinger[i].OriginalX == lastfinger[Mouse_Pointer_LastIndexID].OriginalX && currentfinger[i].OriginalY == lastfinger[Mouse_Pointer_LastIndexID].OriginalY) {//���ٲ���ָ��
			Mouse_Pointer_CurrentIndexID = i;
			continue;
		}

		if (Mouse_LButton_LastIndexID != -1 && currentfinger[i].OriginalX == lastfinger[Mouse_LButton_LastIndexID].OriginalX && currentfinger[i].OriginalY == lastfinger[Mouse_LButton_LastIndexID].OriginalY) {//���ٲ������
			Mouse_LButton_Status = 1; //�ҵ������
			Mouse_LButton_CurrentIndexID = i;
			continue;
		}

		if (Mouse_RButton_LastIndexID != -1 && currentfinger[i].OriginalX == lastfinger[Mouse_RButton_LastIndexID].OriginalX && currentfinger[i].OriginalY == lastfinger[Mouse_RButton_LastIndexID].OriginalY) {//���ٲ����Ҽ�
			Mouse_RButton_Status = 1; //�ҵ������
			Mouse_RButton_CurrentIndexID = i;
			continue;
		}

		if (Mouse_MButton_LastIndexID != -1 && currentfinger[i].OriginalX == lastfinger[Mouse_MButton_LastIndexID].OriginalX && currentfinger[i].OriginalY == lastfinger[Mouse_MButton_LastIndexID].OriginalY) {//���ٲ����м�
			Mouse_MButton_Status = 1; //�ҵ������
			Mouse_MButton_CurrentIndexID = i;
			continue;
		}

		if (Mouse_Wheel_LastIndexID != -1 && currentfinger[i].OriginalX == lastfinger[Mouse_Wheel_LastIndexID].OriginalX && currentfinger[i].OriginalY == lastfinger[Mouse_Wheel_LastIndexID].OriginalY) {//���ٲ��ҹ��ָ�����
			Mouse_Wheel_CurrentIndexID = i;
			continue;
		}
	}

	//��ʼ����¼��߼��ж�
	if (currentfinger_count >3 && PtpReport->ClickOccurred) {
		Mouse_MButton_Enabled = !Mouse_MButton_Enabled;//3ָ���ϴ�������ѹʱ������ʱ�л�����/�ر�����м�����
	}

	if (Mouse_Pointer_LastIndexID==-1 && currentfinger_count ==1) {//���ָ�롢������Ҽ����м���δ����
			Mouse_Pointer_CurrentIndexID = 0;  //�׸���������Ϊָ��
			MouseButtonPointer_IntervalCount = 0;//ָ�����Ҳ����ָ����ʱ����ָ�붨����ʼʱ��ļ������
	}	
	else if (Mouse_Pointer_CurrentIndexID == -1 && Mouse_Pointer_LastIndexID != -1) {//ָ����ʧ
		Mouse_Wheel_mode = FALSE;//��������ģʽ
		//����������ܶ���
		Mouse_LButton_CurrentIndexID = -1;
		Mouse_RButton_CurrentIndexID = -1;
		Mouse_MButton_CurrentIndexID = -1;
		Mouse_Wheel_CurrentIndexID = -1;

		MouseButtonPointer_IntervalCount = 0;
	}
	else if (Mouse_Pointer_CurrentIndexID != -1 && !Mouse_Wheel_mode) {//ָ���Ѷ���ķǹ����¼�����
		//����ָ���������Ҳ��Ƿ��в�£����ָ��Ϊ����ģʽ���߰���ģʽ����ָ�����/�Ҳ����ָ����ʱ����ָ����ָ����ʱ����С���趨��ֵʱ�ж�Ϊ�����ַ���Ϊ��갴������һ��������Ч���𰴼�����ֲ���,����갴���͹��ֲ���һ��ʹ��
		if (MouseButtonPointer_IntervalCount<127) {
			MouseButtonPointer_IntervalCount++;//ָ�����Ҳ����ָ����ʱ����ָ�붨����ʼʱ��ļ�������������걨��Ƶ�ʹ����ʱ����
		}
		
		if (currentfinger_count > lastfinger_count) {//����������ʱ���Ұ���������¼�
			for (char i = 0; i < currentfinger_count; i++) {
				if (i == Mouse_Pointer_CurrentIndexID || i == Mouse_LButton_CurrentIndexID || i == Mouse_RButton_CurrentIndexID || i == Mouse_MButton_CurrentIndexID) {//�����Ѿ�׷�ٵĴ�����,i����ֵ���Բ���Ҫ�ж�ָ�밴�������Ƿ����
					continue;
				}

				dx = (float)(currentfinger[i].X - currentfinger[Mouse_Pointer_CurrentIndexID].X);
				dy = (float)(currentfinger[i].Y - currentfinger[Mouse_Pointer_CurrentIndexID].Y);
				distance = sqrt(dx * dx + dy * dy);//��������ָ��ľ���

				if (!Mouse_MButton_Enabled) {//����м����ܹر�ʱ
					//������м����Ҽ����µ�����²���ת��Ϊ����ģʽ��Ҫ�ų������ж�
					if ((Mouse_LButton_CurrentIndexID == -1 && Mouse_LButton_CurrentIndexID == -1 && Mouse_LButton_CurrentIndexID == -1) && \
						abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && MouseButtonPointer_IntervalCount < MouseButtonPointer_IntervalThreshold) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ

						Mouse_Wheel_mode = TRUE;  //��������ģʽ
						Mouse_Wheel_CurrentIndexID = i;//���ָ����ο���ָ����ֵ
						MouseButtonPointer_IntervalCount = 0;
						break;
					}
					else if (Mouse_LButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״��
						Mouse_LButton_Status = 1; //�ҵ������
						Mouse_LButton_CurrentIndexID = i;
						continue;  //������������
					}
					else if (Mouse_RButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
						Mouse_RButton_Status = 1; //�ҵ��Ҽ�
						Mouse_RButton_CurrentIndexID = i;
						continue;  //������������
					}
				}
				else {//����м����ܿ���ʱ
					//������м����Ҽ����µ�����²���ת��Ϊ����ģʽ��Ҫ�ų������ж�
					if ((Mouse_LButton_CurrentIndexID == -1 && Mouse_LButton_CurrentIndexID == -1 && Mouse_LButton_CurrentIndexID == -1) && \
						abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && MouseButtonPointer_IntervalCount < MouseButtonPointer_IntervalThreshold) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ
						Mouse_Wheel_mode = TRUE;  //��������ģʽ
						Mouse_Wheel_CurrentIndexID = i;//���ָ����ο���ָ����ֵ
						MouseButtonPointer_IntervalCount = 0;
						break;
					}
					else if (Mouse_MButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && MouseButtonPointer_IntervalCount > MouseButtonPointer_IntervalThreshold && dx < 0) {//ָ������в�£����ָ���²�����ָ����ָ��ʼ����ʱ����������ֵ
						Mouse_MButton_Status = 1; //�ҵ��м�
						Mouse_MButton_CurrentIndexID = i;
						MouseButtonPointer_IntervalCount = 0;
						continue;  //��������������ʳָ�Ѿ����м�ռ������ԭ��������Ѿ�������
					}
					else if (Mouse_LButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״����
						Mouse_LButton_Status = 1; //�ҵ����
						Mouse_LButton_CurrentIndexID = i;
						continue;  //������������
					}
					else if (Mouse_RButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
						Mouse_RButton_Status = 1; //�ҵ��Ҽ�
						Mouse_RButton_CurrentIndexID = i;
						continue;  //������������
					}
				}
			}
		}

		if (!Mouse_Wheel_mode) {//�ǹ���ģʽ�����ָ��λ������
			if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݿ��ܲ��ȶ�ָ������ͻ����Ư����Ҫ����
				JitterFixStep = 1;
			}
			else {
				px = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].X - lastfinger[Mouse_Pointer_LastIndexID].X) / thumb_scale;
				py = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].Y - lastfinger[Mouse_Pointer_LastIndexID].Y) / thumb_scale;

				if (JitterFixStep == 1) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
					JitterFixStep=0;
					px = 0;
					py = 0;
				}
				else {
					if (JitterFixStep > 0 && JitterFixStep < 10) {
						JitterFixStep++;
					}

					if (abs(px) <= Jitter_Offset) {//ָ����΢��������
						px = 0;
					}
					if (abs(py) <= Jitter_Offset) {//ָ����΢��������
						py = 0;
					}
					//������ָ�ǳ�������ָʱ���ο�λ����������	
					if (Mouse_LButton_CurrentIndexID != -1) {
						lx = (float)(currentfinger[Mouse_LButton_CurrentIndexID].X - lastfinger[Mouse_LButton_LastIndexID].X) / thumb_scale;
						if (abs(lx - px) > Exception_Offset || (lx > 0 && px < 0) || (lx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
					}
					if (Mouse_RButton_CurrentIndexID != -1) {
						rx = (float)(currentfinger[Mouse_RButton_CurrentIndexID].X - lastfinger[Mouse_RButton_LastIndexID].X) / thumb_scale;
						if (abs(rx - px) > Exception_Offset || (rx > 0 && px < 0) || (rx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
					}
					if (Mouse_MButton_CurrentIndexID != -1) {
						mx = (float)(currentfinger[Mouse_MButton_CurrentIndexID].X - lastfinger[Mouse_MButton_LastIndexID].X) / thumb_scale;
						if (abs(mx - px) > Exception_Offset || (mx > 0 && px < 0) || (mx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
					}

					if (Mouse_LButton_CurrentIndexID != -1) {
						ly = (float)(currentfinger[Mouse_LButton_CurrentIndexID].Y - lastfinger[Mouse_LButton_LastIndexID].Y) / thumb_scale;
						if (abs(ly - py) > Exception_Offset || (ly > 0 && py < 0) || (ly < 0 && py > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
					}
					if (Mouse_RButton_CurrentIndexID != -1) {
						ry = (float)(currentfinger[Mouse_RButton_CurrentIndexID].Y - lastfinger[Mouse_RButton_LastIndexID].Y) / thumb_scale;
						if (abs(ry - py) > Exception_Offset || (ry > 0 && py < 0) || (ry < 0 && py > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
					}
					if (Mouse_MButton_CurrentIndexID != -1) {
						my = (float)(currentfinger[Mouse_MButton_CurrentIndexID].Y - lastfinger[Mouse_MButton_LastIndexID].Y) / thumb_scale;
						if (abs(my - py) > Exception_Offset || (my > 0 && py < 0) || (my < 0 && py > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
					}
				}
				mEvt.dx = (short)(px / PointerSensitivity);
				mEvt.dy = -(short)(py / PointerSensitivity);
			}
		}
	}
	else if (Mouse_Pointer_CurrentIndexID != -1 && Mouse_Wheel_mode) {//���ֲ���ģʽ
		//���ָ��λ������
		if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݿ��ܲ��ȶ�ָ������ͻ����Ư����Ҫ����
			JitterFixStep = 1;
		}
		else {
			px = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].X - lastfinger[Mouse_Pointer_LastIndexID].X) / thumb_scale;
			py = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].Y - lastfinger[Mouse_Pointer_LastIndexID].Y) / thumb_scale;

			if (JitterFixStep == 1) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
				JitterFixStep = 0;
				px = 0;
				py = 0;
			}
			else {
				if (JitterFixStep >0 && JitterFixStep <10) {
					JitterFixStep++;
				}

				if (abs(px) <= Jitter_Offset) {//ָ����΢��������
					px = 0;
				}
				if (abs(py) <= Jitter_Offset) {//ָ����΢��������
					py = 0;
				}
				//������ָ�ǳ�������ָʱ���ο�λ����������	
				if (Mouse_Wheel_CurrentIndexID != -1) {
					wx = (float)(currentfinger[Mouse_Wheel_CurrentIndexID].X - lastfinger[Mouse_Wheel_LastIndexID].X) / thumb_scale;
					if (abs(wx - px) > Exception_Offset || (wx > 0 && px < 0) || (wx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
						px = 0;
						py = 0;
					}
				}
				if (Mouse_Wheel_CurrentIndexID != -1) {
					wy = (float)(currentfinger[Mouse_Wheel_CurrentIndexID].Y - lastfinger[Mouse_Wheel_LastIndexID].Y) / thumb_scale;
					if (abs(wy - py) > Exception_Offset || (wy > 0 && py < 0) || (wy < 0 && py > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
						px = 0;
						py = 0;
					}
				}
			}

			direction_hscale = 1;//�����������ű���
			direction_vscale = 1;//�����������ű���

			if (abs(px) > abs(py) / 4) {//���������ȶ�������
				direction_hscale = 1;
				direction_vscale = 8;
			}
			if (abs(py) > abs(px) / 4) {//���������ȶ�������
				direction_hscale = 8;
				direction_vscale = 1;
			}

			px = px / direction_hscale;
			py = py / direction_vscale;

			if (abs(px) > 4 && abs(px) < 32) {
				if (Scroll_IntervalCount == 0) {//�ȹ���һ���ټ������
					mEvt.h_wheel = (char)(px > 0 ? 1 : -1);
					Scroll_IntervalCount = 16;
				}
				else {
					Scroll_IntervalCount--;
				}
			}
			else if (abs(px) >= 32) {
				mEvt.h_wheel = (char)(px > 0 ? 1 : -1);
				Scroll_IntervalCount = 0;
			}
			if (abs(py) > 4 && abs(py) < 32) {
				if (Scroll_IntervalCount == 0) {
					mEvt.v_wheel = (char)(py > 0 ? 1 : -1);
					Scroll_IntervalCount = 16;
				}
				else {
					Scroll_IntervalCount--;
				}
			}
			else if (abs(py) >= 32) {
				mEvt.v_wheel = (char)(py > 0 ? 1 : -1);
				Scroll_IntervalCount = 0;
			}
		}
	}
	else {
		//���������Ч
	}
		
	mEvt.button = Mouse_LButton_Status + (Mouse_RButton_Status << 1) + (Mouse_MButton_Status << 2);  //�����Ҽ�״̬�ϳ�
	tp->mEvt_cbk(&mEvt, tp->mEvt_param);//��������¼�

	
	//������һ�����д�����ĳ�ʼ���꼰���ܶ���������
	for (char i = 0; i < currentfinger_count; i++) {
		lastfinger[i] = currentfinger[i];
	}

	lastfinger_count = currentfinger_count;
	Mouse_Pointer_LastIndexID = Mouse_Pointer_CurrentIndexID;
	Mouse_LButton_LastIndexID = Mouse_LButton_CurrentIndexID;
	Mouse_RButton_LastIndexID = Mouse_RButton_CurrentIndexID;
	Mouse_MButton_LastIndexID = Mouse_MButton_CurrentIndexID;
	Mouse_Wheel_LastIndexID = Mouse_Wheel_CurrentIndexID;

}

