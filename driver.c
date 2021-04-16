#include <ntddk.h>
#include <initguid.h>

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS NotImplementedDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS IoCtlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID UnloadHandler(IN PDRIVER_OBJECT DriverObject);
VOID MeasureApp(IN unsigned long long * outputBuffer);
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

char* Events[31] = {
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
	"L2_RQSTS.CODE_RD_HIT",
	"L2_RQSTS.PF_HIT",
	"UOPS_RETIRED.RETIRE_SLOTS",
	"UOPS_RETIRED.ALL",
	"UOPS_RETIRED.ACTIVE_CYCLES",
	"UOPS_RETIRED.STALL_CYCLES",

	"UOPS_ISSUED.FLAGS_MERGE"
};

unsigned long long int EventMSRValues[31] = {
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

	BuildEvent(0xD, 0x80, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
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
	BuildEvent(0xC2, 0x02, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0xC2, 0x01, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),
	BuildEvent(0xC2, 0x01, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x1),
	BuildEvent(0xC2, 0x01, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1),

	BuildEvent(0x0E, 0x10, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0),

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

		return Status;
	}

	Status = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);
	if (Status != STATUS_SUCCESS) return Status;

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
	
	HANDLE hThread;
	PETHREAD ThreadObject;
	PULONGLONG MSRBuffer = NULL;
	unsigned long long * output = NULL;
	NTSTATUS Status = STATUS_BAD_DATA;
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	if (!IrpSp) goto fail;
	ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode; // no minor code only io control code
	if (IoControlCode != BENCHMARK_DRV_IOCTL) goto fail;
	unsigned long long * inputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	ULONGLONG inputBufferlen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
	HANDLE hProcess = ZwCurrentProcess();
	ULONG numParams = inputBufferlen / sizeof(PVOID);
	ANSI_STRING aevstr = { 0 }, ainstr = { 0 };
	UNICODE_STRING evstr = { 0 }, instr = { 0 };

	MSRBuffer = ExAllocatePool(NonPagedPool, inputBufferlen);
	RtlZeroMemory(MSRBuffer, inputBufferlen);
	output = ExAllocatePool(NonPagedPool, inputBufferlen);
	RtlZeroMemory(output, inputBufferlen);

	//int threadsRequired = (numParams - 2) / 4 + !!((numParams - 2) % 4);
	//HANDLE* threads = ExAllocatePool(NonPagedPool, threadsRequired*sizeof(HANDLE*));

	for (unsigned j = 2; j < numParams; j++) {
		RtlInitAnsiString(&ainstr, (char*)(inputBuffer[j]));
		RtlAnsiStringToUnicodeString(&instr, &ainstr, TRUE);

		for (unsigned i = 0; i < sizeof(Events) / sizeof(ULONGLONG); i++) {
			RtlInitAnsiString(&aevstr, Events[i]);
			RtlAnsiStringToUnicodeString(&evstr, &aevstr, TRUE);
			if (RtlEqualUnicodeString(&evstr, &instr, OBJ_CASE_INSENSITIVE))
			{
				MSRBuffer[j] = EventMSRValues[i];
			}
			RtlFreeUnicodeString(&evstr);
		}
		RtlFreeUnicodeString(&instr);
	}

	__try {
		ProbeForRead(inputBuffer, inputBufferlen, 1);
		ProbeForWrite(inputBuffer, inputBufferlen, 1);
		ProbeForRead(Irp->UserBuffer, inputBufferlen, 1);
		ProbeForWrite(Irp->UserBuffer, inputBufferlen, 1);
	} 
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		goto fail;
	}

	inputBuffer[1] = (unsigned long long) Irp;

	unsigned long long threadparams[4] = {numParams, inputBuffer[0], (unsigned long long) MSRBuffer, (unsigned long long) output};

	if (PsCreateSystemThread(&hThread,
		THREAD_ALL_ACCESS,
		NULL,
		hProcess,
		NULL,
		MeasureApp,
	    threadparams))
	{
		goto fail;
	};



	if (ObReferenceObjectByHandle(
		hThread,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		(PVOID*)&ThreadObject,
		NULL))
	{
		goto fail;
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
			goto fail;
		};

		ObDereferenceObject(ThreadObject);
	}

	Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = inputBufferlen;
	RtlCopyMemory(Irp->UserBuffer, output, inputBufferlen);

	fail:
	Irp->IoStatus.Status = Status;
	ExFreePool(output);
	ExFreePool(MSRBuffer);
	
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

void MeasureApp(
	IN unsigned long long* threadparams
)
{
	KIRQL oldIrql;
	PVOID hSection;
	ULONG numParams = (ULONG)threadparams[0];
	void(*EntryPoint)() = (void(*)())(threadparams[1]);
	unsigned long long* MSRBuffer = (unsigned long long*) threadparams[2];
	unsigned long long* outputBuffer = (unsigned long long*) threadparams[3];
	ULONGLONG inputBufferlen = numParams * sizeof(unsigned long long);
	PULONGLONG countBuffer = ExAllocatePool(NonPagedPool, inputBufferlen);
	RtlZeroMemory(countBuffer, inputBufferlen);
	hSection = MmLockPagableCodeSection(MeasureApp);
	oldIrql = KeRaiseIrqlToSynchLevel();
	
	for (unsigned i = 2; i < numParams; i++) {

		if (MSRBuffer[i]) {
			__writemsr(IA32_PERF_EVTSEL((i - 2)%4), MSRBuffer[i]);
		}

		if (!((i-1) % 4)) {
			countBuffer[i-3] = __readpmc(0);
			countBuffer[i-2] = __readpmc(1);
			countBuffer[i-1] = __readpmc(2);
			countBuffer[i] = __readpmc(3);
			EntryPoint();
			outputBuffer[i-3] = __readpmc(0);
			outputBuffer[i-2] = __readpmc(1);
			outputBuffer[i-1] = __readpmc(2);
			outputBuffer[i] = __readpmc(3);
		}
	}

	switch ((numParams-2) % 4) {
		case 1:
			__writemsr(IA32_PERF_EVTSEL(0), MSRBuffer[numParams-1]);
			countBuffer[numParams - 1] = __readpmc(0);
			EntryPoint();
			outputBuffer[numParams - 1] = __readpmc(0);
			

		case 2:
			__writemsr(IA32_PERF_EVTSEL(0), MSRBuffer[numParams - 1]);
			__writemsr(IA32_PERF_EVTSEL(1), MSRBuffer[numParams - 2]);
			countBuffer[numParams - 1] = __readpmc(0);
			countBuffer[numParams - 2] = __readpmc(1);
			EntryPoint();
			outputBuffer[numParams - 1] = __readpmc(0);
			outputBuffer[numParams - 2] = __readpmc(1);

		case 3:
			__writemsr(IA32_PERF_EVTSEL(0), MSRBuffer[numParams - 1]);
			__writemsr(IA32_PERF_EVTSEL(1), MSRBuffer[numParams - 2]);
			__writemsr(IA32_PERF_EVTSEL(2), MSRBuffer[numParams - 3]);
			countBuffer[numParams - 1] = __readpmc(0);
			countBuffer[numParams - 2] = __readpmc(1);
			countBuffer[numParams - 3] = __readpmc(2);
			EntryPoint();
			outputBuffer[numParams - 1] = __readpmc(0);
			outputBuffer[numParams - 2] = __readpmc(1);
			outputBuffer[numParams - 3] = __readpmc(2);
	}

	for (unsigned i = 2; i < numParams; i++) {
		if (MSRBuffer[i]) {
			outputBuffer[i] = outputBuffer[i] - countBuffer[i];
		}
	}

	KeLowerIrql(oldIrql);

	MmUnlockPagableImageSection(hSection);

	ExFreePool(countBuffer);

	return;
}
