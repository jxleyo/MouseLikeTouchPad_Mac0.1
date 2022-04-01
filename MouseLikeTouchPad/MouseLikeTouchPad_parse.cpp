#include "MouseLikeTouchPad.h"
#include "apple_tp_proto.h"
#include<math.h>
extern "C" int _fltused = 0;


#define MAXFINGER_CNT 10

#define STABLE_INTERVAL_FingerSeparated_MSEC   20   // ��ָ�ֿ�������������ȶ�ʱ����
#define STABLE_INTERVAL_FingerClosed_MSEC      100   // ��ָ��£������������ȶ�ʱ���� 

#define ButtonPointer_Interval_MSEC      150   // ��������Ҽ���ָ��������ʱ��ms��

#define Jitter_Offset         10    // ������������΢������λ����ֵ


struct FingerPosition
{
	short  pos_x;
	short pos_y;
};

static struct SPI_TRACKPAD_PACKET* PtpReport;//���崥���屨������ָ��
static struct SPI_TRACKPAD_FINGER* pFinger_data;//���崥���屨����ָ����������ָ��

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

static LARGE_INTEGER MousePointer_DefineTime;//���ָ�붨��ʱ�䣬���ڼ��㰴�����ʱ���������ж�����м�͹��ֲ���
static float TouchPad_ReportInterval;//���崥���屨����ʱ��

static LARGE_INTEGER JitterFixStartTime; // ���������㶶������ʱ���ʱ��

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
static float scales[3];//��ָ��С����
static UINT8 sampcount;//��ָ��С��������


static __forceinline short abs(short x)
{
	if (x < 0)return -x;
	return x;
}

void MouseLikeTouchPad_parse_init(DEV_EXT* pDevContext)
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

		Mouse_Wheel_mode = FALSE;
		pDevContext->bMouse_Wheel_Mode_JudgeEnable = TRUE;//���������б�

		lastfinger_count = 0;
		currentfinger_count = 0;
		for (UINT8 i = 0; i < MAXFINGER_CNT; ++i) {
			lastfinger[i].X = 0;
			lastfinger[i].Y = 0;
			currentfinger[i].X = 0;
			currentfinger[i].Y = 0;
		}


		pDevContext->tick_Count = KeQueryTimeIncrement();
		pDevContext->runtimes = 0;

		Scroll_IntervalCount = 0;

		KeQueryTickCount(&last_ticktime);

		//��ȡ������ֱ���
		TouchPad_DPI = 100; //��ˮƽ100��/mmΪ��׼
		//��̬������ָͷ��С����
		thumb_width = 18;//��ָͷ���,Ĭ������ָ18mm��Ϊ��׼
		thumb_scale = 1.0;//��ָͷ�ߴ����ű�����
		FingerTracingMaxOffset = 6 * TouchPad_DPI * thumb_scale;//����׷�ٵ��β������ʱ�������ָ���λ��������
		FingerMinDistance = 12 * TouchPad_DPI * thumb_scale;//������Ч��������ָ��С����(��FingerTracingMaxOffset��ֱ�ӹ�ϵ)
		FingerClosedThresholdDistance = 16 * TouchPad_DPI * thumb_scale;//����������ָ��£ʱ����С����(��FingerTracingMaxOffset��ֱ�ӹ�ϵ)
		FingerMaxDistance = FingerMinDistance * 4;//������Ч��������ָ������(FingerMinDistance*4)  
		PointerSensitivity = TouchPad_DPI / 25;
		sampcount = 0;//��ָ��С��������

}

void MouseLikeTouchPad_parse(DEV_EXT* pDevContext, UINT8* data, LONG length)
{
	//NTSTATUS status = STATUS_SUCCESS;

	PtpReport = (struct SPI_TRACKPAD_PACKET*)data;
	if (length < TP_HEADER_SIZE || length < TP_HEADER_SIZE + TP_FINGER_SIZE * PtpReport->NumOfFingers || PtpReport->NumOfFingers >= MAXFINGER_CNT) return; //

	//���㱨��Ƶ�ʺ�ʱ����
	KeQueryTickCount(&current_ticktime);
	LONGLONG CounterDelta = (current_ticktime.QuadPart - last_ticktime.QuadPart) * pDevContext->tick_Count / 100;//��λ100US��
	ticktime_Interval.QuadPart = (current_ticktime.QuadPart - last_ticktime.QuadPart) * pDevContext->tick_Count / 10000;//��λms����
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
				pFinger_data = (struct SPI_TRACKPAD_FINGER*)(data + TP_HEADER_SIZE + i * TP_FINGER_SIZE);//
				currentfinger[i]= *pFinger_data;
				//currentfinger[i].X = pFinger_data->X;
				//currentfinger[i].Y = pFinger_data->Y;
			}
		}
	}


	//���ͱ��������
	BOOLEAN bPtpReportCollection = FALSE;//Ĭ����꼯��

	//��ʼ��ptp�¼�
	PTP_REPORT ptp_Report;
	RtlZeroMemory(&ptp_Report, sizeof(PTP_REPORT));

	//��ʼ������¼�
	mouse_report_t mReport;
	mReport.report_id = FAKE_REPORTID_MOUSE;
	mReport.button = 0;
	mReport.dx = 0;
	mReport.dy = 0;
	mReport.h_wheel = 0;
	mReport.v_wheel = 0;
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
			short dx = currentfinger[i].X - lastfinger[Mouse_Pointer_LastIndexID].X;
			short dy = currentfinger[i].Y - lastfinger[Mouse_Pointer_LastIndexID].Y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_Pointer_CurrentIndexID = i;//�ҵ�ָ��
				continue;//������������
			}
		}
		if (Mouse_Wheel_LastIndexID != -1) {
			short dx = currentfinger[i].X - lastfinger[Mouse_Wheel_LastIndexID].X;
			short dy = currentfinger[i].Y - lastfinger[Mouse_Wheel_LastIndexID].Y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_Wheel_CurrentIndexID = i;//�ҵ����ָ�����
				continue;//������������
			}
		}
		if (Mouse_LButton_LastIndexID != -1) {
			short dx = currentfinger[i].X - lastfinger[Mouse_LButton_LastIndexID].X;
			short dy = currentfinger[i].Y - lastfinger[Mouse_LButton_LastIndexID].Y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_LButton_Status = 1; //�ҵ������
				Mouse_LButton_CurrentIndexID = i;//��ֵ�����������������
				continue;//������������
			}
		}

		if (Mouse_RButton_LastIndexID != -1) {
			short dx = currentfinger[i].X - lastfinger[Mouse_RButton_LastIndexID].X;
			short dy = currentfinger[i].Y - lastfinger[Mouse_RButton_LastIndexID].Y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_RButton_Status = 1; //�ҵ��Ҽ���
				Mouse_RButton_CurrentIndexID = i;//��ֵ�Ҽ���������������
				continue;//������������
			}
		}
		if (Mouse_MButton_LastIndexID != -1) {
			short dx = currentfinger[i].X - lastfinger[Mouse_MButton_LastIndexID].X;
			short dy = currentfinger[i].Y - lastfinger[Mouse_MButton_LastIndexID].Y;

			if ((abs(dx) < FingerTracingMaxOffset) && (abs(dy) < FingerTracingMaxOffset)) {
				Mouse_MButton_Status = 1; //�ҵ��м���
				Mouse_MButton_CurrentIndexID = i;//��ֵ�м���������������
				continue;//������������
			}
		}
		
	}

	//��ʼ����¼��߼��ж�
	//ע�����ָ��ͬʱ���ٽӴ�������ʱ�����屨����ܴ���һ֡��ͬʱ��������������������Բ����õ�ǰֻ��һ����������Ϊ����ָ����ж�����
	if (Mouse_Pointer_LastIndexID == -1 && currentfinger_count > 0) {//���ָ�롢������Ҽ����м���δ����,
		//ָ�봥����ѹ�����Ӵ��泤�����ֵ���������ж����ƴ����󴥺���������,ѹ��ԽС�Ӵ��泤�����ֵԽ�󡢳�����ֵԽС
		BOOLEAN FakePointer = TRUE;
		short press = currentfinger[0].Pressure;
		float len = currentfinger[0].ToolMajor/ thumb_scale;
		double ratio;
		if (currentfinger[0].ToolMinor != 0) {
			ratio = (currentfinger[0].ToolMajor / currentfinger[0].ToolMinor) / thumb_scale;
		}
		else {
			ratio = 2;
		}
		if (press < 4) {
			if (ratio < 1.7 && len < 800) {
				FakePointer = FALSE;
			}
		}
		else if (press < 8) {
			if (ratio < 1.6 && len < 900) {
				FakePointer = FALSE;
			}
		}
		else if (press >= 8 && press <12) {
			if (ratio < 1.5 && len < 1000) {
				FakePointer = FALSE;
			}
		}
		else if (press >= 12 && press < 16) {
			if (ratio < 1.45 && len < 1200) {
				FakePointer = FALSE;
			}
		}
		else if (press >= 16) {
			if (ratio < 1.4 && len < 1350) {
				FakePointer = FALSE;
			}
		}
		
		if (!FakePointer) {
			Mouse_Pointer_CurrentIndexID = 0;  //�׸���������Ϊָ��
			MousePointer_DefineTime = current_ticktime;//���嵱ǰָ����ʼʱ��
		}
	}
	else if (Mouse_Pointer_CurrentIndexID == -1 && Mouse_Pointer_LastIndexID != -1) {//ָ����ʧ
		Mouse_Wheel_mode = FALSE;//��������ģʽ
		pDevContext->bMouse_Wheel_Mode_JudgeEnable = TRUE;//���������б�

		Mouse_Pointer_CurrentIndexID = -1;
		Mouse_LButton_CurrentIndexID = -1;
		Mouse_RButton_CurrentIndexID = -1;
		Mouse_MButton_CurrentIndexID = -1;
		Mouse_Wheel_CurrentIndexID = -1;
	}
	else if (Mouse_Pointer_CurrentIndexID != -1 && !Mouse_Wheel_mode) {  //ָ���Ѷ���ķǹ����¼�����
		//����ָ���������Ҳ��Ƿ��в�£����ָ��Ϊ����ģʽ���߰���ģʽ����ָ�����/�Ҳ����ָ����ʱ����ָ����ָ����ʱ����С���趨��ֵʱ�ж�Ϊ�����ַ���Ϊ��갴������һ��������Ч���𰴼�����ֲ���,����갴���͹��ֲ���һ��ʹ��
		//���������������������������м����ܻ���ʳָ�����л���Ҫ̧��ʳָ����иı䣬���/�м�/�Ҽ����µ�����²���ת��Ϊ����ģʽ��
		LARGE_INTEGER MouseButton_Interval;
		MouseButton_Interval.QuadPart = (current_ticktime.QuadPart - MousePointer_DefineTime.QuadPart) * pDevContext->tick_Count / 10000;//��λms����
		float Mouse_Button_Interval = (float)MouseButton_Interval.LowPart;//ָ�����Ҳ����ָ����ʱ����ָ�붨����ʼʱ��ļ��ms

		if (currentfinger_count > 1) {//��������������1����Ҫ�жϰ�������
			for (char i = 0; i < currentfinger_count; i++) {
				if (i == Mouse_Pointer_CurrentIndexID || i == Mouse_LButton_CurrentIndexID || i == Mouse_RButton_CurrentIndexID || i == Mouse_MButton_CurrentIndexID || i == Mouse_Wheel_CurrentIndexID) {//iΪ��ֵ�����������������Ƿ�Ϊ-1
					continue;  // �Ѿ����������
				}
				float dx = (float)(currentfinger[i].X - currentfinger[Mouse_Pointer_CurrentIndexID].X);
				float dy = (float)(currentfinger[i].Y - currentfinger[Mouse_Pointer_CurrentIndexID].Y);
				float distance = sqrt(dx * dx + dy * dy);//��������ָ��ľ���

				// ָ�����Ҳ�����ָ���²�����ָ����ָ��ʼ����ʱ����С����ֵ��ָ�뱻��������ֹ��ֲ���ֻ���ж�һ��ֱ��ָ����ʧ���������������жϲ��ᱻʱ����ֵԼ��ʹ����Ӧ�ٶȲ���Ӱ��
				BOOLEAN isWheel = pDevContext->bMouse_Wheel_Mode_JudgeEnable && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && Mouse_Button_Interval < ButtonPointer_Interval_MSEC;
				if (isWheel) {//����ģʽ��������
					Mouse_Wheel_mode = TRUE;  //��������ģʽ
					pDevContext->bMouse_Wheel_Mode_JudgeEnable = FALSE;//�رչ����б�

					Mouse_Wheel_CurrentIndexID = i;//���ָ����ο���ָ����ֵ

					Mouse_LButton_CurrentIndexID = -1;
					Mouse_RButton_CurrentIndexID = -1;
					Mouse_MButton_CurrentIndexID = -1;
					break;
				}
				else {//ǰ�����ģʽ�����ж��Ѿ��ų������Բ���Ҫ������ָ����ָ��ʼ����ʱ������
					if (Mouse_MButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerClosedThresholdDistance && dx < 0) {//ָ������в�£����ָ����
						Mouse_MButton_Status = 1; //�ҵ��м�
						Mouse_MButton_CurrentIndexID = i;//��ֵ�м���������������
						continue;  //����������������ʳָ�Ѿ����м�ռ������ԭ��������Ѿ�������
					}
					else if (Mouse_LButton_CurrentIndexID == -1 && abs(distance) > FingerClosedThresholdDistance && abs(distance) < FingerMaxDistance && dx < 0) {//ָ������зֿ�����ָ����
						Mouse_LButton_Status = 1; //�ҵ����
						Mouse_LButton_CurrentIndexID = i;//��ֵ�����������������
						continue;  //��������������
					}
					else if (Mouse_RButton_CurrentIndexID == -1 && abs(distance) > FingerMinDistance && abs(distance) < FingerMaxDistance && dx > 0) {//ָ���Ҳ�����ָ����
						Mouse_RButton_Status = 1; //�ҵ��Ҽ�
						Mouse_RButton_CurrentIndexID = i;//��ֵ�Ҽ���������������
						continue;  //��������������
					}
				}
			}
		}

		//���ָ��λ������
		if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݿ��ܲ��ȶ�ָ������ͻ����Ư����Ҫ����
			JitterFixStartTime = current_ticktime;//����������ʼ��ʱ
		}
		else {
			LARGE_INTEGER FixTimer;
			FixTimer.QuadPart = (current_ticktime.QuadPart - JitterFixStartTime.QuadPart) * pDevContext->tick_Count / 10000;//��λms����
			float JitterFixTimer = (float)FixTimer.LowPart;//��ǰ����ʱ���ʱ

			float STABLE_INTERVAL;
			if (Mouse_MButton_CurrentIndexID != -1) {//�м�״̬����ָ��£�Ķ�������ֵ������
				STABLE_INTERVAL = STABLE_INTERVAL_FingerClosed_MSEC;
			}
			else {
				STABLE_INTERVAL = STABLE_INTERVAL_FingerSeparated_MSEC;
			}
			if (JitterFixTimer > STABLE_INTERVAL) {//�������ȶ�ʱ���
				float px = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].X - lastfinger[Mouse_Pointer_LastIndexID].X) / thumb_scale;
				float py = (float)(currentfinger[Mouse_Pointer_CurrentIndexID].Y - lastfinger[Mouse_Pointer_LastIndexID].Y) / thumb_scale;

				if (Mouse_LButton_CurrentIndexID != -1 || Mouse_RButton_CurrentIndexID != -1 || Mouse_MButton_CurrentIndexID != -1) {//�а���ʱ��������ָ��ʱ����Ҫʹ��ָ�����ȷ
					if (abs(px) <= Jitter_Offset) {//ָ����΢��������
						px = 0;
					}
					if (abs(py) <= Jitter_Offset) {//ָ����΢��������
						py = 0;
					}
				}


				mReport.dx = (char)(px / PointerSensitivity);
				mReport.dy = -(char)(py / PointerSensitivity);
				if (abs(px) <= Jitter_Offset/2) {//ָ�����پ�ϸ�ƶ�ʱ����
					mReport.dx = (char)(px * 2 / PointerSensitivity);
				}
				if (abs(py) <= Jitter_Offset/2) {//ָ�����پ�ϸ�ƶ�ʱ����
					mReport.dy = -(char)(py * 2 / PointerSensitivity);
				}

				//thumb_scale��ָ��С���������䲻ͬ��ʹ����,3�β���ȡƽ��ֵ,�����������Сֵ
				if (sampcount < 3) {
					if (currentfinger[Mouse_Pointer_CurrentIndexID].Pressure > 20 && currentfinger[Mouse_Pointer_CurrentIndexID].Pressure < 25) {
						if (currentfinger[Mouse_Pointer_CurrentIndexID].ToolMinor > 600 || currentfinger[Mouse_Pointer_CurrentIndexID].ToolMinor < 850) {
							scales[sampcount] = (float)currentfinger[Mouse_Pointer_CurrentIndexID].ToolMinor / 750;
							sampcount++;
						}
					}
					else if (currentfinger[Mouse_Pointer_CurrentIndexID].Pressure > 25 && currentfinger[Mouse_Pointer_CurrentIndexID].Pressure < 75) {
						if (currentfinger[Mouse_Pointer_CurrentIndexID].ToolMinor > 600 || currentfinger[Mouse_Pointer_CurrentIndexID].ToolMinor < 850) {
							scales[sampcount] = (float)currentfinger[Mouse_Pointer_CurrentIndexID].ToolMinor / 800;
							sampcount++;
						}
					}
				}
				else if (sampcount == 3) {
					thumb_scale = (scales[0] + scales[1] + scales[2]) / 3;
					sampcount = 0;
				}
				
			}
		}
	}
	else if (Mouse_Pointer_CurrentIndexID != -1 && Mouse_Wheel_mode) {//���ֲ���ģʽ
		RegDebug(L"Mouse_Wheel_mode on", NULL, 0x12345678);
		////���ָ��λ������
		//if (currentfinger_count != lastfinger_count) {//��ָ�仯˲��ʱ���ݿ��ܲ��ȶ�ָ������ͻ����Ư����Ҫ����
		//	JitterFixStartTime = current_ticktime;//����������ʼ��ʱ
		//}
		//else {
			bPtpReportCollection = TRUE;//ȷ�ϴ��ذ屨��ģʽ

			ptp_Report.ReportID = FAKE_REPORTID_MULTITOUCH;//����ʵ��ֵ

			ptp_Report.ContactCount = PtpReport->NumOfFingers;
			RegDebug(L"ptp_Report.ContactCount=", NULL, ptp_Report.ContactCount);
			UINT8 AdjustedCount = (PtpReport->NumOfFingers > 5) ? 5 : PtpReport->NumOfFingers;
			for (int i = 0; i < AdjustedCount; i++) {
				ptp_Report.Contacts[i].Confidence = 1;
				ptp_Report.Contacts[i].TipSwitch = (currentfinger[i].Pressure > 0) ? 1 : 0; //1;
				ptp_Report.Contacts[i].ContactID = i;// ContactID;
				float LOGICAL_MAXIMUM_scale = 9;//macbook��win�ʼǱ����ذ���߼������������ֵת������
				LONG x = currentfinger[i].X - pDevContext->TrackpadInfo.XMin;
				if (x < 0) { x = 0; }

				LONG y = pDevContext->TrackpadInfo.YMax - currentfinger[i].Y;
				if (y < 0) { y = 0; }

				ptp_Report.Contacts[i].X = (USHORT)(x / LOGICAL_MAXIMUM_scale);
				ptp_Report.Contacts[i].Y = (USHORT)(y / LOGICAL_MAXIMUM_scale);

				if (i == 0) {
					RegDebug(L"ptp_Report.Contacts[0].Confidence=", NULL, ptp_Report.Contacts[i].Confidence);
					RegDebug(L"ptp_Report.Contacts[0].TipSwitch=", NULL, ptp_Report.Contacts[i].TipSwitch);
					RegDebug(L"ptp_Report.Contacts[0].X=", NULL, ptp_Report.Contacts[i].X);
					RegDebug(L"ptp_Report.Contacts[0].Y=", NULL, ptp_Report.Contacts[i].Y);
				}
				if (i == 1) {
					RegDebug(L"ptp_Report.Contacts[1].Confidence=", NULL, ptp_Report.Contacts[i].Confidence);
					RegDebug(L"ptp_Report.Contacts[1].TipSwitch=", NULL, ptp_Report.Contacts[i].TipSwitch);
					RegDebug(L"ptp_Report.Contacts[1].X=", NULL, ptp_Report.Contacts[i].X);
					RegDebug(L"ptp_Report.Contacts[1].Y=", NULL, ptp_Report.Contacts[i].Y);
				}
				if (i == 2) {
					RegDebug(L"ptp_Report.Contacts[2].Confidence=", NULL, ptp_Report.Contacts[i].Confidence);
					RegDebug(L"ptp_Report.Contacts[2].TipSwitch=", NULL, ptp_Report.Contacts[i].TipSwitch);
					RegDebug(L"ptp_Report.Contacts[2].X=", NULL, ptp_Report.Contacts[i].X);
					RegDebug(L"ptp_Report.Contacts[2].Y=", NULL, ptp_Report.Contacts[i].Y);
				}
			}

			if (CounterDelta < 0xF)
			{
				ptp_Report.ScanTime = 0xF;
			}
			else if (CounterDelta >= 0xFF)
			{
				ptp_Report.ScanTime = 0xFF;
			}
			else {
				ptp_Report.ScanTime = (USHORT)CounterDelta;
			}
			RegDebug(L"ptp_Report.ScanTime=", NULL, ptp_Report.ScanTime);
		/*}*/
	}
	else {
		//���������Ч
	}
	

	ptp_event_t pEvt;
	RtlZeroMemory(&pEvt, sizeof(ptp_event_t));
	if (!bPtpReportCollection) {//����MouseCollection
		mReport.button = Mouse_LButton_Status + (Mouse_RButton_Status << 1) + (Mouse_MButton_Status << 2);  //�����Ҽ�״̬�ϳ�

		pEvt.collectionType = MOUSE_CollectionType;//
		pEvt.mReport = mReport;
		//��������¼�
		mltp_Event(pDevContext, &pEvt);
	}
	else {
		pEvt.collectionType = PTP_CollectionType;//
		pEvt.ptpReport = ptp_Report;
		RegDebug(L"pEvt.ptpReport=", &pEvt.ptpReport, sizeof(PTP_REPORT));
		//���ʹ��ذ��¼�
		mltp_Event(pDevContext, &pEvt);
	}
	
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


