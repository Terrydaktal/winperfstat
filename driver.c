#include "driver.h"
//#include "wdm.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, NotImplementedDispatch)
#pragma alloc_text (PAGE, IoCtlDispatch)
#pragma alloc_text (PAGE, UnloadHandler)
#pragma alloc_text (PAGE, CreateCloseDispatch)
#endif

#define BENCHMARK_DRV_IOCTL 0x69
#define IA32_PERF_EVTSEL(x) 0x186+x
#define BuildEvent(EVT, UMASK, USR, OS, E, PC, INTT, ANY, EN, INV, CMASK) (EVT + (UMASK << 8) + (USR << 16) + (OS << 17) + (E << 18) + (PC << 19) + (INTT << 20) + (ANY << 21) + (EN << 22) + (INV << 23) + (CMASK << 24))

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS NotImplementedDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS IoCtlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID UnloadHandler(IN PDRIVER_OBJECT DriverObject);
NTSTATUS CreateCloseDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID MeasureApp(IN char** inputBuffer);

LPCSTR Events[26] = {
	"LD_BLOCKS.STORE_FORWARD", //strings automatically get nul character
	"LD_BLOCKS.NO_SR",
	"LD_BLOCKS_PARTIAL.ADDRESS_ALIAS",
	"DTLB_LOAD_MISSES.MISS_CAUSES_A_WALK",
	"DTLB_LOAD_MISSES.WALK_COMPLETED",
	"DTLB_LOAD_MISSES.WALK_PENDING",
	"DTLB_LOAD_MISSES.WALK_ACTIVE",
	"DTLB_LOAD_MISSES.STLB_HIT",
	"INT_MISC.RECOVERY_CYCLES",
	"INT_MISC.RECOVERY_CYCLES_ANY",
	"INT_MISC.CLEAR_RESTEER_CYCLES",
	"UOPS_ISSUED.ANY",
	"UOPS_ISSUED.STALL_CYCLES",
	"UOPS_ISSUED.VECTOR_WIDTH_MISMATCH",
	"UOPS_ISSUED.SLOW_LEA",
	"ARITH.FPU_DIVIDER_ACTIVE",
	"L2_RQSTS.DEMAND_DATA_RD_MISS",
	"L2_RQSTS.RFO_MISS",
	"L2_RQSTS.CODE_RD_MISS",
	"L2_RQSTS.ALL_DEMAND_MISS",
	"L2_RQSTS.PF_MISS",
	"L2_RQSTS.MISS",
	"L2_RQSTS.DEMAND_DATA_RD_HIT",
	"L2_RQSTS.RFO_HIT",
	"L2_RQSTS.DEMAND_DATA_RD_HIT"
};

unsigned long long int EventMSRValues[26] = {
	BuildEvent(0x3, 0x2, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x3, 0x8, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x7, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x8, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x8, 0xE, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x8, 0x10, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x8, 0x10, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x1),
	BuildEvent(0x8, 0x20, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0xD, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0xD, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0),
	BuildEvent(0xE, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0xE, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1),
	BuildEvent(0xE, 0x2, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0xE, 0x20, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x14, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x21, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x22, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x24, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x27, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x38, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x3F, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x41, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x42, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0x44, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0xD8, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0x24, 0xE1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0)
};

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	PDEVICE_OBJECT DeviceObject = NULL;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DeviceName, DosDeviceName = { 0 };
	RtlInitUnicodeString(&DeviceName, L"\\Device\\winperfstat");
	RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\winperfstat"); // in Global Name: winperfstat SymLink \Device\winperfstat

	Status = IoCreateDevice(DriverObject,
		0,
		&DeviceName,
		FILE_DEVICE_UNKNOWN,
		NULL,
		FALSE,
		&DeviceObject);

	if (!NT_SUCCESS(Status)) {
		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}

		return Status;
	}

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
#pragma warning(push) // Disable the Compiler Warning: 28169
#pragma warning(disable : 28169) 
		DriverObject->MajorFunction[i] = NotImplementedDispatch;
#pragma warning(pop)
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseDispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoCtlDispatch;
	DriverObject->DriverUnload = UnloadHandler;

	DeviceObject->Flags |= DO_DIRECT_IO;
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	Status = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);
	return Status;
}


VOID UnloadHandler(
	IN PDRIVER_OBJECT DriverObject
)
{
	UNICODE_STRING DosDeviceName = { 0 };

	PAGED_CODE();

	RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\winperfstat");

	IoDeleteSymbolicLink(&DosDeviceName); 	// Delete the symbolic link

	IoDeleteDevice(DriverObject->DeviceObject); 	// Delete the device
	// Driver Unloaded
}

NTSTATUS NotImplementedDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	// Complete the request
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_NOT_SUPPORTED;
}

NTSTATUS CreateCloseDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	// Complete the request
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS IoCtlDispatch(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
)
{
	PIO_STACK_LOCATION IrpSp = NULL;
	NTSTATUS Status = STATUS_NOT_SUPPORTED;
	ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode; // no minor code only io control code
	char** inputBuffer;

	HANDLE hThread;
	
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	HANDLE hProcess = ZwCurrentProcess();

	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	inputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	*((char**)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer + 1) = Irp;


	if (IrpSp) {
		switch (IoControlCode) {
		case BENCHMARK_DRV_IOCTL:
			PsCreateSystemThread(&hThread,
				THREAD_ALL_ACCESS,
				NULL, 
				hProcess, 
				NULL,
				MeasureApp,
				inputBuffer
			);
			break;

		default:
			DbgPrint("[-] Invalid IOCTL Code: 0x%X\n", IoControlCode);
			Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;

	// Complete the request
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

void MeasureApp(
	IN char** inputBuffer
)
{
	unsigned long long int before = 0;
	unsigned long long int after = 0;
	void(*EntryPoint)() = inputBuffer[0];
	PIRP Irp = inputBuffer[1];
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	int inputBufferlen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
	
	__writemsr(IA32_PERF_EVTSEL(0), inputBuffer[2]);
	before = __readpmc(0);
	EntryPoint();
	after = __readpmc(0);
}
