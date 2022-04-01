#pragma once

#include <ntddk.h>

#include <wdf.h>

#include <hidport.h>

#include "apple_tp_proto.h"
#include "hid_mouse_desc.h"
#include <ntstrsafe.h>

#define DPT DbgPrint

#define FUNCTION_FROM_CTL_CODE(ctrlCode) (((ULONG)((ctrlCode) & 0x3FFC)) >> 2)

////
#define VHID_HARDWARE_IDS    L"HID\\MouseLikeTouchPad\0\0"
#define VHID_HARDWARE_IDS_LENGTH sizeof (VHID_HARDWARE_IDS)
#define PoolTag               'mltp'

typedef unsigned long       DWORD;
typedef unsigned short      WORD;

#define LOWORD(l)   ((WORD) (l))


////
struct DEV_EXT
{
	WDFDEVICE     hDevice;
	WDFIOTARGET   IoTarget; ///����������next device

		/////
	WDFLOOKASIDE  LookasideHandle;
	WDFCOLLECTION ReportCollection;

	////
	WDFQUEUE      ReportQueue; //�ϲ��Request���󣬵ײ㻹û�������ݣ���ʱ�����������
	///
	BOOLEAN       bRequestStop;   //ָʾ����ֹͣ
	KEVENT        RequestEvent;   //����ȴ�ֹͣ�¼�
	KEVENT        RequestEvent2;   //����ȴ�ֹͣ�¼�
	WDFREQUEST    ReuseRequest;   //�ظ�ʹ�õ� Request
	WDFMEMORY     RequestBuffer;  // �����Buffer

	WDFTIMER      timerHandle;  //���ڿ�������

	ULONG         ActiveCount;

	ULONG         tick_Count;
	ULONG         runtimes;
	ULONG         runtimes_ReadReport;

	// SPI device metadata
	USHORT HidVendorID;
	USHORT HidProductID;
	USHORT HidVersionNumber;
	SPI_TRACKPAD_INFO TrackpadInfo;

	BOOLEAN       PtpInputModeOn;
	BOOLEAN       bMouse_Wheel_Mode_JudgeEnable;//�����Ƿ��������б�
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEV_EXT, GetDeviceContext)

/////
////// function

NTSTATUS EvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit);

VOID EvtInternalDeviceControl(
	IN WDFQUEUE     Queue,
	IN WDFREQUEST   Request,
	IN size_t       OutputBufferLength,
	IN size_t       InputBufferLength,
	IN ULONG        IoControlCode);

NTSTATUS EvtPnpQueryIds(WDFDEVICE device, PIRP Irp);

NTSTATUS create_reuse_request(DEV_EXT* ext);

NTSTATUS
EvtDevicePrepareHardware(
	_In_ WDFDEVICE Device,
	_In_ WDFCMRESLIST ResourceList,
	_In_ WDFCMRESLIST ResourceListTranslated
);

NTSTATUS EvtDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState);
NTSTATUS EvtDeviceD0Exit(WDFDEVICE Device, WDF_POWER_DEVICE_STATE TargetState);

NTSTATUS StartReadReport(DEV_EXT* ext);
void StopReadReport(DEV_EXT* ext);

NTSTATUS SetSpecialFeature(DEV_EXT* ext);


void MouseLikeTouchPad_parse(DEV_EXT* pDevContext, UINT8* data, LONG length);

void MouseLikeTouchPad_parse_init(DEV_EXT* pDevContext);

VOID RegDebug(WCHAR* strValueName, PVOID dataValue, ULONG datasizeValue);

void mltp_Event(DEV_EXT* ext, ptp_event_t* evt);

NTSTATUS
HidReadReport(
	DEV_EXT* pDevContext,
	WDFREQUEST Request,
	BOOLEAN* requestPendingFlag
);

NTSTATUS
PtpReportFeatures(
	_In_ WDFDEVICE Device,
	_In_ WDFREQUEST Request
);
