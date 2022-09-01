/**
 * Creator: Christopher Andrew
 * Creation Date: 7/4/2022
 * Purpose: to provide CDC and DFU functionality to the device and allow external systems via a driver to send commands over a virtual serial port
 * based on the USB CDC example provided by texas instraments
 */
//##################################################################################################################################################
//include statements
//##################################################################################################################################################
//HWI
#include <inc/hw_ints.h>
#include <inc/hw_types.h>

//USB libraries
#include <usblib/usb-ids.h>
#include <usblib/usblib.h>
#include <usblib/usbcdc.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdcdc.h>

//std
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(TIVAWARE)
typedef uint32_t            USBCDCDEventType;
#else
#define eUSBModeForceDevice USB_MODE_FORCE_DEVICE
typedef unsigned long       USBCDCDEventType;
#endif

//##################################################################################################################################################
//generic USB configurations
//##################################################################################################################################################

#define USB_VID 0x6666 //change to real own VID when publishing
#define USB_PID 0xC6C4 //product ID replace with real owned when publishing
#define USB_MAX_PACKET 64 //max number of bytes that can be Tx/RX in a single packet
						//leave at 64 unless USB buffer is implemented
//##################################################################################################################################################
//DEFINE USB CDC PORT HERE
//##################################################################################################################################################

//USB DType strings are specific length with hull seperators?

//language supported
const uint8_t g_pui8LangDescriptor[] =
{
		4,
		USB_DTYPE_STRING,
		USBShort(USB_LANG_EN_US)
};

//manurfactureer name
//	Team #6 Cohort C4
const uint8_t g_pui8ManufacturerString[] =
{
		(17+1)*2,
		USB_DTYPE_STRING,
		'T', 0, 'e', 0, 'a', 0, 'm', 0, ' ', 0, '#', 0, '6', 0, ' ',
		'C', 0, 'o', 0, 'h', 0, 'o', 0, 'r', 0, 't', 0, ' ', 0, 'C', 0, '4', 0
};

//product string
//	RLS COM Port
const uint8_t g_pui8ProductString[] =
{
		(12+1)*2,
		USB_DTYPE_STRING,
		'R', 0, 'L', 0, 'S', 0, ' ', 0,
		'C', 0, 'O', 0, 'M', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0

};

//serial number string
//	12345678
const uint8_t g_pui8SerialNumberString[] =
{
	(8+1)*2,
	USB_DTYPE_STRING,
	'1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};
//control interface desecription
const uint8_t g_pui8ControlInterfaceString[] =
{
		(18+1)*2,
		USB_DTYPE_STRING,
		'R', 0, 'L', 0, 'S', 0, ' ', 0,
		'C', 0, 't', 0, 'r', 0, 'l', 0, ' ', 0,
		'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};
//configuration description string
//	External Charging, control
const uint8_t g_pui8ConfigString[] =
{
		(25+1)*2,
		USB_DTYPE_STRING,
		'E', 0, 'x', 0, 't', 0, 'e', 0, 'r', 0, 'n', 0, 'a', 0, 'l', 0,
		' ', 0, 'C', 0, 'h', 0, 'a', 0, 'r', 0, 'g', 0, 'i', 0, 'n', 0, 'g', 0,
		' ', 0, 'c', 0, 'o', 0, 'n', 0, 't', 0, 'r', 0, 'o', 0, 'l', 0
};

//usb description table
const uint8_t * const g_ppui8StringDescriptors[] =
{
		g_pui8LangDescriptor,
		g_pui8ManufacturerString,
		g_pui8ProductString,
		g_pui8SerialNumberString,
		g_pui8ControlInterfaceString,
		g_pui8ConfigString
};

//size of description table
#define NUM_STRING_DESCRIPTORS (sizeof(g_ppui8StringDescriptors) / sizeof(uint8_t *))

//defing cdc interface
const tUSBDCDCDevice g_sCDCDevice =
{
		// The Vendor ID
		USB_VID,
		//product ID
		USB_PID,
		//estimated power consumption in mA
		//update once we know the max draw of the battery while charging
		500,
		//power type
		USB_CONF_ATTR_BUS_PWR,
		//
		// A pointer to your control callback event handler.
		//
		cdc_Event_Control_Handler,
		//
		// A value that you want passed to the control callback alongside every
		// event.
		//
		0, //IDK WHAT DATA I WANT
		//
		// A pointer to your receive callback event handler.
		//
		cdc_RX_Handler,
		//
		// A value that you want passed to the receive callback alongside every
		// event.
		//
		0, //IDK WHAT DATA I WANT buffer if usb buffer is used
		//
		// A pointer to your transmit callback event handler.
		//
		YourUSBTransmitEventCallback,
		//
		// A value that you want passed to the transmit callback alongside every
		// event.
		//
		0, //IDK WHAT DATA I WANT buffer if used
		//
		// A pointer to your string table describing the cdc port
		//
		g_ppui8StringDescriptors,
		//
		// The number of entries in your string table size define
		//
		NUM_STRING_DESCRIPTORS
};

//############################################################################################################################
//usb CDC event handler functions start here
//############################################################################################################################

/*
 * Handles recieve events that fire when the device recives information over USB
 *
 */
static USBCDCDEventType cdc_RX_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
        void *eventMsgPtr)
{
	switch(event)
		{
		case USB_EVENT_RX_AVAILABLE: //data availible
			break;
		case USB_EVENT_DATA_REMAINING: //data still present?
			break;
		case USB_EVENT_ERROR: //error of some kind
			break;
		default:
			break;
		}
}

static USBCDCDEventType cdc_TX_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
        void *eventMsgPtr)
{
	switch(event)
	{
		case USB_EVENT_TX_COMPLETE: //data sent, can have more data
			break;
		default:
			break;
	}
}

/**
 * handles control events of the sub device
 *
 */
static USBCDCDEventType cdc_Event_Control_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
                                        void *eventMsgPtr)
{
	switch(event)
	{
		case USB_EVENT_CONNECTED: //change power state and stuff
			break;
		case USB_EVENT_DISCONNECTED:
			break;
		case USB_EVENT_SUSPEND:
			break;
		case USB_EVENT_RESUME:
			break;
		case USBD_CDC_EVENT_SEND_BREAK:
			break;
		case USBD_CDC_EVENT_CLEAR_BREAK:
			break;
		case USBD_CDC_EVENT_SET_LINE_CODING:
			break;
		case USBD_CDC_EVENT_GET_LINE_CODING:
			break;
		case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
			break;
	}
}

/**
 * forwards the usb interrupts to the usb lib because we can't easily point the rtos to the proper
 * 	interrupt handler in tivaware because reasons
 *
 */
static void USBCDCD_hwiHandler(Uarg arg0)
{
    USB0DeviceIntHandler();
}


//############################################################################################################################
//usb TX/RX
//############################################################################################################################

/**
 * transmits provided data
 * fails and provides error data if required
 *
 */
unsigned int USB_serialTX(char* packet, int size)
{

}

/**
 * attempts to retrive a packet from the usb buffer if there is no packet the function will wait for one
 * varius integers will be returned if this fails
 *
 */
unsigned int USB_serialRX(char* packet, int size)
{

}

//############################################################################################################################
//USB setup and utility functions
//############################################################################################################################

/**
 * sets up the composite CDC DFU device
 *
 */
unsigned int USB_Setup(void)
{

}

unsigned int USB_WaitConnect(unsigned int timeout)
{

}

//############################################################################################################################
//USB DFU functionality
//############################################################################################################################


uint32_t DFU_Detach_Callback(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData, void *pvMsgData)
{
	switch(ui32Event)
	{
		case USBD_DFU_EVENT_DETACH:
			//start up clean up then BIOS_exit somewhere else once clean up is done
			break;
	}

}

