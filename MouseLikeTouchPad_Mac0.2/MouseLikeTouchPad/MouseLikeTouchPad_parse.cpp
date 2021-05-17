#include "MouseLikeTouchPad.h"
#include "apple_tp_proto.h"
#include<math.h>
extern "C" int _fltused = 0;


#define MAXFINGER_CNT 10

#define STABLE_INTERVAL_MSEC         100   // ��ָ������������ȶ�ʱ���� 

#define MouseReport_INTERVAL_MSEC     8   // ��걨����ʱ��ms����Ƶ��125hzΪ��׼
#define MButton_Interval_MSEC         200   // ��������Ҽ���ָ��������ʱ��ms��

#define Jitter_Offset         5    // ������������΢������λ����ֵ
#define Exception_Offset         2    // ����������Ư���쳣��λ����ֵ

struct MouseLikeTouchPad_state
{
	MOUSEEVENTCALLBACK evt_cbk;
	void* evt_param;
	ULONG tick_count;
};


struct FingerPosition
{
	short          pos_x;
	short          pos_y;
};

static struct SPI_TRACKPAD_PACKET* PtpReport;//���崥���屨������ָ��
static struct SPI_TRACKPAD_FINGER* pFinger_data;//���崥���屨����ָ����������ָ��

//����׷�ٵ���ָ
static struct FingerPosition lastfinger[MAXFINGER_CNT];
static struct FingerPosition currentfinger[MAXFINGER_CNT];
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
static BOOLEAN Jitter;         // ���������㶶������״̬

static LARGE_INTEGER MousePointer_DefineTime;//���ָ�붨��ʱ�䣬���ڼ��㰴�����ʱ���������ж�����м�͹��ֲ���
static float TouchPad_ReportInterval;//���崥���屨����ʱ��

static BYTE Scroll_IntervalCount; //�����������������

static LARGE_INTEGER last_ticktime; //�ϴα����ʱ
static LARGE_INTEGER current_ticktime;//��ǰ�����ʱ
static LARGE_INTEGER ticktime_Interval;//������ʱ��


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

void MouseLikeTouchPad_init(MOUSEEVENTCALLBACK cbk, void* param)
{
	RtlZeroMemory(tp, sizeof(MouseLikeTouchPad_state));

	tp->tick_count = KeQueryTimeIncrement(); ///
	tp->evt_cbk = cbk;
	tp->evt_param = param;
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
		for (UINT8 i = 0; i < MAXFINGER_CNT; ++i) {
			lastfinger[i].pos_x = 0;
			lastfinger[i].pos_y = 0;
			currentfinger[i].pos_x = 0;
			currentfinger[i].pos_y = 0;
		}

		Scroll_IntervalCount = 0;

		Mouse_MButton_Enabled = FALSE;//Ĭ�Ϲر��м�����
		Jitter = FALSE;

		KeQueryTickCount(&last_ticktime);

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

	//���㱨��Ƶ�ʺ�ʱ����
	KeQueryTickCount(&current_ticktime);
	ticktime_Interval.QuadPart = (current_ticktime.QuadPart - last_ticktime.QuadPart) * tp->tick_count / 10000;//��λms����
	TouchPad_ReportInterval = (float)ticktime_Interval.LowPart;//�����屨����ʱ��ms
	last_ticktime = current_ticktime;

	//���浱ǰ��ָ����
	if (!PtpReport->IsFinger) {//is_finger�����ж���ָȫ���뿪��������pr->state & 0x80)��pr->state �ж���Ϊ��㴥��ʱ����һ����ָ�뿪����������źţ�Ҳ������pr->finger_number�ж���Ϊ��ֵ����Ϊ0
		currentfinger_count = 0;
	}
	else {
		currentfinger_count = PtpReport->NumOfFingers;
		if (currentfinger_count > 0) {
			for (char i = 0; i < currentfinger_count; i++) {
				pFinger_data = (struct SPI_TRACKPAD_FINGER*)(data + TP_HEADER_SIZE + i * TP_FINGER_SIZE); //
				currentfinger[i].pos_x = pFinger_data->X;
				currentfinger[i].pos_y = pFinger_data->Y;
			}
		}
	}

	//��ʼ������¼�
	mouse_event_t mEvt;
	mEvt.button = 0;
	mEvt.dx = 0;
	mEvt.dy = 0;
	mEvt.h_wheel = 0;
	mEvt.v_wheel = 0;
	Mouse_LButton_Status = 0; //������ʱ������״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�
	Mouse_MButton_Status = 0; //������ʱ����м�״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�
	Mouse_RButton_Status = 0; //������ʱ����Ҽ�״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�


	//��ʼ����ǰ�����������ţ����ٺ�δ�ٸ�ֵ�ı�ʾ��������
	Mouse_Pointer_CurrentIndexID = -1;
	Mouse_LButton_CurrentIndexID = -1;
	Mouse_RButton_CurrentIndexID = -1;
	Mouse_MButton_CurrentIndexID = -1;
	Mouse_Wheel_CurrentIndexID = -1;

	//������ָ���������
	for (char i = 0; i < currentfinger_count; i++) {
		if (i == Mouse_Pointer_CurrentIndexID || i == Mouse_LButton_CurrentIndexID || i == Mouse_RButton_CurrentIndexID || i == Mouse_MButton_CurrentIndexID || i == Mouse_Wheel_CurrentIndexID) {//iΪ��ֵ�����������������Ƿ�Ϊ-1
				continue;// �Ѿ����������
		}

		if (Mouse_Pointer_LastIndexID != -1) {
			short dx = currentfinger[i].pos_x - lastfinger[Mouse_Pointer_LastIndexID].pos_x;
			short dy = currentfinger[i].pos_y - lastfinger[Mouse_Pointer_LastIndexID].pos_y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_Pointer_CurrentIndexID = i;//�ҵ�ָ��
				continue;//������������
			}
		}
		if (Mouse_Wheel_LastIndexID != -1) {
			short dx = currentfinger[i].pos_x - lastfinger[Mouse_Wheel_LastIndexID].pos_x;
			short dy = currentfinger[i].pos_y - lastfinger[Mouse_Wheel_LastIndexID].pos_y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_Wheel_CurrentIndexID = i;//�ҵ����
				continue;//������������
			}
		}
		if (Mouse_LButton_LastIndexID != -1) {
			short dx = currentfinger[i].pos_x - lastfinger[Mouse_LButton_LastIndexID].pos_x;
			short dy = currentfinger[i].pos_y - lastfinger[Mouse_LButton_LastIndexID].pos_y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_LButton_Status = 1; //�ҵ������
				Mouse_LButton_CurrentIndexID = i;//�ҵ����
				continue;//������������
			}
		}

		if (Mouse_RButton_LastIndexID != -1) {
			short dx = currentfinger[i].pos_x - lastfinger[Mouse_RButton_LastIndexID].pos_x;
			short dy = currentfinger[i].pos_y - lastfinger[Mouse_RButton_LastIndexID].pos_y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_RButton_Status = 1; //�ҵ������
				Mouse_RButton_CurrentIndexID = i;//�ҵ����
				continue;//������������
			}
		}
		if (Mouse_MButton_LastIndexID != -1) {
			short dx = currentfinger[i].pos_x - lastfinger[Mouse_MButton_LastIndexID].pos_x;
			short dy = currentfinger[i].pos_y - lastfinger[Mouse_MButton_LastIndexID].pos_y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_MButton_Status = 1; //�ҵ��м���
				Mouse_MButton_CurrentIndexID = i;//�ҵ��м�
				continue;//������������
			}
		}
		
	}


	//��ʼ����¼��߼��ж�
	if (currentfinger_count > 3 && PtpReport->ClickOccurred) {
		Mouse_MButton_Enabled = !Mouse_MButton_Enabled;//3ָ���ϴ�������ѹʱ������ʱ�л�����/�ر�����м�����
	}

	//ע�����ָ��ͬʱ���ٽӴ�������ʱ�����屨����ܴ���һ֡��ͬʱ��������������������Բ����õ�ǰֻ��һ����������Ϊ����ָ����ж�����������Ҳ������Mouse_Pointer_LastIndexID==-1��Ϊ��ָ�붨����������������ʣ����ָ���ں�����
	if (lastfinger_count == 0 && currentfinger_count > 0) {//���ָ�롢������Ҽ����м���δ����,
		Mouse_Pointer_CurrentIndexID = 0;  //�׸���������Ϊָ��
		MousePointer_DefineTime = current_ticktime;//���嵱ǰָ����ʼʱ��
	}
	else if (Mouse_Pointer_CurrentIndexID == -1 && Mouse_Pointer_LastIndexID != -1) {//ָ����ʧ
		Mouse_Wheel_mode = FALSE;//��������ģʽ

		Mouse_Pointer_CurrentIndexID = -1;
		Mouse_LButton_CurrentIndexID = -1;
		Mouse_RButton_CurrentIndexID = -1;
		Mouse_MButton_CurrentIndexID = -1;
		Mouse_Wheel_CurrentIndexID = -1;
	}
	else if (Mouse_Pointer_CurrentIndexID != -1 && !Mouse_Wheel_mode) {//ָ���Ѷ���ķǹ����¼�����
		//����ָ���������Ҳ��Ƿ��в�£����ָ��Ϊ����ģʽ���߰���ģʽ����ָ�����/�Ҳ����ָ����ʱ����ָ����ָ����ʱ����С���趨��ֵʱ�ж�Ϊ�����ַ���Ϊ��갴������һ��������Ч���𰴼�����ֲ���,����갴���͹��ֲ���һ��ʹ��
		//�߼�Ԥ��Ϊ������м����ܻ���ʳָ�����л���Ҫ̧��ʳָ����иı䣬���/�м�/�Ҽ����µ�����²���ת��Ϊ����ģʽ���Կ���������
		LARGE_INTEGER MouseButton_Interval;
		MouseButton_Interval.QuadPart = (current_ticktime.QuadPart - MousePointer_DefineTime.QuadPart) * tp->tick_count / 10000;//��λms����
		float Mouse_Button_Interval = (float)MouseButton_Interval.LowPart;//ָ�����Ҳ����ָ����ʱ����ָ�붨����ʼʱ��ļ��ms

		for (char i = 0; i < currentfinger_count; i++) {
			if (i == Mouse_Pointer_CurrentIndexID || i == Mouse_LButton_CurrentIndexID || i == Mouse_RButton_CurrentIndexID || i == Mouse_MButton_CurrentIndexID || i == Mouse_Wheel_CurrentIndexID) {//iΪ��ֵ�����������������Ƿ�Ϊ-1
				continue;  // �Ѿ����������
			}
			float dx = (float)(currentfinger[i].pos_x - currentfinger[Mouse_Pointer_CurrentIndexID].pos_x);
			float dy = (float)(currentfinger[i].pos_y - currentfinger[Mouse_Pointer_CurrentIndexID].pos_y);
			float distance = sqrt(dx * dx + dy * dy);//��������ָ��ľ���

			if (!Mouse_MButton_Enabled) {//����м����ܹر�ʱ
				if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval < MButton_Interval_MSEC) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ
					Mouse_Wheel_mode = TRUE;  //��������ģʽ
					Mouse_Wheel_CurrentIndexID = i;//���ָ����ο���ָ����ֵ

					Mouse_LButton_CurrentIndexID = -1;
					Mouse_RButton_CurrentIndexID = -1;
					Mouse_MButton_CurrentIndexID = -1;
					break;
				}
				else if (Mouse_LButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״��
					Mouse_LButton_Status = 1; //�ҵ������
					Mouse_LButton_CurrentIndexID = i;//��ֵ�����������������
					continue;  //��������������
				}
				else if (Mouse_RButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
					Mouse_RButton_Status = 1; //�ҵ��Ҽ�
					Mouse_RButton_CurrentIndexID = i;//��ֵ�Ҽ���������������
					continue;  //��������������
				}
			}
			else {//����м����ܿ���ʱ
				if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval < MButton_Interval_MSEC) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ
					Mouse_Wheel_mode = TRUE;  //��������ģʽ
					Mouse_Wheel_CurrentIndexID = i;//���ָ����ο���ָ����ֵ

					Mouse_LButton_CurrentIndexID = -1;
					Mouse_RButton_CurrentIndexID = -1;
					Mouse_MButton_CurrentIndexID = -1;
					break;
				}
				else if (Mouse_MButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval > MButton_Interval_MSEC && dx < 0) {//ָ������в�£����ָ���²�����ָ����ָ��ʼ����ʱ����������ֵ
					Mouse_MButton_Status = 1; //�ҵ��м�
					Mouse_MButton_CurrentIndexID = i;//��ֵ�м���������������
					continue;  //����������������ʳָ�Ѿ����м�ռ������ԭ��������Ѿ�������
				}
				else if (Mouse_LButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״����
					Mouse_LButton_Status = 1; //�ҵ����
					Mouse_LButton_CurrentIndexID = i;//��ֵ�����������������
					continue;  //��������������
				}
				else if (Mouse_RButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
					Mouse_RButton_Status = 1; //�ҵ��Ҽ�
					Mouse_RButton_CurrentIndexID = i;//��ֵ�Ҽ���������������
					continue;  //��������������
				}
			}
		}

		//���ָ��λ������
		if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݿ��ܲ��ȶ�ָ������ͻ����Ư����Ҫ����
			Jitter = TRUE;
		}
		else{
			float px = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].pos_x - lastfinger[Mouse_Pointer_LastIndexID].pos_x) / thumb_scale;
			float py = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].pos_y - lastfinger[Mouse_Pointer_LastIndexID].pos_y) / thumb_scale;

			if (Jitter) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
				Jitter = FALSE;
				px =0;
				py= 0;
			}
			else {
				if (abs(px) <=Jitter_Offset) {//ָ����΢��������
					px = 0;
				}
				if (abs(py) <= Jitter_Offset) {//ָ����΢��������
					py = 0;
				}
					//������ָ�ǳ�������ָʱ���ο�λ����������	
				if (Mouse_LButton_CurrentIndexID != -1) {
						float lx = (float)(currentfinger[Mouse_LButton_CurrentIndexID].pos_x - lastfinger[Mouse_LButton_LastIndexID].pos_x) / thumb_scale;
						if (abs(lx - px) > Exception_Offset|| (lx>0 && px<0) || (lx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
				}
				if (Mouse_RButton_CurrentIndexID != -1) {
					float rx = (float)(currentfinger[Mouse_RButton_CurrentIndexID].pos_x - lastfinger[Mouse_RButton_LastIndexID].pos_x) / thumb_scale;
					if (abs(rx - px) > Exception_Offset || (rx > 0 && px < 0) || (rx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
						px = 0;
						py = 0;
					}
				}
				if (Mouse_MButton_CurrentIndexID != -1) {
					float mx = (float)(currentfinger[Mouse_MButton_CurrentIndexID].pos_x - lastfinger[Mouse_MButton_LastIndexID].pos_x) / thumb_scale;
					if (abs(mx - px) > Exception_Offset || (mx > 0 && px < 0) || (mx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
						px = 0;
						py = 0;
					}
				}
				
				if (Mouse_LButton_CurrentIndexID != -1) {
						float ly = (float)(currentfinger[Mouse_LButton_CurrentIndexID].pos_y - lastfinger[Mouse_LButton_LastIndexID].pos_y) / thumb_scale;
						if (abs(ly - py) > Exception_Offset || (ly > 0 && py < 0) || (ly < 0 && py > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
							px = 0;
							py = 0;
						}
				}
				if (Mouse_RButton_CurrentIndexID != -1) {
					float ry = (float)(currentfinger[Mouse_RButton_CurrentIndexID].pos_y - lastfinger[Mouse_RButton_LastIndexID].pos_y) / thumb_scale;
					if (abs(ry - py) > Exception_Offset || (ry > 0 && py < 0) || (ry < 0 && py > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
						px = 0;
						py = 0;
					}
				}
				if (Mouse_MButton_CurrentIndexID != -1) {
					float my = (float)(currentfinger[Mouse_MButton_CurrentIndexID].pos_y - lastfinger[Mouse_MButton_LastIndexID].pos_y) / thumb_scale;
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
	else if (Mouse_Pointer_CurrentIndexID != -1 && Mouse_Wheel_mode) {//���ֲ���ģʽ
		//���ָ��λ������
		if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݿ��ܲ��ȶ�ָ������ͻ����Ư����Ҫ����
			Jitter = TRUE;
		}
		else {
			float px = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].pos_x - lastfinger[Mouse_Pointer_LastIndexID].pos_x) / thumb_scale;
			float py = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].pos_y - lastfinger[Mouse_Pointer_LastIndexID].pos_y) / thumb_scale;

			if (Jitter) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
				Jitter = FALSE;
				px = 0;
				py = 0;
			}
			else {
				if (abs(px) <= Jitter_Offset) {//ָ����΢��������
					px = 0;
				}
				if (abs(py) <= Jitter_Offset) {//ָ����΢��������
					py = 0;
				}
				//������ָ�ǳ�������ָʱ���ο�λ����������	
				if (Mouse_Wheel_CurrentIndexID != -1) {
					float wx = (float)(currentfinger[Mouse_Wheel_CurrentIndexID].pos_x - lastfinger[Mouse_Wheel_LastIndexID].pos_x) / thumb_scale;
					if (abs(wx - px) > Exception_Offset || (wx > 0 && px < 0) || (wx < 0 && px > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
						px = 0;
						py = 0;
					}
				}
				if (Mouse_Wheel_CurrentIndexID != -1) {
					float wy = (float)(currentfinger[Mouse_Wheel_CurrentIndexID].pos_y - lastfinger[Mouse_Wheel_LastIndexID].pos_y) / thumb_scale;
					if (abs(wy - py) > Exception_Offset || (wy > 0 && py < 0) || (wy < 0 && py > 0)) {//�ο�������λ����ָ��λ�Ʋ���������λ�Ʒ����෴
						px = 0;
						py = 0;
					}
				}
			}

			int direction_hscale = 1;//�����������ű���
			int direction_vscale = 1;//�����������ű���

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
	tp->evt_cbk(&mEvt, tp->evt_param);//��������¼�
	
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

