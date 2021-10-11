/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)xcb.h	1.4	93/12/06 SMI"

#define MAXXCB 4

#ifdef SCPA

#define uchar unsigned char
#define ulong unsigned long

#ifdef PROTO
unsigned char scpa_inb(scpa_cookie_t, int offset);
int scpa_outb(scpa_cookie_t, int offset, int val);
ulong scpa_get_reg(scpa_cookie_t, int handle, ulong reg);
int scpa_set_reg(scpa_cookie_t, int handle, ulong reg, ulong val);
int scpa_set_ctrl_reg(scpa_cookie_t, int handle, ulong reg, ulong val);
int scpa_setup_get_block(scpa_cookie_t, int handle);
int scpa_finish_get_block(scpa_cookie_t, int handle);
int scpa_get_block(scpa_cookie_t,int handle, char far *buf, int count);
int scpa_setup_put_block(scpa_cookie_t, int handle);
int scpa_finish_put_block(scpa_cookie_t, int handle);
int scpa_put_block(scpa_cookie_t, int handle, char far *buf, int count);
#endif

#define OUTB(port, offset, val)  scpa_outb(xcb[h].cookie, offset, val)
#define INB(port, offset) ((unsigned char) scpa_inb(xcb[h].cookie, offset))
#define PUT_REGISTER(h, reg, val) scpa_set_reg(xcb[h].cookie, h, reg, val)
#define GET_REGISTER(h, reg) scpa_get_reg(xcb[h].cookie, h, reg)
#define FINISH_BLOCK_WRITE(h) scpa_finish_put_block(xcb[h].cookie, h)
#define BLOCK_WRITE(h, buf, count) scpa_put_block(xcb[h].cookie, h, buf,count)
#define SETUP_BLOCK_WRITE(h) scpa_setup_put_block(xcb[h].cookie, h)
#define BLOCK_READ(h, buf, count) scpa_get_block(xcb[h].cookie, h, buf, count)
#define SETUP_BLOCK_READ(h) scpa_setup_get_block(xcb[h].cookie, h)
#define FINISH_BLOCK_READ(h) scpa_finish_get_block(xcb[h].cookie, h)
#else
#define OUTB(port, offset, val)  outb(port+offset, val)
#define INB(port, offset) ((unsigned char) inb(port+offset))
#define PUT_REGISTER xcb[h].Put_Register
#define GET_REGISTER xcb[h].Get_Register
#define FINISH_BLOCK_WRITE xcb[h].Finish_Block_Write
#define BLOCK_WRITE xcb[h].Block_Write
#define SETUP_BLOCK_WRITE xcb[h].Setup_Block_Write
#define BLOCK_READ xcb[h].Block_Read
#define SETUP_BLOCK_READ xcb[h].Setup_Block_Read
#define FINISH_BLOCK_READ xcb[h].Finish_Block_Read
#endif

struct Adapter_Control {
	int	Adapter_Type;
	int	(*Check_Adapter)();
	void	(*Profile_Port)();
	void	(*Set_IRQ_Signal)();
};

struct Receive_Buffer_Header {
unsigned char RCV_NIC_Status;
unsigned char RCV_NIC_Next_Packet;	/* Unused in pe3 */
unsigned short  RCV_NIC_Byte_Count;
};

struct EEPROM_Structure {
	unsigned char	EE_Copyright[16];			/* Copyright message */
	long		EE_Serial_Number;			/* serial number, 32 bit long */
	unsigned char	EE_Manufacture_Date[4];			/* manufacture date (w/o hsecs) */
	unsigned char	EE_Model_Number[3];			/* model number, ascii string */
	unsigned char	EE_Address_Prefix[3];			/* First three bytes of network address */
	unsigned short	EE_Check_Sum;				/* 16 bit check sum */
};

/*// Configuration EEPROM Structure PE2 */
struct PE2_EEPROM_Register_Map {
char		EE_Model_Number[12];	/* model number, asciiz string */
long		EE_Serial_Number;	/* serial number, 32 bit long */
char		EE_Manufacture_Date[7];	/* manufacture date (w/o hsecs) */
char		EE_Flags;		/* miscellaneous flags */
unsigned char	EE_Network_Address[6];	/* 48 bit network address */
unsigned char	EE_Check_Sum;		/* 16 bit check sum */
};



struct xcb {
	int inuse;

	unsigned char far *Send_Header;
	unsigned char far *Receive_Header;
	int Send_Header_Size;
	int Receive_Header_Size;
	unsigned char far *Node_Address;

	int Media_Configuration;
	int Media_Memory_Address;
	int Media_IRQ_Number;
	int Media_IO_Address;
	int Media_Data_Strobe;
	int Media_Not_Data_Strobe;
	struct EEPROM_Structure PX_EEPROM_Buffer;
	char	Setup_Block_Read_Value;
	char	Setup_Block_Write_Value;
	int	EEPROM_Enable_Flag;
	void	(*Set_Ctrl_Reg)();

	void	(*User_ISR)();
	int	Media_Status;
	int MCP_Leftovers;
	int MCP_Fragment_Count;
	int MCP_Fragment_Index;
	int MS_Reenable_Ints;
	int Tx_Length;
	int Interrupt_Status;
	int SBF_Resend;
	int Rx_Leftovers;
	int Rx_Bytes_Left;
	int Link_Receive_Status;
	int Link_Integrity;
	int Receive_Mode;
	int Last_Tx_Status;

	/* host layer */

	int Hardware_Memory_Address;
	int Hardware_Status;
	int Hardware_Configuration;
	int Hardware_IRQ_Number;

	int LPT_Write_Ctrl;
	int LPT_Read_Ctrl;
	int LPT_Port_Number;
	int IRQ_Ctrl_Polarity;
	int Adapter_Type;
	struct Adapter_Control *PPT_Table_Ptr;

	struct	Receive_Buffer_Header Receive_Status;
	int Data_Strobe;

	/* pe2 */

	struct	PE2_EEPROM_Register_Map PE2_PX_EEPROM_Buffer;

	void (*Adapter_Enable_Int)();
	void (*Adapter_Disable_Int)();
	void (*Adapter_Pulse_Int)();
	void (*Adapter_Force_Int)();
	void (*Adapter_Reset_Ptr)();
	void (*Adapter_Unhook_Ptr)();
	void (*Adapter_Get_Data)();

	int (*Block_Read)();
	int  (*Get_Register)();

	void (*Put_Register)();
	void (*Block_Write)();

	void (*Setup_Block_Read)();
	void (*Setup_Block_Write)();
	void (*Finish_Block_Write)();
	void (*Finish_Block_Read)();

	int   (*Write_Test_Pattern)();
	int   (*Check_Test_Pattern)();
	void  (*EEPROM_Disable)();
	void  (*EEPROM_Enable)();
	void  (*EEPROM_Put_Bit)();
	int   (*EEPROM_Get_Bit)();

	char   *Message_Ptr;
	int	Reset_Flags;

	int Tx_Page;

	int MEM_END_PAGE;
	int XMT_BUFFERS;
	int RCV_BEG_PAGE;
	int RCV_PAGES;
	int RCV_END_PAGE;

	int	Next_Page;
	int Boundary;
	void *XCB_Link_Pointer;

#ifdef SCPA
	scpa_cookie_t cookie;
#endif

};

extern struct xcb xcb[];

struct Media_Initialize_Params {
	int Media_IO_Address;
	int Media_IRQ;
	char far *Send_Header;
	char far *Receive_Header;
	int Send_Header_Size;
	int Receive_Header_Size;
	void (*User_Service_Routine)();
	unsigned char far *Node_Address;
	void *MIP_Link_Pointer;
};
