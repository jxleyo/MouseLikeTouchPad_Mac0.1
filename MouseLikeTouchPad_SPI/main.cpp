#include "MouseLikeTouchPad_SPI.h"


////
static VOID EvtDriverContextCleanup(IN WDFOBJECT Object)
{
	DPT("EvtDriverContextCleanup\n");
	////

	UNREFERENCED_PARAMETER(Object);

}

///����Ҫ�����ˣ�ÿ�����󶼻����
//static VOID EvtIoStop(WDFQUEUE Queue, WDFREQUEST Request, ULONG ActionFlags)
//{
//	DPT("--- EvtIoStop \n");
//	///
//	UNREFERENCED_PARAMETER(Queue);
//	UNREFERENCED_PARAMETER(Request);
//	UNREFERENCED_PARAMETER(ActionFlags);
//}

//�豸ж��,�������ͷ�EvtDeviceAdd�������Դ, WDF��Դ�����ͷţ�WDF��ܻ��Զ�����
static VOID EvtCleanupCallback( WDFOBJECT Object)
{
	DPT("-- EvtCleanupCallback \n");

	DEV_EXT* ext = GetDeviceContext((WDFDEVICE)Object);
	////
	StopReadReport(ext);

	if(ext->timerHandle) WdfTimerStop(ext->timerHandle, TRUE);

	///

}

NTSTATUS EvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE   hDevice;
	WDF_OBJECT_ATTRIBUTES         attributes;
	WDF_IO_QUEUE_CONFIG           queueConfig;
	WDFQUEUE                      queue;
	WDF_PNPPOWER_EVENT_CALLBACKS  pnpPowerCallbacks;


	UNREFERENCED_PARAMETER(Driver);

	///���⴦�� PNP�Ĳ�ѯID������
	UCHAR miniFunc = IRP_MN_QUERY_ID;
	status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit,
		EvtPnpQueryIds,
		IRP_MJ_PNP, &miniFunc, 1);
	if (!NT_SUCCESS(status)) {
		DPT("OOPS\n");
		return status;
	}

	////����һ�� ��������
	WdfFdoInitSetFilter(DeviceInit);

	//���õ�Դ�ص�����
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	//�豸���ڹ���������D0״̬�����߷ǹ���״̬
	pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = EvtDeviceD0Entry ;
	pnpPowerCallbacks.EvtDeviceD0Exit = EvtDeviceD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks); ///

	////���������豸
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEV_EXT);
	attributes.EvtCleanupCallback = EvtCleanupCallback; //�豸ɾ��

	status = WdfDeviceCreate(&DeviceInit, &attributes, &hDevice);
	if (!NT_SUCCESS(status)) {
		DPT("WdfDeviceCreate err=0x%X\n", status);
		return status;
	}

	///���� Dispatch Queue
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
	queueConfig.EvtIoInternalDeviceControl = EvtInternalDeviceControl;
//	queueConfig.EvtIoStop = EvtIoStop;

	status = WdfIoQueueCreate(
		hDevice,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue);
	if (!NT_SUCCESS(status)) {
		DPT("WdfIoQueueCreate err=0x%X\n", status);
		return status;
	}

	////
	DEV_EXT* ext = GetDeviceContext(hDevice);

	///��ʼ������
	ext->hDevice = hDevice; ////
	ext->IoTarget = WdfDeviceGetIoTarget(hDevice);
	ext->bRequestStop = TRUE;
	ext->ReuseRequest = NULL;
	ext->RequestBuffer = NULL;
	ext->ReportCollection = NULL;
	ext->timerHandle = NULL;
	ext->ActiveCount = 0;
	KeInitializeEvent(&ext->RequestEvent, NotificationEvent, FALSE); ////֪ͨ��ʽ���¼�
	KeInitializeEvent(&ext->RequestEvent2, NotificationEvent, TRUE); ////֪ͨ��ʽ���¼�
	
	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

	queueConfig.PowerManaged = WdfFalse;

	status = WdfIoQueueCreate(
		hDevice,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&ext->ReportQueue
	);
	if (!NT_SUCCESS(status)) {
		DPT("WdfIoQueueCreate: err=0x%X\n", status );
		return status;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = hDevice;
	status = WdfCollectionCreate(&attributes, &ext->ReportCollection);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = WdfLookasideListCreate(
		WDF_NO_OBJECT_ATTRIBUTES,
		sizeof(ptp_event_t),
		NonPagedPool,
		WDF_NO_OBJECT_ATTRIBUTES,
		PoolTag,
		&ext->LookasideHandle
	);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	///
	status = create_reuse_request(ext);
	if (!NT_SUCCESS(status)) {
		DPT("create_reuse_request: err=0x%X\n", status );
		return status;
	}

	return status;
}

/////
extern "C" NTSTATUS
DriverEntry(
	 PDRIVER_OBJECT  DriverObject,
	 PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES  attributes;

	///
	WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
	////
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = EvtDriverContextCleanup;

	///// 
	status = WdfDriverCreate(
		DriverObject,
		RegistryPath,
		&attributes,      // Driver Attributes
		&config,          // Driver Config Info
		WDF_NO_HANDLE
	);
	if (!NT_SUCCESS(status)) {
		DPT("--- WdfDriverCreate err=0x%X\n", status );
	}

	////

	return status;
}

