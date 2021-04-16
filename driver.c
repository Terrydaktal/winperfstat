#include <ntddk.h>
#include <initguid.h>

//#include "wdm.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS NotImplementedDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS IoCtlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID UnloadHandler(IN PDRIVER_OBJECT DriverObject);
VOID MeasureApp(IN char** inputBuffer);
NTSTATUS CreateCloseDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, NotImplementedDispatch)
#pragma alloc_text (PAGE, IoCtlDispatch)
#pragma alloc_text (PAGE, UnloadHandler)
#pragma alloc_text (PAGE, CreateCloseDispatch)
#pragma alloc_text (PAGELK, MeasureApp)
#endif

#define BENCHMARK_DRV_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IA32_PERF_EVTSEL(x) 0x186+x
#define BuildEvent(EVT, UMASK, USR, OS, E, PC, INTT, ANY, EN, INV, CMASK) (EVT + (UMASK << 8) + (USR << 16) + (OS << 17) + (E << 18) + (PC << 19) + (INTT << 20) + (ANY << 21) + (EN << 22) + (INV << 23) + (CMASK << 24))

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
	UNREFERENCED_PARAMETER(RegistryPath);
	PAGED_CODE();

	PDEVICE_OBJECT DeviceObject = NULL;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DeviceName, DosDeviceName = { 0 };
	RtlInitUnicodeString(&DeviceName, L"\\Device\\winperfstat");
	RtlInitUnicodeString(&DosDeviceName, L"\\??\\winperfstat"); // in Global Name: winperfstat SymLink \Device\winperfstat

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		#pragma warning(push) // Disable the Compiler Warning: 28169
		#pragma warning(disable : 28169) 
		DriverObject->MajorFunction[i] = NotImplementedDispatch;
		#pragma warning(pop)
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoCtlDispatch;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseDispatch;
	DriverObject->DriverUnload = UnloadHandler;

	Status = IoCreateDevice(DriverObject,
		0,
		&DeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&DeviceObject);

	if (!NT_SUCCESS(Status)) {
		if (DeviceObject) {
			IoDeleteDevice(DeviceObject);
		}

		KeBugCheck(1);
		return Status;
	}

	Status = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);
	if (Status != STATUS_SUCCESS) KeBugCheck(2);

	DeviceObject->Flags |= DO_DIRECT_IO;
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	
	return Status;
}


VOID UnloadHandler(
	IN PDRIVER_OBJECT DriverObject
)
{
	PAGED_CODE();

	UNICODE_STRING DosDeviceName = { 0 };

	RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\winperfstat");

	IoDeleteSymbolicLink(&DosDeviceName); 	// Delete the symbolic link

	IoDeleteDevice(DriverObject->DeviceObject); 	// Delete the device
	// Driver Unloaded
}

NTSTATUS CreateCloseDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	// Complete the request
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS NotImplementedDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	// Complete the request
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_NOT_SUPPORTED;
}

NTSTATUS IoCtlDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();

	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode; // no minor code only io control code
	NTSTATUS Status = STATUS_NOT_SUPPORTED;
	unsigned long long * inputBuffer;
	int inputBufferlen;
	HANDLE hThread;
	PETHREAD ThreadObject;
	HANDLE hProcess = ZwCurrentProcess();

	inputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	inputBufferlen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
	
	__try {
		ProbeForWrite(inputBuffer, inputBufferlen, 1);
		inputBuffer[1] = (unsigned long long) Irp;
	} 
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		goto fail;
	}

	if (IrpSp) {
		switch (IoControlCode) {
		case BENCHMARK_DRV_IOCTL:
			if (PsCreateSystemThread(&hThread,
				THREAD_ALL_ACCESS,
				NULL,
				hProcess,
				NULL,
				MeasureApp,
				inputBuffer))
			{
				Status = STATUS_BAD_DATA;
				break;
			};

			if (ObReferenceObjectByHandle(
				hThread,
				THREAD_ALL_ACCESS,
				NULL,
				KernelMode,
				(PVOID*)&ThreadObject,
				NULL))
			{
				Status = STATUS_BAD_DATA;
				break;
			};

			if (ThreadObject)
			{
				if (KeWaitForSingleObject(
					ThreadObject,
					Executive,
					KernelMode,
					FALSE,
					0))
				{
					Status = STATUS_BAD_DATA;
					break;
				};

				ObDereferenceObject(ThreadObject);
			}
			Status = STATUS_SUCCESS;
			break;

		default:
			fail:
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
	PVOID hSection = MmLockPagableCodeSection(MeasureApp);
	void(*EntryPoint)() = (void(*)())(inputBuffer[0]);
	PIRP Irp = (PIRP)(inputBuffer[1]);
	PIO_STACK_LOCATION IrpSp;
	int inputBufferlen;
	KIRQL oldIrql;
	int numParams;
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	inputBufferlen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
	numParams = inputBufferlen / sizeof(PVOID);
	PULONGLONG MSRBuffer = ExAllocatePool(NonPagedPool, inputBufferlen);
	PULONGLONG CountBuffer = ExAllocatePool(NonPagedPool, inputBufferlen);
	ANSI_STRING aevstr, ainstr;
	UNICODE_STRING evstr, instr;

	for (int j = 2; j < numParams; j++) {
		RtlInitAnsiString(&ainstr, inputBuffer[j]);
		RtlAnsiStringToUnicodeString(&instr, &ainstr, TRUE);
		
		for (int i = 0; i < sizeof(Events) / sizeof(ULONGLONG); i++) {
			RtlInitAnsiString(&aevstr, Events[i]);
			RtlAnsiStringToUnicodeString(&evstr, &aevstr, TRUE);
			if (RtlEqualUnicodeString(&evstr, &instr, OBJ_CASE_INSENSITIVE))
			{
				MSRBuffer[j] = EventMSRValues[i];
			}
		}
	}

	oldIrql = KeRaiseIrqlToSynchLevel();

	for (int i = 2; i < numParams; i++) {
		if (MSRBuffer[i]) {
			__writemsr(IA32_PERF_EVTSEL(0), MSRBuffer[i]);
			CountBuffer[i] = __readpmc(0);
		}
	}

	EntryPoint();

	for (int i = 2; i < numParams; i++) {
		if (MSRBuffer[i]) {
			__writemsr(IA32_PERF_EVTSEL(0), MSRBuffer[i]);
			CountBuffer[i] = __readpmc(0) - CountBuffer[i];
		}
	}

	KeLowerIrql(oldIrql);

	MmUnlockPagableImageSection(hSection);
	
	CountBuffer[2] = __readpmc(0);

	CountBuffer[3] = 3;

	RtlCopyMemory(Irp->UserBuffer, CountBuffer, inputBufferlen);
	ExFreePool(MSRBuffer);
	ExFreePool(CountBuffer);

	return;
}
