/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)proto.h	1.2	93/11/02 SMI"

#ifdef PROTO
int CPQ_Check_Test_Pattern(int h, int Seed, int Count);
int CPQ_Register_Test(int h);
int DQNT_Check_Test_Pattern(int h, int Seed, int Count);
int DQNT_Register_Test(int h);
int HH_PGA_Register_Test(int h);
int HH_Write_Test_Pattern(int h, int Seed, int Count);
int HH_Check_Test_Pattern(int h, int Seed, int Count);
int HH_Enable_EECS(int h);
int HH_Disable_EECS(int h);
int HH_EEPROM_Put_Bit(int h, int Bit);
int HH_EEPROM_Get_Bit(int h);
int HH_Enable_Int(int h);
int HH_Disable_Int(int h);
int HH_Pulse_Int(int h);
int HH_Adapter_Reset(int h, int Flags);
int HH_Adapter_Unhook(int h);
int HH_Register_Test(int h);
int Adapter_Reset(int h);
int Adapter_Unhook(int h);
int Media_Config_Text(int h, int Config, int Mode, int Mode_Continued);
int Setup_Configuration(int h, int Mode);
int Not_Implemented(int h);
int Process_Mode_Keyword(int h);
int Process_LPT_Keyword(int h);
int PS2_PGA_Register_Test(int h);
int PS2_Write_Test_Pattern(int h, int Seed, int Count);
int PS2_Register_Test(int h);
int SH_Enable_Int(int h);
int SH_Disable_Int(int h);
int SH_Pulse_Int(int h);
int SH_PGA_Register_Test(int h);
int SH_Adapter_Reset(int h, int Flags);
int SH_Adapter_Unhook(int h);
int SH_Write_Test_Pattern(int h, int Seed, int Count);
int SH_Enable_EECS(int h);
int SH_Disable_EECS(int h);
int SH_EEPROM_Put_Bit(int h, int Bit);
int SH_EEPROM_Get_Bit(int h);
int WBT_Check_Test_Pattern(int h, int Seed, int Count);
int WBT_Register_Test(int h);
int Read_EEPROM(int h, char *Buffer);
int Write_EEPROM(int h, char *Buffer);
int Write_Wait(int h);
int Command(int h, int Cmd);
int Setup(int h);
int Out(int h, int Byte);
int Cleanup(int h);
int Microsec_Delay(int h, int ticks);
#endif
