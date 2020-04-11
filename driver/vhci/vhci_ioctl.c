#include "vhci.h"

#include "usbreq.h"
#include "vhci_devconf.h"
#include "vhci_pnp.h"
#include "usbip_vhci_api.h"

extern PAGEABLE NTSTATUS
vhci_plugin_dev(ioctl_usbip_vhci_plugin *plugin, pusbip_vhub_dev_t vhub, PFILE_OBJECT fo);

extern PAGEABLE NTSTATUS
vhci_get_ports_status(ioctl_usbip_vhci_get_ports_status *st, pusbip_vhub_dev_t vhub, ULONG *info);

extern PAGEABLE NTSTATUS
vhci_eject_device(PUSBIP_VHCI_EJECT_HARDWARE Eject, pusbip_vhub_dev_t vhub);

NTSTATUS
vhci_ioctl_abort_pipe(pusbip_vpdo_dev_t vpdo, USBD_PIPE_HANDLE hPipe)
{
	KIRQL		oldirql;
	PLIST_ENTRY	le;
	unsigned char	epaddr;

	if (!hPipe) {
		DBGI(DBG_IOCTL, "vhci_ioctl_abort_pipe: empty pipe handle\n");
		return STATUS_INVALID_PARAMETER;
	}

	epaddr = PIPE2ADDR(hPipe);

	DBGI(DBG_IOCTL, "vhci_ioctl_abort_pipe: EP: %02x\n", epaddr);

	KeAcquireSpinLock(&vpdo->lock_urbr, &oldirql);

	// remove all URBRs of the aborted pipe
	for (le = vpdo->head_urbr.Flink; le != &vpdo->head_urbr;) {
		struct urb_req	*urbr_local = CONTAINING_RECORD(le, struct urb_req, list_all);
		le = le->Flink;

		if (!is_port_urbr(urbr_local, epaddr))
			continue;

		DBGI(DBG_IOCTL, "aborted urbr removed: %s\n", dbg_urbr(urbr_local));

		if (urbr_local->irp) {
			PIRP	irp = urbr_local->irp;

			IoSetCancelRoutine(irp, NULL);
			irp->IoStatus.Status = STATUS_CANCELLED;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
		}
		RemoveEntryListInit(&urbr_local->list_state);
		RemoveEntryListInit(&urbr_local->list_all);
		free_urbr(urbr_local);
	}

	KeReleaseSpinLock(&vpdo->lock_urbr, oldirql);

	return STATUS_SUCCESS;
}

static NTSTATUS
process_urb_get_status(PURB urb)
{
	struct _URB_CONTROL_GET_STATUS_REQUEST	*urb_get = &urb->UrbControlGetStatusRequest;
	if (urb_get->TransferBuffer != NULL && urb_get->TransferBufferLength == 2) {
		*(USHORT *)urb_get->TransferBuffer = 0;
		return STATUS_SUCCESS;
	}
	return STATUS_INVALID_PARAMETER;
}

static NTSTATUS
process_urb_get_frame(pusbip_vpdo_dev_t vpdo, PURB urb)
{
	struct _URB_GET_CURRENT_FRAME_NUMBER	*urb_get = &urb->UrbGetCurrentFrameNumber;
	UNREFERENCED_PARAMETER(vpdo);

	urb_get->FrameNumber = 0;
	return STATUS_SUCCESS;
}

static NTSTATUS
submit_urbr_irp(pusbip_vpdo_dev_t vpdo, PIRP irp)
{
	struct urb_req	*urbr;
	NTSTATUS	status;

	urbr = create_urbr(vpdo, irp, 0);
	if (urbr == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;
	status = submit_urbr(vpdo, urbr);
	if (NT_ERROR(status))
		free_urbr(urbr);
	return status;
}

static NTSTATUS
process_irp_urb_req(pusbip_vpdo_dev_t vpdo, PIRP irp, PURB urb)
{
	if (urb == NULL) {
		DBGE(DBG_IOCTL, "process_irp_urb_req: null urb\n");
		return STATUS_INVALID_PARAMETER;
	}

	DBGI(DBG_IOCTL, "process_irp_urb_req: function: %s\n", dbg_urbfunc(urb->UrbHeader.Function));

	switch (urb->UrbHeader.Function) {
	case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
	case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
	case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
	case URB_FUNCTION_GET_STATUS_FROM_OTHER:
		return process_urb_get_status(urb);
	case URB_FUNCTION_ABORT_PIPE:
		return vhci_ioctl_abort_pipe(vpdo, urb->UrbPipeRequest.PipeHandle);
	case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
		return process_urb_get_frame(vpdo, urb);
	case URB_FUNCTION_SELECT_CONFIGURATION:
	case URB_FUNCTION_ISOCH_TRANSFER:
	case URB_FUNCTION_CLASS_DEVICE:
	case URB_FUNCTION_CLASS_INTERFACE:
	case URB_FUNCTION_CLASS_ENDPOINT:
	case URB_FUNCTION_CLASS_OTHER:
	case URB_FUNCTION_VENDOR_DEVICE:
	case URB_FUNCTION_VENDOR_INTERFACE:
	case URB_FUNCTION_VENDOR_ENDPOINT:
	case URB_FUNCTION_VENDOR_OTHER:
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
	case URB_FUNCTION_SELECT_INTERFACE:
	case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
	case URB_FUNCTION_CONTROL_TRANSFER_EX:
		return submit_urbr_irp(vpdo, irp);
	default:
		DBGW(DBG_IOCTL, "process_irp_urb_req: unhandled function: %s: len: %d\n",
			dbg_urbfunc(urb->UrbHeader.Function), urb->UrbHeader.Length);
		return STATUS_INVALID_PARAMETER;
	}
}

static NTSTATUS
setup_topology_address(pusbip_vpdo_dev_t vpdo, PIO_STACK_LOCATION irpStack)
{
	PUSB_TOPOLOGY_ADDRESS	topoaddr;

	topoaddr = (PUSB_TOPOLOGY_ADDRESS)irpStack->Parameters.Others.Argument1;
	topoaddr->RootHubPortNumber = (USHORT)vpdo->port;
	return STATUS_SUCCESS;
}

PAGEABLE NTSTATUS
vhci_internal_ioctl(__in PDEVICE_OBJECT devobj, __in PIRP Irp)
{
	PIO_STACK_LOCATION      irpStack;
	NTSTATUS		status;
	pusbip_vpdo_dev_t	vpdo;
	pdev_common_t		devcom;
	ULONG			ioctl_code;

	devcom = (pdev_common_t)devobj->DeviceExtension;

	DBGI(DBG_GENERAL | DBG_IOCTL, "vhci_internal_ioctl: Enter\n");

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	ioctl_code = irpStack->Parameters.DeviceIoControl.IoControlCode;

	DBGI(DBG_IOCTL, "ioctl code: %s\n", dbg_vhci_ioctl_code(ioctl_code));

	if (devcom->is_vhub) {
		DBGW(DBG_IOCTL, "internal ioctl for vhub is not allowed\n");
		Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	vpdo = (pusbip_vpdo_dev_t)devobj->DeviceExtension;

	if (!vpdo->Present) {
		DBGW(DBG_IOCTL, "device is not connected\n");
		Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_DEVICE_NOT_CONNECTED;
	}

	switch (ioctl_code) {
	case IOCTL_INTERNAL_USB_SUBMIT_URB:
		status = process_irp_urb_req(vpdo, Irp, (PURB)irpStack->Parameters.Others.Argument1);
		break;
	case IOCTL_INTERNAL_USB_GET_PORT_STATUS:
		status = STATUS_SUCCESS;
		*(unsigned long *)irpStack->Parameters.Others.Argument1 = USBD_PORT_ENABLED | USBD_PORT_CONNECTED;
		break;
	case IOCTL_INTERNAL_USB_RESET_PORT:
		status = submit_urbr_irp(vpdo, Irp);
		break;
	case IOCTL_INTERNAL_USB_GET_TOPOLOGY_ADDRESS:
		status = setup_topology_address(vpdo, irpStack);
		break;
	default:
		DBGE(DBG_IOCTL, "unhandled internal ioctl: %s\n", dbg_vhci_ioctl_code(ioctl_code));
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	if (status != STATUS_PENDING) {
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	DBGI(DBG_GENERAL | DBG_IOCTL, "vhci_internal_ioctl: Leave: %s\n", dbg_ntstatus(status));
	return status;
}

PAGEABLE NTSTATUS
vhci_ioctl(__in PDEVICE_OBJECT devobj, __in PIRP Irp)
{
	PIO_STACK_LOCATION	irpStack;
	NTSTATUS			status;
	ULONG				inlen, outlen;
	ULONG				info = 0;
	pusbip_vhub_dev_t		vhub;
	pdev_common_t			devcom;
	PVOID				buffer;
	ULONG				ioctl_code;

	PAGED_CODE();

	devcom = (pdev_common_t)devobj->DeviceExtension;

	DBGI(DBG_GENERAL | DBG_IOCTL, "vhci_ioctl: Enter\n");

	// We only allow create/close requests for the vhub.
	if (!devcom->is_vhub) {
		DBGE(DBG_IOCTL, "ioctl for vhub is not allowed\n");

		Irp->IoStatus.Status = status = STATUS_INVALID_DEVICE_REQUEST;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}

	vhub = (pusbip_vhub_dev_t)devobj->DeviceExtension;
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	ioctl_code = irpStack->Parameters.DeviceIoControl.IoControlCode;
	DBGI(DBG_IOCTL, "ioctl code: %s\n", dbg_vhci_ioctl_code(ioctl_code));

	inc_io_vhub(vhub);

	// Check to see whether the bus is removed
	if (vhub->common.DevicePnPState == Deleted) {
		status = STATUS_NO_SUCH_DEVICE;
		goto END;
	}

	buffer = Irp->AssociatedIrp.SystemBuffer;
	inlen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outlen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

	status = STATUS_INVALID_PARAMETER;

	switch (ioctl_code) {
	case IOCTL_USBIP_VHCI_PLUGIN_HARDWARE:
		if (sizeof(ioctl_usbip_vhci_plugin) == inlen) {
			status = vhci_plugin_dev((ioctl_usbip_vhci_plugin *)buffer, vhub, irpStack->FileObject);
		}
		break;
	case IOCTL_USBIP_VHCI_GET_PORTS_STATUS:
		if (sizeof(ioctl_usbip_vhci_get_ports_status) == outlen) {
			status = vhci_get_ports_status((ioctl_usbip_vhci_get_ports_status *)buffer, vhub, &info);
		}
		break;
	case IOCTL_USBIP_VHCI_UNPLUG_HARDWARE:
		if (sizeof(ioctl_usbip_vhci_unplug) == inlen) {
			status = vhci_unplug_dev(((ioctl_usbip_vhci_unplug *)buffer)->addr, vhub);
		}
		break;
	case IOCTL_USBIP_VHCI_EJECT_HARDWARE:
		if (inlen == sizeof(USBIP_VHCI_EJECT_HARDWARE) && ((PUSBIP_VHCI_EJECT_HARDWARE)buffer)->Size == inlen) {
			status = vhci_eject_device((PUSBIP_VHCI_EJECT_HARDWARE)buffer, vhub);
		}
		break;
	default:
		DBGE(DBG_IOCTL, "unhandled ioctl: %s", dbg_vhci_ioctl_code(ioctl_code));
		break;
	}

	Irp->IoStatus.Information = info;
END:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	dec_io_vhub(vhub);

	DBGI(DBG_GENERAL | DBG_IOCTL, "vhci_ioctl: Leave: %s\n", dbg_ntstatus(status));

	return status;
}
