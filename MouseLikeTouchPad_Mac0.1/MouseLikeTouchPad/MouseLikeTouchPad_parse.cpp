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


//����׷�ٵ���ָ
static struct FingerPosition lastfinger[MAXFINGER_CNT];
static struct FingerPosition currentfinger[MAXFINGER_CNT];
static int currentfinger_count=0;
static int lastfinger_count=0;

static int Mouse_Pointer_lastIndexID; //������ʱ׷�����ָ�봥������������������ţ�-1Ϊδ����
static int Mouse_Pointer_currentIndexID; //������ʱ���ָ�봥������������������ţ�-1Ϊδ����

static BOOLEAN Mouse_LButton_Status; //����������״̬��0Ϊ�ͷţ�1Ϊ����
static BOOLEAN Mouse_MButton_Status; //��������м�״̬��0Ϊ�ͷţ�1Ϊ����
static BOOLEAN Mouse_RButton_Status; //��������Ҽ�״̬��0Ϊ�ͷţ�1Ϊ����

static BOOLEAN Mouse_Wheel_mode; //����������״̬��0Ϊ����δ���1Ϊ���ּ���
static BOOLEAN TouchRoundStart; //�����غϿ�ʼ��־
static BOOLEAN Mouse_MButton_Enabled; //����м����ܿ�����־

static LARGE_INTEGER MousePointer_DefineTime;//���ָ�붨��ʱ�䣬���ڼ��㰴�����ʱ���������ж�����м�͹��ֲ���
static float TouchPad_ReportInterval;//���崥���屨����ʱ��
static float Jitter_Interval;//���嶶���������ʱ��

static int Scroll_IntervalCount; //�����������������
static short evt_idle_repeat_count; //�����ؼ���

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
		Mouse_Pointer_lastIndexID = -1; //�������ָ�봥������������������ţ�-1Ϊδ����
		Mouse_Pointer_currentIndexID = -1; //�������ָ�봥������������������ţ�-1Ϊδ����
		Mouse_LButton_Status = 0; //����������״̬��0Ϊ�ͷţ�1Ϊ����
		Mouse_MButton_Status = 0;//��������м�״̬��0Ϊ�ͷţ�1Ϊ����
		Mouse_RButton_Status = 0; //��������Ҽ�״̬��0Ϊ�ͷţ�1Ϊ����

		Mouse_Wheel_mode = 0;

		lastfinger_count = 0;
		currentfinger_count = 0;
		for (u8 i = 0; i < MAXFINGER_CNT; ++i) {
			lastfinger[i].pos_x = 0;
			lastfinger[i].pos_y = 0;
			currentfinger[i].pos_x = 0;
			currentfinger[i].pos_y = 0;
		}

		Jitter_Interval = 0;
		Scroll_IntervalCount = 0;
		evt_idle_repeat_count = 0;
		TouchRoundStart = TRUE;	
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
		PointerSensitivity = TouchPad_DPI / 40;
}

void MouseLikeTouchPad_parse(u8* data, LONG length)
{
	mouse_event_t evt;
	struct tp_protocol* pr = (struct tp_protocol*)data;

	if (length < TP_HEADER_SIZE || length < TP_HEADER_SIZE + TP_FINGER_SIZE * pr->finger_number || pr->finger_number >= MAXFINGER_CNT) return; //

	//���㱨��Ƶ�ʺ�ʱ����
	KeQueryTickCount(&current_ticktime);
	ticktime_Interval.QuadPart = (current_ticktime.QuadPart - last_ticktime.QuadPart) * tp->tick_count / 10000;//��λms����
	TouchPad_ReportInterval = (float)ticktime_Interval.LowPart;//�����屨����ʱ��ms
	last_ticktime = current_ticktime;

	//���浱ǰ��ָ����
	if (!pr->is_finger) {//is_finger�����ж���ָȫ���뿪��������pr->state & 0x80)��pr->state �ж���Ϊ��㴥��ʱ����һ����ָ�뿪����������źţ�Ҳ������pr->finger_number�ж���Ϊ��ֵ����Ϊ0
		TouchRoundStart = TRUE;//��һ�ִ��������غϿ�ʼ

		currentfinger_count = 0;
		/*for (int i = 0; i < MAXFINGER_CNT; ++i) {
			currentfinger[i].pos_x = 0;
			currentfinger[i].pos_y = 0;
		}*/
	}
	else {
		currentfinger_count = pr->finger_number;
		if (currentfinger_count > 0) {
			for (int i = 0; i < currentfinger_count; i++) {
				struct tp_finger* g = (struct tp_finger*)(data + TP_HEADER_SIZE + i * TP_FINGER_SIZE); //
				currentfinger[i].pos_x = g->x;
				currentfinger[i].pos_y = g->y;
			}
		}
	}

	//��ʼ������¼�
	evt.button = 0;
	evt.dx = 0;
	evt.dy = 0;
	evt.h_wheel = 0;
	evt.v_wheel = 0;
	Mouse_LButton_Status = 0; //����������״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�
	Mouse_MButton_Status = 0; //��������м�״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�
	Mouse_RButton_Status = 0; //��������Ҽ�״̬��0Ϊ�ͷţ�1Ϊ���£�ÿ�ζ���Ҫ����ȷ�������߼�

	if (currentfinger_count >3 && pr->clicked) {
		Mouse_MButton_Enabled = !Mouse_MButton_Enabled;//3ָ���ϴ�������ѹʱ������ʱ�л�����/�ر�����м�����
	}

	if (Mouse_Pointer_lastIndexID==-1 && TouchRoundStart) {//���ָ�롢������Ҽ����м���δ����
		if (currentfinger_count > 0) {
			Mouse_Pointer_currentIndexID = 0;  //�׸���������Ϊָ��
			MousePointer_DefineTime = current_ticktime;//���嵱ǰָ����ʼʱ��
		}	
	}
	else if (Mouse_Pointer_lastIndexID != -1 && !Mouse_Wheel_mode && TouchRoundStart) {//ָ���Ѷ���ķǹ����¼�����
		//����ָ��
		BOOLEAN foundi = 0;
		for (int i = 0; i < currentfinger_count; i++) {
			float dx = (float)(currentfinger[i].pos_x - lastfinger[Mouse_Pointer_lastIndexID].pos_x);
			float dy = (float)(currentfinger[i].pos_y - lastfinger[Mouse_Pointer_lastIndexID].pos_y);
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

					evt.dx = (short)(rx / PointerSensitivity);
					evt.dy = -(short)(ry / PointerSensitivity);
						
				}
				
				Mouse_Pointer_currentIndexID = i; //���Ϊָ�븳ֵ�µ�����
				foundi = 1;
				break;
			}
		}
		if (!foundi) {//ָ����ʧ��ȫ��������Ч
			TouchRoundStart = FALSE;

			Mouse_Pointer_currentIndexID = -1;//ָ����ʧ
		}
		else {//ָ����ڵ�����²�ѯ�Ƿ�������Ҽ��������߹��ֲ���
			for (int i = 0; i < currentfinger_count; i++) {
				if (i == Mouse_Pointer_currentIndexID) {
					continue;
				}
				float dx = (float)(currentfinger[i].pos_x - currentfinger[Mouse_Pointer_lastIndexID].pos_x);
				float dy = (float)(currentfinger[i].pos_y - currentfinger[Mouse_Pointer_lastIndexID].pos_y);
				float distance = sqrt(dx * dx + dy * dy);

				//����ָ���������Ҳ��Ƿ��в�£����ָ��Ϊ����ģʽ���߰���ģʽ����ָ�����/�Ҳ����ָ����ʱ����ָ����ָ����ʱ����С���趨��ֵʱ�ж�Ϊ�����ַ���Ϊ��갴������һ��������Ч���𰴼�����ֲ���,����갴���͹��ֲ���һ��ʹ��
				LARGE_INTEGER MouseButton_Interval;
				MouseButton_Interval.QuadPart = (current_ticktime.QuadPart - MousePointer_DefineTime.QuadPart) * tp->tick_count / 10000;//��λms����
				float Mouse_Button_Interval = (float)MouseButton_Interval.LowPart;//ָ�����Ҳ����ָ����ʱ����ָ�붨����ʼʱ��ļ��ms

				if (!Mouse_MButton_Enabled) {//����м����ܹر�ʱ
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval < MButton_Interval_MSEC) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ
						Mouse_Wheel_mode = TRUE;  //��������ģʽ
						break;
					}
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״��
						Mouse_LButton_Status = 1; //�ҵ����
						continue;  //�������Ҽ�
					}
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
						Mouse_RButton_Status = 1; //�ҵ��Ҽ�
						continue;  //���������
					}
				}
				else {//����м����ܿ���ʱ
					if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval < MButton_Interval_MSEC) {//ָ�����Ҳ��в�£����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ
						Mouse_Wheel_mode = TRUE;  //��������ģʽ
						break;
					}
					else if (abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && Mouse_Button_Interval > MButton_Interval_MSEC && dx < 0) {//ָ������в�£����ָ���²�����ָ����ָ��ʼ����ʱ����������ֵ
						Mouse_MButton_Status = 1; //�ҵ��м�
						continue;  //�������Ҽ���ʳָ�Ѿ����м�ռ������ԭ��������Ѿ�������
					}
					else if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx < (FingerMinDistance / 2)) {//ָ���������ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ���������Ĳ�����������п���x���Դ���ָ��һ����״����
						Mouse_LButton_Status = 1; //�ҵ����
						continue;  //�������Ҽ�
					}
					else if (abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ���£�ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ����
						Mouse_RButton_Status = 1; //�ҵ��Ҽ�
						continue;  //���������
					}
				}
					
			}
		}
	}
	else if (Mouse_Pointer_lastIndexID != -1 && Mouse_Wheel_mode && TouchRoundStart) {//���ֲ���ģʽ
		// ����ָ��
		BOOLEAN foundi = 0;
		for (int i = 0; i < currentfinger_count; i++) {
			float dx = (float)(currentfinger[i].pos_x - lastfinger[Mouse_Pointer_lastIndexID].pos_x);
			float dy = (float)(currentfinger[i].pos_y - lastfinger[Mouse_Pointer_lastIndexID].pos_y);
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

					if (abs(rx) > 4 && abs(rx) < 32) {
						if (Scroll_IntervalCount == 0) {//�ȹ���һ���ټ������
							evt.h_wheel = (char)(rx > 0 ? 1 : -1);
							Scroll_IntervalCount = 16;
						}
						else {
							Scroll_IntervalCount--;
						}
					}
					else if (abs(rx) >= 32) {
						evt.h_wheel = (char)(rx > 0 ? 1 : -1);
						Scroll_IntervalCount = 0;
					}
					if (abs(ry) > 4 && abs(ry) < 32) {
						if (Scroll_IntervalCount == 0) {
							evt.v_wheel = (char)(ry > 0 ? 1 : -1);
							Scroll_IntervalCount = 16;
						}
						else {
							Scroll_IntervalCount--;
						}
					}
					else if (abs(ry) >= 32) {
						evt.v_wheel = (char)(ry > 0 ? 1 : -1);
						Scroll_IntervalCount = 0;
					}
				}
				
				Mouse_Pointer_currentIndexID = i; //���Ϊָ�븳ֵ�µ�����
				foundi = 1;//����ģʽ����ֱ��ָ����ʧ
				break;
			}
		}
		if (!foundi) {//ָ����ʧ��ȫ��������Ч
			TouchRoundStart = FALSE;

			Mouse_Wheel_mode = FALSE;
			Mouse_Pointer_currentIndexID = -1;//ָ����ʧ

			Scroll_IntervalCount = 0;
		} 
		
	}
	else {
		//���������Ч
	}

	evt.button = Mouse_LButton_Status + (Mouse_RButton_Status << 1) + (Mouse_MButton_Status << 2);  //�����Ҽ�״̬�ϳ�
	tp->evt_cbk(&evt, tp->evt_param);//��������¼�
	
	//������һ�ֳ�ʼ����
	for (int i = 0; i < currentfinger_count; i++) {
		lastfinger[i] = currentfinger[i];
	}
	lastfinger_count = currentfinger_count;
	Mouse_Pointer_lastIndexID = Mouse_Pointer_currentIndexID;

}

