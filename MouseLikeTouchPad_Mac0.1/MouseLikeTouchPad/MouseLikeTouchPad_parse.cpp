#include "MouseLikeTouchPad.h"
#include "apple_tp_proto.h"
#include<math.h>
extern "C" int _fltused = 0;


#define MAXFINGER_CNT 10

struct MouseLikeTouchPad_state
{
	MOUSEEVENTCALLBACK evt_cbk;
	void* evt_param;
	ULONG tick_count;
};


struct FingerPosition
{
	short              pos_x;
	short              pos_y;
};


static struct SPI_TRACKPAD_PACKET* PtpReport;//���崥���屨������ָ��
static struct SPI_TRACKPAD_FINGER* pFinger_data;//���崥���屨����ָ����������ָ��

//����׷�ٵ���ָ
static struct FingerPosition lastfinger[MAXFINGER_CNT];
static struct FingerPosition currentfinger[MAXFINGER_CNT];
static UINT8 currentfinger_count;
static UINT8 lastfinger_count;

static INT8 Mouse_Pointer_LastIndexID; //������ʱ׷�����ָ�봥������������������ţ�-1Ϊδ����
static INT8 Mouse_Pointer_CurrentIndexID; //������ʱ���ָ�봥������������������ţ�-1Ϊδ����

static BOOLEAN Mouse_LButton_Status; //����������״̬��0Ϊ�ͷţ�1Ϊ����
static BOOLEAN Mouse_MButton_Status; //��������м�״̬��0Ϊ�ͷţ�1Ϊ����
static BOOLEAN Mouse_RButton_Status; //��������Ҽ�״̬��0Ϊ�ͷţ�1Ϊ����

static BOOLEAN Mouse_Wheel_Mode; //����������״̬��0Ϊ����δ���1Ϊ���ּ���
static BOOLEAN Mouse_MButton_Enabled; //����м����ܿ�����־

static LARGE_INTEGER MousePointer_DefineTime;//���ָ�붨��ʱ�䣬���ڼ��㰴�����ʱ���������ж�����м�͹��ֲ���
static float TouchPad_ReportInterval;//���崥���屨����ʱ��
static float Jitter_Interval;//���嶶���������ʱ��

static int Scroll_IntervalCount; //�����������������

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
	    Mouse_Pointer_LastIndexID = -1; //�������ָ�봥������������������ţ�-1Ϊδ����
		Mouse_Pointer_CurrentIndexID = -1; //�������ָ�봥������������������ţ�-1Ϊδ����
		Mouse_LButton_Status = 0; //����������״̬��0Ϊ�ͷţ�1Ϊ����
		Mouse_MButton_Status = 0;//��������м�״̬��0Ϊ�ͷţ�1Ϊ����
		Mouse_RButton_Status = 0; //��������Ҽ�״̬��0Ϊ�ͷţ�1Ϊ����

		Mouse_Wheel_Mode = FALSE;

		lastfinger_count = 0;
		currentfinger_count = 0;

		for (UINT8 i = 0; i < MAXFINGER_CNT; ++i) {
			lastfinger[i].pos_x = 0;
			lastfinger[i].pos_y = 0;
			currentfinger[i].pos_x = 0;
			currentfinger[i].pos_y = 0;
		}

		Jitter_Interval = 0;
		Scroll_IntervalCount = 0;

		Mouse_MButton_Enabled = FALSE;//Ĭ�Ϲر��м�����

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

	//��ʼ����¼��߼��ж�
	if (currentfinger_count > 3 && PtpReport->ClickOccurred) {
		Mouse_MButton_Enabled = !Mouse_MButton_Enabled;//3ָ���ϴ�������ѹʱ������ʱ�л�����/�ر�����м�����
	}

	//ע�����ָ��ͬʱ���ٽӴ�������ʱ�����屨����ܴ���һ֡��ͬʱ��������������������Բ����õ�ǰֻ��һ����������Ϊ����ָ����ж�����������Ҳ������Mouse_Pointer_LastIndexID==-1��Ϊ��ָ�붨����������������ʣ����ָ���ں�����
	if (lastfinger_count == 0 && currentfinger_count > 0) {//���ָ�롢������Ҽ����м���δ����,
		Mouse_Pointer_CurrentIndexID = 0;  //�׸���������Ϊָ��
		MousePointer_DefineTime = current_ticktime;//���嵱ǰָ����ʼʱ��
	}
	else if (Mouse_Pointer_LastIndexID != -1 && !Mouse_Wheel_Mode) {//ָ���Ѷ���ķǹ����¼�����
		//����ָ��
		BOOLEAN foundi = FALSE;
		for (UINT8 i = 0; i < currentfinger_count; i++) {
			float dx = (float)(currentfinger[i].pos_x - lastfinger[Mouse_Pointer_LastIndexID].pos_x);
			float dy = (float)(currentfinger[i].pos_y - lastfinger[Mouse_Pointer_LastIndexID].pos_y);
			float distance = sqrt(dx*dx + dy*dy);

			if (abs(distance) < FingerTracingMaxOffset) {	
				if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݲ��ȶ�ָ������ͻ����Ư����Ҫ����
					Jitter_Interval = 0;//��ʼ����������ʱ
				}
				else {
					float rx = dx / thumb_scale;
					float ry = dy / thumb_scale;

					if (Jitter_Interval < Jitter_Time_MSEC) {//��ָ�仯����һ��ʱ����ָ��λ������������
						Jitter_Interval += TouchPad_ReportInterval;//�ۼƶ�������ʱ��
						if (abs(rx) < Jitter_Offset) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
							rx = 0;
						}
						if (abs(ry) < Jitter_Offset) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
							ry = 0;
						}
					}
					else {//����ʱ�����������
						Jitter_Interval = 0;//���ö���������ʱ	
					}

					mEvt.dx = (short)(rx / PointerSensitivity);
					mEvt.dy = -(short)(ry / PointerSensitivity);
						
				}
				
				Mouse_Pointer_CurrentIndexID = i; //���Ϊָ�븳ֵ�µ�����
				foundi = TRUE;
				break;
			}
		}
		if (!foundi) {//ָ����ʧ��ȫ��������Ч
			Mouse_Pointer_CurrentIndexID = -1;//ָ����ʧ
		}
		else {//ָ����ڵ�����²�ѯ�Ƿ�������Ҽ��������߹��ֲ���
			for (int i = 0; i < currentfinger_count; i++) {
				if (i == Mouse_Pointer_CurrentIndexID) {
					continue;
				}
				float dx = (float)(currentfinger[i].pos_x - currentfinger[Mouse_Pointer_LastIndexID].pos_x);
				float dy = (float)(currentfinger[i].pos_y - currentfinger[Mouse_Pointer_LastIndexID].pos_y);
				float distance = sqrt(dx * dx + dy * dy);

				//����ָ���������Ҳ��Ƿ��в�£����ָ��Ϊ����ģʽ���߰���ģʽ����ָ�����/�Ҳ����ָ����ʱ����ָ����ָ����ʱ����С���趨��ֵʱ�ж�Ϊ�����ַ���Ϊ��갴������һ��������Ч���𰴼�����ֲ���,����갴���͹��ֲ���һ��ʹ��
				LARGE_INTEGER MouseButton_Interval;
				MouseButton_Interval.QuadPart = (current_ticktime.QuadPart - MousePointer_DefineTime.QuadPart) * tp->tick_count / 10000;//��λms����
				float Mouse_Button_Interval = (float)MouseButton_Interval.LowPart;//ָ�����Ҳ����ָ����ʱ����ָ�붨����ʼʱ��ļ��ms

				if (!Mouse_MButton_Enabled) {//����м����ܹر�ʱ
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval < ButtonPointer_Interval_MSEC) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ
						Mouse_Wheel_Mode = TRUE;  //��������ģʽ
						break;
					}
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״��
						Mouse_LButton_Status = 1; //�ҵ����
						continue;  //������������
					}
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
						Mouse_RButton_Status = 1; //�ҵ��Ҽ�
						continue;  //������������
					}
				}
				else {//����м����ܿ���ʱ
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval < ButtonPointer_Interval_MSEC) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ
						Mouse_Wheel_Mode = TRUE;  //��������ģʽ
						break;
					}
					else if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval > ButtonPointer_Interval_MSEC && dx < 0) {//ָ������в�£����ָ���²�����ָ����ָ��ʼ����ʱ����������ֵ
						Mouse_MButton_Status = 1; //�ҵ��м�
						continue;  //�������Ҽ���ʳָ�Ѿ����м�ռ������ԭ��������Ѿ�������
					}
					else if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״����
						Mouse_LButton_Status = 1; //�ҵ����
						continue;  //������������
					}
					else if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
						Mouse_RButton_Status = 1; //�ҵ��Ҽ�
						continue;  //������������
					}
				}
					
			}
		}
	}
	else if (Mouse_Pointer_LastIndexID != -1 && Mouse_Wheel_Mode) {//���ֲ���ģʽ
		// ����ָ��
		BOOLEAN foundi = FALSE;
		for (UINT8 i = 0; i < currentfinger_count; i++) {
			float dx = (float)(currentfinger[i].pos_x - lastfinger[Mouse_Pointer_LastIndexID].pos_x);
			float dy = (float)(currentfinger[i].pos_y - lastfinger[Mouse_Pointer_LastIndexID].pos_y);
			float distance = sqrt(dx * dx + dy * dy);
			if (abs(distance) < FingerTracingMaxOffset) {
				if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݲ��ȶ�ָ������ͻ����Ư����Ҫ����
					Jitter_Interval = 0;//��ʼ����������ʱ
				}
				else {
					float rx = dx / thumb_scale;
					float ry = dy / thumb_scale;

					if (Jitter_Interval < Jitter_Time_MSEC) {//��ָ�仯����һ��ʱ����ָ��λ������������
						Jitter_Interval += TouchPad_ReportInterval;//�ۼƶ�������ʱ��
						if (abs(rx) < Jitter_Offset) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
							rx = 0;
						}
						if (abs(ry) < Jitter_Offset) {//ָ�붶������������ο�ָ���Աߵ���ָλ��������ָ��λ��
							ry = 0;
						}
					}
					else {//����ʱ�����������
						Jitter_Interval = 0;//���ö���������ʱ	
					}

					int direction_hscale = 1;//�����������ű���
					int direction_vscale = 1;//�����������ű���

					if (abs(rx) > abs(dy) / 4) {//���������ȶ�������
						direction_hscale = 1;
						direction_vscale = 8;
					}
					if (abs(ry) > abs(dx) / 4) {//���������ȶ�������
						direction_hscale = 8;
						direction_vscale = 1;
					}

					rx = rx / direction_hscale;
					ry = ry / direction_vscale;

					if (abs(rx) > 4 && abs(rx) < 64) {
						if (Scroll_IntervalCount == 0) {//�ȹ���һ���ټ������
							mEvt.h_wheel = (char)(rx > 0 ? 1 : -1);
							Scroll_IntervalCount = 16;
						}
						else {
							Scroll_IntervalCount--;
						}
					}
					else if (abs(rx) >= 64) {
						mEvt.h_wheel = (char)(rx > 0 ? 1 : -1);
						Scroll_IntervalCount = 0;
					}
					if (abs(ry) > 4 && abs(ry) < 64) {
						if (Scroll_IntervalCount == 0) {
							mEvt.v_wheel = (char)(ry > 0 ? 1 : -1);
							Scroll_IntervalCount = 16;
						}
						else {
							Scroll_IntervalCount--;
						}
					}
					else if (abs(ry) >= 64) {
						mEvt.v_wheel = (char)(ry > 0 ? 1 : -1);
						Scroll_IntervalCount = 0;
					}
				}
				
				Mouse_Pointer_CurrentIndexID = i; //���Ϊָ�븳ֵ�µ�����
				foundi = TRUE;//����ģʽ����ֱ��ָ����ʧ
				break;
			}
		}
		if (!foundi) {//ָ����ʧ��ȫ��������Ч
			Mouse_Wheel_Mode = FALSE;
			Mouse_Pointer_CurrentIndexID = -1;//ָ����ʧ

			Scroll_IntervalCount = 0;
		} 
		
	}
	else {
		//���������Ч
	}

	mEvt.button = Mouse_LButton_Status + (Mouse_RButton_Status << 1) + (Mouse_MButton_Status << 2);  //�����Ҽ�״̬�ϳ�
	tp->evt_cbk(&mEvt, tp->evt_param);//��������¼�
	
	//������һ�ֳ�ʼ����
	for (int i = 0; i < currentfinger_count; i++) {
		lastfinger[i] = currentfinger[i];
	}
	lastfinger_count = currentfinger_count;
	Mouse_Pointer_LastIndexID = Mouse_Pointer_CurrentIndexID;

}

