
/********************************** ZMC系列控制器  ************************************************
**--------------文件信息--------------------------------------------------------------------------------
**文件名: zmcaux.c
**创建人: zxy
**时间: 20130621
**描述: ZMCDLL 辅助函数

本库提供对EXCUTE等在线命令执行函数的封装，不对控制器本身的执行程序进行修改操作.
  
	
	  
**------------修订历史记录----------------------------------------------------------------------------
		
** 修改人: zxy
** 版  本: 1.1
** 日　期: 2014.5.11
** 描　述: ZMC_ExecuteNoAck 替换为 ZMC_Execute
		  
			
** 修改人: zxy
** 版  本: 1.3
** 日　期: 2014.7.21
** 描　述: ZMC_Execute ZMC_DirectCommand 替换为ZAux_Execute ZAux_DirectCommand
			  
增加 ZAux_SetParam  ZAux_GetParam  ZAux_Direct_SetParam  ZAux_Direct_GetParam
				
增加 ZAux_WriteUFile  ZAux_ReadUFile
				  
** 修改人: wy
** 版  本: 1.5
** 日　期: 2016.6.6
** 描　述: 对所有BASIC指令进行封装，整合ZMC库到AUX库

** 修改人: wy
** 版  本: 2.1
** 日　期: 2018.8.24
** 描  述：添加PCI链接函数
**		   对所有BASIC指令运动指令进行封装，封装轴列表到函数
**		   增加部分总线指令
**		   增加部分MOVE_PARA指令
		   增加位置比较输出指令
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/

//#include "stdafx.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "stdarg.h"
#include "ctype.h"
#include "malloc.h"
#include "stdlib.h"



#include "zmotion.h"
#include "zmcaux.h"

#ifdef Z_DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = "zmcaux";
#endif

int g_ZMC_MaxExcuteWaitms = 1000;
int g_ZMC_bIfDebugtoFile = false;
char g_ZMC_aDebugFileName[2048] = "zauxcmd.txt";

#if 0
//错误输出部分
#endif

void  ZAux_TraceOut(const char * cText, ...)
{
    char ErrorText[2048];

	
    va_list valist;
    
    // Build variable text buffer
    va_start(valist, cText);
    vsprintf(ErrorText, cText, valist);
    va_end(valist);
	
	
    //OutputDebugString( ErrorText );
	
	
#ifdef ZAUX_DEBUG
	FILE * DebugFileId;
	DebugFileId = fopen(ZAUX_DEBUG_FILE,"a");
	if(NULL == DebugFileId)
	{
		::MessageBox(NULL, "Can not open file", _T("Error"), MB_OK | MB_ICONEXCLAMATION);
	}
	else
	{
		fputs(ErrorText, DebugFileId);
		fclose(DebugFileId);
	}
	
#endif    
	
}

/*************************************************************
Description:    //与控制器建立链接， 串口方式.
Input:          //串口号COMId 
Output:         //卡链接phandle
Return:         //错误码
*************************************************************/
int32  ZAux_OpenCom(uint32 comid, ZMC_HANDLE * phandle)
{
	int32 iresult;
	iresult = ZMC_OpenCom(comid, phandle);
	
	return iresult;
}

/*************************************************************
Description:    //快速控制器建立链接
Input:          //最小串口号uimincomidfind
Input:          //最大串口号uimaxcomidfind
Input:          //链接时间uims
Output:         //有效COM pcomid
Output:         //卡链接handle
Return:         //错误码
*************************************************************/
int32  ZAux_SearchAndOpenCom(uint32 uimincomidfind, uint32 uimaxcomidfind,uint* pcomid, uint32 uims, ZMC_HANDLE * phandle)
{
	int32 iresult;
	iresult =ZMC_SearchAndOpenCom(uimincomidfind, uimaxcomidfind, pcomid,uims, phandle);
	
	return iresult;
}

/*************************************************************
Description:    //可以修改缺省的波特率等设置
Input:          //dwBaudRate 波特率
				dwByteSize   数据位 
				dwParity = NOPARITY,校验位 
				dwStopBits = ONESTOPBIT停止位
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_SetComDefaultBaud(uint32 dwBaudRate, uint32 dwByteSize, uint32 dwParity, uint32 dwStopBits)
{
	int32 iresult;
	iresult = ZMC_SetComDefaultBaud(dwBaudRate,dwByteSize,dwParity,dwStopBits);
	
	return iresult;
}


/*************************************************************
Description:    //修改控制器IP地址
Input:          //卡链接handle 
Input:          //ipaddress IP地址  
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_SetIp(ZMC_HANDLE handle, char * ipaddress)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
    //生成命令
	sprintf(cmdbuff, "IP_ADDRESS=%s", ipaddress);
	
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	return  ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
}

/*************************************************************
Description:    //与控制器建立链接
Input:          //IP地址，字符串的方式输入
Output:         //卡链接handle
Return:         //错误码
*************************************************************/
int32  ZAux_OpenEth(char *ipaddr, ZMC_HANDLE * phandle)
{
	int32 iresult;
	iresult = ZMC_OpenEth(ipaddr, phandle);
	
	return iresult;
}

/*************************************************************
Description:    //快速检索IP列表
Input:          //uims 响应时间
Input:          //addrbufflength		最大长度
output:			//ipaddrlist		当前晚点IP列表
Return:         //错误码, ERR_OK表示有搜索到.
*************************************************************/
int32  ZAux_SearchEthlist(char *ipaddrlist, uint32 addrbufflength, uint32 uims)
{
    //自动搜索IP地址
	int32 iresult;
    iresult = ZMC_SearchEth(ipaddrlist, addrbufflength, uims);
	return iresult;
}

/*************************************************************
Description:    //快速检索控制器
Input:          //ipaddress 控制器IP地址
Input:          //uims 响应时间
Output:         //
Return:         //错误码, ERR_OK表示有搜索到.
*************************************************************/
int32  ZAux_SearchEth(const char *ipaddress,  uint32 uims)
{
    //自动搜索IP地址
    char buffer[10240];
	int32 iresult;
    
    //
    iresult = ZMC_SearchEth(buffer, 10230, uims);
    if(ERR_OK != iresult)
    {
		return 20010; //错误
    }
	
    //从字符串转换过来
    int ipos =0;
    const char * pstring;
    pstring = buffer;
	
    for(int j= 0; j< 100;j++) //最多100个IP列表
    {
        char buffer2[256];
        buffer2[0] = '\0';
		
        //跳过空格
        while(' ' == pstring[0])
        {
            pstring++;
        }
        
        ipos = sscanf(pstring , "%s", &buffer2);
        if(EOF  == ipos)
        {
            break;
        }
		
        //跳过字符
        while((' ' != pstring[0]) && ('\t' != pstring[0]) && ('\0' != pstring[0]))
        {
            pstring++;
        }
        
		if(0 == strcmp(buffer2, ipaddress))
		{
			return ERR_OK;
		}
		
    }
    
	return 20010; //错误
}


/*************************************************************
Description:    //关闭控制器链接
Input:          //卡链接handle
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Close(ZMC_HANDLE  handle)
{
	int32 iresult;
	iresult = ZMC_Close(handle);
	
	return iresult;	
}

/*************************************************************
Description:    //暂停继续运行BAS项目
Input:          //卡链接handle
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Resume(ZMC_HANDLE handle)
{
	int32 iresult;
	iresult = ZMC_Resume(handle);
	
	return iresult;	
}

/*************************************************************
Description:    //暂停控制器中BAS程序
Input:          //卡链接handle
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Pause(ZMC_HANDLE handle)
{
	int32 iresult;
	iresult = ZMC_Pause(handle);
	
	return iresult;	
}


/*************************************************************
Description:    //单个BAS文件生成ZAR并且下载到控制器运行
Input:          //卡链接handle
Input:          //Filename BAS文件路径
Input:          //下载到RAM-ROM  0-RAM  1-ROM
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_BasDown(ZMC_HANDLE handle,const char *Filename,uint32 run_mode )
{
	int32 	iresult;
    char    atemp[4096];
    char    adir[4096];
	
    iresult = ZMC_MakeOneFileZpj(atemp, adir, Filename);
    if(ERR_OK != iresult)
    {
        return iresult;
    }
    
	if (run_mode == 0)
	{
		iresult = ZMC_MakeZarAndRamRun2(handle, atemp, adir, NULL, 0);
    
	}
	else
	{
		iresult = ZMC_MakeZarAndDown2(handle, atemp, adir, NULL, 0);
	}

	if(ERR_OK != iresult)
    {
		return iresult;
    }        
	
    iresult = ZMC_Resume(handle);
    if(ERR_OK != iresult)
    {
		return iresult;
    }  
	
	return ERR_OK;
}



/*************************************************************
Description:    //封装 Excute 函数, 以便接收错误
Input:          //卡链接			handle
Input:          //字符串命令		pszCommand
Input:			//返回的字符长度	uiResponseLength 
Output:         //返回的字符串		psResponse
Return:         //错误码
*************************************************************/
int32  ZAux_Execute(ZMC_HANDLE handle, const char* pszCommand, char* psResponse, uint32 uiResponseLength)
{
	int32 iresult;
	iresult = ZMC_Execute(handle, pszCommand, g_ZMC_MaxExcuteWaitms, psResponse, uiResponseLength);
	if(ERR_OK != iresult)
	{
		ZAUX_ERROR2("ZMC_Execute:%s error:%d.",  pszCommand, iresult);
	}

	//把命令写入文件
	if(g_ZMC_bIfDebugtoFile)
	{
		FILE * DebugFileId;
		char  Backbuff[2048];
		char time_str[32];
		time_t SysTime = time(0); 
		DebugFileId = fopen(g_ZMC_aDebugFileName,"a");
		if(NULL == DebugFileId)
		{
			//::MessageBox(NULL, "Can not open file", _T("Error"), MB_OK | MB_ICONEXCLAMATION);
		}
		else
		{

			if(g_ZMC_bIfDebugtoFile == 1)		//错误输出
			{
				if(ERR_OK != iresult) 
				{
					strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&SysTime));
					sprintf(Backbuff,"%s\t%s\tError:%d\r",time_str,pszCommand,iresult);
					fputs(Backbuff, DebugFileId);
				}

			}
			else if( g_ZMC_bIfDebugtoFile == 2)  //非读取返回
			{
				//
				if(0 == uiResponseLength) 
				{
					strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&SysTime));

					if(iresult != 0 )
					{
						sprintf(Backbuff,"%s\t%s\tError:%d\r",time_str,pszCommand,iresult);
					}
					else
					{
						sprintf(Backbuff,"%s\t%s\r",time_str,pszCommand);				
					}
					fputs(Backbuff, DebugFileId);
				}
			}
			else										
			{
				strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&SysTime));

				if(iresult != 0 )
				{
					sprintf(Backbuff,"%s\t%s\tError:%d\r",time_str,pszCommand,iresult);

				}
				else if(0 != uiResponseLength)
				{
					sprintf(Backbuff,"%s\t%s\tRe:%s\r",time_str,pszCommand,psResponse);
				}
				else
				{
					sprintf(Backbuff,"%s\t%s\r",time_str,pszCommand);			
				}
				fputs(Backbuff, DebugFileId);
			}

			fclose(DebugFileId);
		}
	}

	return iresult;
}


/*************************************************************
Description:    //封装 DirectCommand 函数, 以便接收错误
Input:          //卡链接			handle
Input:          //字符串命令		pszCommand
Input:			//返回的字符长度	uiResponseLength 
Output:         //返回的字符串		psResponse
Return:         //错误码
*************************************************************/
int32  ZAux_DirectCommand(ZMC_HANDLE handle, const char* pszCommand, char* psResponse, uint32 uiResponseLength)
{
	int32 iresult;
	iresult = ZMC_DirectCommand(handle, pszCommand, psResponse, uiResponseLength);
	if(ERR_OK != iresult)
	{
		ZAUX_ERROR2("ZMC_DirectCommand:%s error:%d.", pszCommand, iresult);
	}

	//把命令写入文件
	if(g_ZMC_bIfDebugtoFile)
	{
		FILE * DebugFileId;
		char  Backbuff[2048];
		char time_str[32];
		time_t SysTime = time(0); 
		DebugFileId = fopen(g_ZMC_aDebugFileName,"a");
		if(NULL == DebugFileId)
		{
			//::MessageBox(NULL, "Can not open file", _T("Error"), MB_OK | MB_ICONEXCLAMATION);
		}
		else
		{

			if(g_ZMC_bIfDebugtoFile == 1)		//错误输出
			{
				if(ERR_OK != iresult) 
				{
					strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&SysTime));
					sprintf(Backbuff,"%s\t%s\tError:%d\r",time_str,pszCommand,iresult);
					fputs(Backbuff, DebugFileId);
				}

			}
			else if( g_ZMC_bIfDebugtoFile == 2)  //非读取返回
			{
				//
				if(0 == uiResponseLength) 
				{
					strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&SysTime));

					if(iresult != 0 )
					{
						sprintf(Backbuff,"%s\t%s\tError:%d\r",time_str,pszCommand,iresult);
					}
					else
					{
						sprintf(Backbuff,"%s\t%s\r",time_str,pszCommand);				
					}
					fputs(Backbuff, DebugFileId);
				}
			}
			else										
			{
				strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&SysTime));

				if(iresult != 0 )
				{
					sprintf(Backbuff,"%s\t%s\tError:%d\r",time_str,pszCommand,iresult);

				}
				else if(0 != uiResponseLength)
				{
					sprintf(Backbuff,"%s\t%s\tRe:%s\r",time_str,pszCommand,psResponse);
				}
				else
				{
					sprintf(Backbuff,"%s\t%s\r",time_str,pszCommand);			
				}
				fputs(Backbuff, DebugFileId);
			}

			fclose(DebugFileId);
		}
	}
	return iresult;
}


/*************************************************************
Description:    //命令跟踪设置.
Input:          //卡链接handle 
bifTofile		0 关闭  1-只输出错误命令  2-只输出运动与设置命令  3输出全部命令
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_SetTraceFile(int bifTofile, const char *pFilePathName)
{
	g_ZMC_bIfDebugtoFile = bifTofile;
	strcpy(g_ZMC_aDebugFileName, pFilePathName);
	
	return ERR_OK;
}


#if 0
//****************************************************IO指令**************************
// 可以使用 ZMC_GetIn ZMC_GetOutput 等
#endif

/*************************************************************
Description:    //读取输入信号
Input:          //卡链接handle
				ionum IN编号
Output:         //piValue 输入口状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetIn(ZMC_HANDLE handle, int ionum , uint32 *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?IN(%d)", ionum);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //打开输出信号
Input:          //卡链接handle 
				ionum 输出口编号
				iValue	输出口状态
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetOp(ZMC_HANDLE handle, int ionum, uint32 iValue)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
    //生成命令
	sprintf(cmdbuff, "op(%d,%d)", ionum, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取输出口状态
Input:          //卡链接handle  
				ionum 输出口编号
Output:         //piValue 输出口状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetOp(ZMC_HANDLE handle, int ionum, uint32 *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?OP(%d)", ionum);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取模拟量输入信号
Input:          //卡链接handle 
				ionum AIN口编号			
Output:         //pfValue 返回的模拟量值 4系列以下0-4095
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAD(ZMC_HANDLE handle, int ionum , float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?AIN(%d)", ionum);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //打开模拟量输出信号
Input:          //卡链接handle  
				ionum DA输出口编号
				fValue 设定的模拟量值4系列以下0-4095
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetDA(ZMC_HANDLE handle, int ionum, float fValue)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
    //生成命令
	sprintf(cmdbuff, "AOUT(%d) = %f", ionum, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取模拟输出口状态
Input:          //卡链接handle  
				ionum 模拟量输出口编号
Output:         //pfValue 读取的的模拟量值 4系列以下0-4095
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetDA(ZMC_HANDLE handle, int ionum, float *pfValue)
{
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?AOUT(%d)", ionum);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置输入口反转
Input:          //卡链接handle  
				 ionum 输入口编号
				 bifInvert 反转状态 0/1
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetInvertIn(ZMC_HANDLE handle, int ionum, int bifInvert)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
    //生成命令
	sprintf(cmdbuff, "INVERT_IN(%d,%d)", ionum, bifInvert);
	
	//调用命令执行函数
	return  ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取输入口反转状态
Input:          //卡链接handle  
				ionum 输入口编号
Output:         //piValue 反转状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetInvertIn(ZMC_HANDLE handle, int ionum, int *piValue)
{
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?INVERT_IN(%d)", ionum);
	
	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //设置pwm频率
Input:          //卡链接handle  
				ionum PWM编号口
				fValue 频率 硬件PWM1M 软PWM 2K
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetPwmFreq(ZMC_HANDLE handle, int ionum, float fValue)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
    //生成命令
	sprintf(cmdbuff, "PWM_FREQ(%d) = %f", ionum, fValue);
	
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
}

/*************************************************************
Description:    //读取pwm频率
Input:          //卡链接handle  
				ionum PWM口编号
Output:         //pfValue 返回的频率
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetPwmFreq(ZMC_HANDLE handle, int ionum, float *pfValue)
{
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
   	if(NULL == pfValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?PWM_FREQ(%d)", ionum);
	
	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置pwm占空比
Input:          //卡链接handle  
				ionum PWM口编号
				fValue 占空变	0-1  0表示关闭PWM口
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetPwmDuty(ZMC_HANDLE handle, int ionum, float fValue)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
    //生成命令
	sprintf(cmdbuff, "PWM_DUTY(%d) = %f", ionum, fValue);
	
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
}

/*************************************************************
Description:    //读取pwm占空比
Input:          //卡链接handle  
				ionum PWM口编号
Output:         //pfValue 读取的占空比
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetPwmDuty(ZMC_HANDLE handle, int ionum, float *pfValue)
{
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
   	if(NULL == pfValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?PWM_DUTY(%d)", ionum);
	
	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}



#if 0
//通过modbus快速读取特殊寄存器
#endif

/*************************************************************
Description:    //参数 快速读取多个输入
Input:          //卡链接handle  
				ionumfirst IN起始编号
				ionumend	IN结束编号
Output:         //pValueList 位状态 按位存储
Return:         //错误码
*************************************************************/
int32  ZAux_GetModbusIn(ZMC_HANDLE handle, int ionumfirst, int ionumend, uint8 *pValueList)
{
    if(ionumend < ionumfirst)
    {
        return ERR_AUX_PARAERR;
    }
    
    return ZMC_Modbus_Get0x(handle, 10000+ionumfirst, ionumend-ionumfirst +1, pValueList);
}

/*************************************************************
Description:    //参数 快速读取多个当前的输出状态
Input:          //卡链接handle  
				ionumfirst IN起始编号
				ionumend	IN结束编号
Output:         //pValueList 位状态 按位存储
Return:         //错误码
*************************************************************/
int32  ZAux_GetModbusOut(ZMC_HANDLE handle, int ionumfirst, int ionumend, uint8 *pValueList)
{
    if(ionumend < ionumfirst)
    {
        return ERR_AUX_PARAERR;
    }
    
    return ZMC_Modbus_Get0x(handle, 20000+ionumfirst, ionumend-ionumfirst +1, pValueList);
}

/*************************************************************
Description:    //参数 快速读取多个当前的DPOS
Input:          //卡链接handle  
				imaxaxises 轴数量
Output:         //pValueList 读取的坐标值 从轴0开始
Return:         //错误码
*************************************************************/
int32  ZAux_GetModbusDpos(ZMC_HANDLE handle, int imaxaxises, float *pValueList)
{
    return ZMC_Modbus_Get4x(handle, 10000, imaxaxises*2, (uint16 *)pValueList);
}

/*************************************************************
Description:    //参数 快速读取多个当前的MPOS
Input:          //卡链接handle 
				imaxaxises 轴数量
Output:         //pValueList 读取的反馈坐标值 从轴0开始
Return:         //错误码
*************************************************************/
int32  ZAux_GetModbusMpos(ZMC_HANDLE handle, int imaxaxises, float *pValueList)
{
    return ZMC_Modbus_Get4x(handle, 11000, imaxaxises*2, (uint16 *)pValueList);
}

/*************************************************************
Description:    //参数 快速读取多个当前的速度
Input:          //卡链接handle  
				imaxaxises 轴数量
Output:         //pValueList 读取的当前速度 从轴0开始
Return:         //错误码
*************************************************************/
int32  ZAux_GetModbusCurSpeed(ZMC_HANDLE handle, int imaxaxises, float *pValueList)
{
    return ZMC_Modbus_Get4x(handle, 12000, imaxaxises*2, (uint16 *)pValueList);
}

#if 0
//采用ZAux_DirectCommand 来快速获取一些状态, ZAux_DirectCommand的执行比ZMC_Execute要快
// 只有参数，变量，数组元素等能使用ZAux_DirectCommand
// 20130901以后的版本，一些运动函数也可以调用ZAux_DirectCommand，当运动条件不满足的时候，会立刻返回失败。
// ZAux_DirectCommand调用运动函数时，参数必须是具体的数值，不能是变量表达式。
#endif

#if 0
//**************************************轴参数部分***************************************
#endif


/*************************************************************
Description:    //通用的参数修改函数 sParam: 填写参数名称
Input:          //卡链接handle 
				sParam 轴参数名称 "DPOS" ...
				iaxis 轴号
				fset 设定值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetParam(ZMC_HANDLE handle,const char *sParam,int iaxis, float fset)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
    //生成命令
	sprintf(cmdbuff, "%s(%d)=%f", sParam,iaxis, fset);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //参数 通用的参数读取函数, sParam:填写参数名称
Input:          //卡链接handle  
				sParam 轴参数名称 "DPOS" ...
				iaxis 轴号
Output:         //pfValue  读取的返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetParam(ZMC_HANDLE handle,const char *sParam, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?%s(%d)", sParam,iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置加速度
Input:          //卡链接handle  
				iaxis 轴号
				fValue  设定值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetAccel(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "ACCEL(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取加速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 加速度返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAccel(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ACCEL(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取叠加轴
Input:          //卡链接handle  
				iaxis 轴号
Output:         //读取的轴叠加轴号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAddax(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ADDAX_AXIS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置轴告警信号
Input:          //卡链接handle  
				iaxis 轴号
				iValue 报警信号输入口编号，取消时设定-1
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetAlmIn(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "ALM_IN(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取告警信号
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 报警信号输入口返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAlmIn(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ALM_IN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置轴类型
Input:          //卡链接handle 
				iaxis 轴号
				iValue 轴类型
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetAtype(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "ATYPE(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取轴类型
Input:          //卡链接handle 
				iaxis 轴号
Output:         //iValue 轴类型返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAtype(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ATYPE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取轴状态
Input:          //卡链接handle  
				iaxis 轴号
Output:         //轴状态返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAxisStatus(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?AXISSTATUS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置轴地址
Input:          //卡链接handle  
				iaxis 轴号
				iValue 轴地址设定值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetAxisAddress(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "AXIS_ADDRESS(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取轴地址
Input:          //卡链接handle  
				iaxis 轴号 
Output:         //piValue 轴地址返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAxisAddress(ZMC_HANDLE handle, int iaxis, int *piValue)
{
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?AXIS_ADDRESS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置轴使能 （只针对总线控制器轴使用有效）
Input:          //卡链接handle  
				iaxis 轴号
				iValue 状态 0-关闭 1- 打开
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetAxisEnable(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "AXIS_ENABLE(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取轴使能状态
Input:          //卡链接handle 
				iaxis 轴号
Output:         //piValue 返回的使能状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAxisEnable(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?AXIS_ENABLE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置链接速率
Input:          //卡链接handle 
				iaxis 轴号
				fValue 同步连接速率
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetClutchRate(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "CLUTCH_RATE(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取链接速率
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 连接速率返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetClutchRate(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?CLUTCH_RATE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置锁存触发的结束坐标范围点
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设定的范围值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetCloseWin(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "CLOSE_WIN(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取锁存触发的结束坐标范围点
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的范围值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetCloseWin(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?CLOSE_WIN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置拐角减速
Input:          //卡链接handle 
				iaxis 轴号
				iValue 拐角减速模式
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetCornerMode(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "CORNER_MODE(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取拐角减速
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的拐角模式
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetCornerMode(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?CORNER_MODE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置回零爬行速度
Input:          //卡链接handle  
				iaxis 轴号
				fValue设置的速度值
Output:         //	
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetCreep(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "CREEP(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取回零爬行速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的爬行速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetCreep(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?CREEP(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置原点信号   设定-1为取消原点设置
Input:          //卡链接handle 
				iaxis 轴号
				iValue 设置的原点信号输入口编号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetDatumIn(ZMC_HANDLE handle, int iaxis, int iValue)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "DATUM_IN(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取原点信号
Input:          //卡链接handle  
				iaxis 
Output:         //piValue 返回原点输入口编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetDatumIn(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?DATUM_IN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置减速度
Input:          //卡链接handle
				iaxis 轴号
				fValue 设置的减速度值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetDecel(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "DECEL(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取减速度
Input:          //卡链接handle 
				iaxis 轴号
Output:         //pfValue 设定的减速度返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetDecel(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?DECEL(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置拐角减速角度，开始减速角度，单位为弧度
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的拐角减速角度
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetDecelAngle(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "DECEL_ANGLE(%d)=%.3f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取拐角开始减速角度，单位为弧度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的拐角减速角度
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetDecelAngle(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?DECEL_ANGLE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置轴位置
Input:          //卡链接handle
				iaxis 轴号
				fValue 设置的坐标值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetDpos(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "DPOS(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取轴位置
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的命令位置坐标
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetDpos(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?DPOS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取内部编码器值  （总线绝对值伺服时为绝对值位置）
Input:          //卡链接handle  
				iaxis 轴号		
Output:         //pfValue 返回的内部编码器值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetEncoder(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ENCODER(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前运动的最终位置
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的最终位置
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetEndMove(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ENDMOVE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前和缓冲中运动的最终位置，可以用于相对绝对转换
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的最终位置
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetEndMoveBuffer(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ENDMOVE_BUFFER(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置SP运动的结束速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //fValue 设定的速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetEndMoveSpeed(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "ENDMOVE_SPEED(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取SP运动的结束速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetEndMoveSpeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ENDMOVE_SPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置错误标记，和AXISSTATUS做与运算来决定哪些错误需要关闭WDOG。
Input:          //卡链接handle 
				iaxis 轴号
				iValue 设置值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetErrormask(ZMC_HANDLE handle, int iaxis, int iValue)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "ERRORMASK(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取错误标记，和AXISSTATUS做与运算来决定哪些错误需要关闭WDOG。
Input:          //卡链接handle 
				iaxis 轴号
Output:         //piValue 返回的标记值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetErrormask(ZMC_HANDLE handle, int iaxis , int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?ERRORMASK(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置快速JOG输入
Input:          //卡链接handle  
				iaxis 轴号
				iValue 快速JOG输入口编号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFastJog(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FAST_JOG(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取快速JOG输入
Input:          //卡链接handle  
				iaxis 轴号
Output:         //返回的JOG输入口编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFastJog(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FAST_JOG(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置快速减速度
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设定的快速减速度
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFastDec(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FASTDEC(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取快速减速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的快速减速度
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFastDec(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FASTDEC(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取随动误差
Input:          //卡链接handle 
				iaxis 轴号
Output:         //pfValue 返回的随动误差
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFe(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置最大允许的随动误差值
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的最大误差值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFeLimit(ZMC_HANDLE handle, int iaxis, float fValue)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FE_LIMIT(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取最大允许的随动误差值
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的设置最大误差值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFeLimit(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FE_LIMIT(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置报警时随动误差值
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的误差值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFRange(ZMC_HANDLE handle, int iaxis, float fValue)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FE_RANGE(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取报警时的随动误差值
Input:          //卡链接handle
				iaxis 轴号
Output:         //pfValue 返回的报警误差值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFeRange(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FE_RANGE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //设置保持输入
Input:          //卡链接handle  
				iaxis 轴号
				iValue 设置的输入口编号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFholdIn(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FHOLD_IN(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取保持输入
Input:          //卡链接handle 
				iaxis 轴号
Output:         //piValue 返回输入HOLDIN输入口编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFholdIn(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FHOLD_IN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //设置轴保持速度
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的速度值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFhspeed(ZMC_HANDLE handle, int iaxis, float fValue)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FHSPEED(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取轴保持速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的保持速度
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFhspeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FHSPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置SP运动的运行速度
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的速度值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetForceSpeed(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FORCE_SPEED(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取SP运动的运行速度
Input:          //卡链接handle
				iaxis 轴号
Output:         //pfValue 返回SP运动速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetForceSpeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FORCE_SPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置正向软限位		取消时设置一个较大值即可
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设定的限位值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFsLimit(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FS_LIMIT(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取正向软限位
Input:          //卡链接handle
				iaxis 轴号
Output:         //pfValue 返回的正向限位坐标
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFsLimit(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FS_LIMIT(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置小圆限速最小半径
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的最小半径
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFullSpRadius(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FULL_SP_RADIUS(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取小圆限速最小半径
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的限速半径
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFullSpRadius(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FULL_SP_RADIUS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置正向硬限位输入  设置成-1时表示不设置限位
Input:          //卡链接handle  
				iaxis 轴号
				iValue 设置的限位输入口编号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFwdIn(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FWD_IN(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取正向硬限位输入
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回正向限位输入口编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFwdIn(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FWD_IN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置正向JOG输入
Input:          //卡链接handle  
				iaxis 轴号
				iValue 设置的JOG输入口编号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetFwdJog(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "FWD_JOG(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取正向JOG输入
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的JOG输入口编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetFwdJog(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?FWD_JOG(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取轴是否运动结束
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回运行状态 0-运动中 -1 停止
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetIfIdle(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?IDLE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置脉冲输出模式
Input:          //卡链接handle  
				iaxis 轴号
				iValue 设定的脉冲输出模式 脉冲+方向/双脉冲
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetInvertStep(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "INVERT_STEP(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取脉冲输出模式
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的脉冲模式
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetInvertStep(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?INVERT_STEP(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置插补时轴是否参与速度计算，缺省参与（1）。此参数只对直线和螺旋的第三个轴起作用
Input:          //卡链接handle  
				iaxis	轴号
				iValue 模式 0-不参数 1-参与
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetInterpFactor(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "INTERP_FACTOR(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取插补时轴是否参与速度计算，缺省参与（1）。此参数只对直线和螺旋的第三个轴起作用
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的速度计算模式
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetInterpFactor(ZMC_HANDLE handle, int iaxis, int *piValue)
{
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?INTERP_FACTOR(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置JOG时速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //fValue 设定的速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetJogSpeed(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "JOGSPEED(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取JOG时速度
Input:          //卡链接handle  
				iaxis	轴号
Output:         //pfValue 返回的JOG速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetJogSpeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?JOGSPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前链接运动的参考轴号
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回链接的参考轴号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetLinkax(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?LINK_AXIS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前除了当前运动是否还有缓冲 
Input:          //卡链接handle
				iaxis 轴号
Output:         //piValue 返回状态值  -1 没有剩余函数 0-还有剩余运动
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetLoaded(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?LOADED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置轴起始速度
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的速度值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetLspeed(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "LSPEED(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取轴起始速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的起始速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetLspeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?LSPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置回零反找等待时间
Input:          //卡链接handle 
				iaxis 轴号
				iValue 回零反找等待时间 MS
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetHomeWait(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "HOMEWAIT(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取回零反找等待时间
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的反找等待时间
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetHomeWait(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?HOMEWAIT(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取编码器锁存示教返回状态
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue  返回的锁存触发状态 -1-锁存触发 0-未触发
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMark(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MARK(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取编码器锁存b返回状态
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue  返回的锁存触发状态 -1-锁存触发 0-未触发
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMarkB(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MARKB(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置脉冲输出最高频率
Input:          //卡链接handle  
				iaxis 轴号
				iValue 设置的最高脉冲频率
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetMaxSpeed(ZMC_HANDLE handle, int iaxis, int iValue)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "MAX_SPEED(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取脉冲输出最高频率
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的脉冲频率
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMaxSpeed(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MAX_SPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置连续插补
Input:          //卡链接handle  
				iaxis 轴号
				iValue	连续插补开关 0-关闭连续插补 1-打开连续插补
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetMerge(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "MERGE(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取连续插补状态
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的连续插补开关状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMerge(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MERGE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前被缓冲起来的运动个数
Input:          //卡链接handle  
				iaxis 轴数
Output:         //piValue 缓冲运动数
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMovesBuffered(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MOVES_BUFFERED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前正在运动指令的MOVE_MARK标号
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 当前MARK标号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMoveCurmark(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MOVE_CURMARK(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置运动指令的MOVE_MARK标号 每当有运动进入轴运动缓冲时MARK自动+1
Input:          //卡链接handle  
				iaxis 轴号
				iValue 设定的MARK值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetMovemark(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "MOVE_MARK(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //设置反馈位置
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的反馈位置
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetMpos(ZMC_HANDLE handle, int iaxis, float fValue)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "MPOS(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取反馈位置
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的轴反馈位置坐标
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMpos(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MPOS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取反馈速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的编码器反馈速度
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMspeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MSPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前正在运动指令类型
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回当前的运动类型
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetMtype(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?MTYPE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前正在进行的运动指令后面的第一条指令类型，当插补联动时，对从轴总是返回主轴的运动指令类型
Input:          //卡链接handle  
				iaxis  轴号
Output:         //piValue 返回下一条指令的运动类型
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetNtype(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?NTYPE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置修改偏移位置
Input:          //卡链接handle 
				iaxis 轴号
				fValue 设置的偏移值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetOffpos(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "OFFPOS(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取修改偏移位置
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的偏移坐标值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetOffpos(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?OFFPOS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置锁存触发的结束坐标范围点。
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的坐标值
Output:         // 
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetOpenWin(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "OPEN_WIN(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取锁存触发的结束坐标范围点。
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的结束坐标值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetOpenWin(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?OPEN_WIN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取返回锁存的测量反馈位置(MPOS)
Input:          //卡链接handle 
				iaxis 轴号
Output:         //pfValue 锁存的坐标位置
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRegPos(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REG_POS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取返回锁存的测量反馈位置(MPOS)
Input:          //卡链接handle 
iaxis 轴号
Output:         //pfValue 锁存的坐标位置
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRegPosB(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REG_POSB(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}	

/*************************************************************
Description:    //读取返回轴当前运动还未完成的距离
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的剩余距离
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRemain(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REMAIN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}	

/*************************************************************
Description:    //参数  轴剩余的缓冲, 按直线段来计算
REMAIN_BUFFER为唯一一个可以加AXIS并用ZAux_DirectCommand获取的.
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 剩余的直线缓冲数量
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRemain_LineBuffer(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REMAIN_BUFFER(1) AXIS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //参数  轴剩余的缓冲, 按最复杂的空间圆弧来计算
REMAIN_BUFFER为唯一一个可以加AXIS并用ZAux_DirectCommand获取的.
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 剩余的缓冲数量
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRemain_Buffer(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REMAIN_BUFFER() AXIS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //设置根据REP_OPTION设置来自动循环轴DPOS和MPOS坐标。
Input:          //卡链接handle  
				iaxis 轴号
				fValue	设置的坐标值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetRepDist(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "REP_DIST(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取根据REP_OPTION设置来自动循环轴DPOS和MPOS坐标。
Input:          //卡链接handle 
				iaxis	轴号
Output:         //pfValue 返回的循环坐标值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRepDist(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REP_DIST(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置坐标重复设置
Input:          //卡链接handle  
				iaxis 轴号
				iValue 模式
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetRepOption(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "REP_OPTION(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取坐标重复设置
Input:          //卡链接handle
				iaxis 轴号
Output:         //piValue 返回的模式
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRepOption(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REP_OPTION(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置负向硬件限位开关对应的输入点编号，-1无效。
Input:          //卡链接handle  
				iaxis  轴号
				iValue 设置的输入口编号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetRevIn(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "Rev_In(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取负向硬件限位开关对应的输入点编号，-1无效。
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的负向限位输入口编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRevIn(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REV_IN(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置负向JOG输入对应的输入点编号，-1无效。
Input:          //卡链接handle  
				iaxis 轴号
				iValue 设置的输入口编号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetRevJog(ZMC_HANDLE handle, int iaxis, int iValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "REV_JOG(%d)=%d", iaxis, iValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取负向JOG输入对应的输入点编号，-1无效。
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的输入口编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRevJog(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?REV_JOG(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置负向软限位位置。  设置一个较大的值时认为取消限位
Input:          //卡链接handle  
				iaxis	轴号
				fValue  负向限位值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetRsLimit(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "RS_LIMIT(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取负向软限位位置。
Input:          //卡链接handle  
				iaxis	轴号
Output:         //pfValue 设定的限位值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetRsLimit(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?RS_LIMIT(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置轴速度，单位为units/s，当多轴运动时，作为插补运动的速度
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的速度值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetSpeed(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "SPEED(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取轴速度，单位为units/s，当多轴运动时，作为插补运动的速度
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetSpeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?Speed(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置 S曲线设置。 0-梯形加减速
Input:          //卡链接handle  
				iaxis 轴号
				fValue S曲线平滑时间MS
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetSramp(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "SRAMP(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取 S曲线设置。
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 平滑时间
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetSramp(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?SRAMP(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置 自定义速度的SP运动的起始速度
Input:          //卡链接handle 
				iaxis 轴号
				fValue 设置的速度值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetStartMoveSpeed(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "STARTMOVE_SPEED(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取自定义速度的SP运动的起始速度
Input:          //卡链接handle  
				iaxis	轴号
Output:         //pfValue 返回的SP运动起始速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetStartMoveSpeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?STARTMOVE_SPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //设置 减速到最低的最小拐角 弧度制
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的角度值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetStopAngle(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "STOP_ANGLE(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取减速到最低的最小拐角 弧度制
Input:          //卡链接handle  
				iaxis 轴号
Output:         //pfValue 返回的拐角停止角度
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetStopAngle(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?STOP_ANGLE(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //设置 减速倒角半径
Input:          //卡链接handle  
				iaxis	轴号
				fValue	倒角半径
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetZsmooth(ZMC_HANDLE handle, int iaxis, float fValue)
{


	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "ZSMOOTH(%d)=%f", iaxis, fValue);

	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取倒角半径
Input:          //卡链接handle  
				iaxis	轴号
Output:         //pfValue	返回的倒角半径值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetZsmooth(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "?ZSMOOTH(%d)", iaxis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}

	//
	sscanf(cmdbuffAck, "%f",pfValue);

	return ERR_OK;
}

/*************************************************************
Description:    //设置 脉冲当量
Input:          //卡链接handle  
				iaxis 轴号
				fValue 设置的当量值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetUnits(ZMC_HANDLE handle, int iaxis, float fValue)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "UNITS(%d)=%f", iaxis, fValue);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取脉冲当量
Input:          //卡链接handle
				iaxis	轴号
Output:         //pfValue 返回的脉冲当量
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetUnits(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?UNITS(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //读取返回轴当前当前运动和缓冲运动还未完成的距离
Input:          //卡链接handle
				iaxis 轴号
Output:         //pfValue 返回的阵雨距离
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetVectorBuffered(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?VECTOR_BUFFERED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //读取当前轴运行的命令速度
Input:          //卡链接handle  
				iaxis	轴号
Output:         //pfValue	返回的当前速度值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetVpSpeed(ZMC_HANDLE handle, int iaxis, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?VP_SPEED(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}


/*************************************************************
Description:    //全局变量读取, 也可以是参数等等
Input:          //卡链接handle  
				pname 全局变量名称/或者指定轴号的轴参数名称DPOS(0)
Output:         //pfValue 返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetVariablef(ZMC_HANDLE handle, const char *pname, float *pfValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?%s", pname);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%f",pfValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //全局变量读取, 也可以是参数等等
Input:          //卡链接handle  
				pname 全局变量名称/或者指定轴号的轴参数名称DPOS(0)
Output:         //piValue 返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetVariableInt(ZMC_HANDLE handle, const char *pname, int *piValue)
{

	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?%s", pname);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}


///////////////////////  只有下面的运动函数支持直接调用，并不是所有的指令都支持
///////////////////////  必须 20130901 以后的控制器版本支持

/*************************************************************
Description:    //BASE指令调用
仅仅修改在线命令的BASE列表，不对控制器的运行任务的BASE进行修改.
修改后，后续的所有MOVE等指令都是以这个BASE为基础  
Input:          //卡链接handle
				imaxaxises 参与轴数
				piAxislist	轴列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Base(ZMC_HANDLE handle, int imaxaxises, int *piAxislist)
{
	int i;
	char  cmdbuffAck[2048];	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	
	//
	
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}




/*************************************************************
Description:    //定义DPOS,不建议使用，可以直接调用SETDPOS达到同样效果
Input:          //卡链接handle
				iaxis	轴号
				pfDpos 设置的坐标值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Defpos(ZMC_HANDLE handle, int iaxis, float pfDpos)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	//
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	sprintf(cmdbuff, "DEFPOS(%f) AXIS(%d)",pfDpos,iaxis);

	//调用命令执行函数
	return ZAux_Execute(handle, cmdbuff,  cmdbuffAck,2048);
	//return   ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
}

/*************************************************************
Description:    //多轴相对直线插补  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				pfDisancelist		距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Move(ZMC_HANDLE handle, int imaxaxises, int *piAxislist,float *pfDisancelist)
{
	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
	//
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
	
    //生成命令
	strcat(cmdbuff, "MOVE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%f,",pfDisancelist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%f)",pfDisancelist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //相对多轴直线插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				pfDisancelist		距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float *pfDisancelist)
{
	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];		
	//
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
	
    //生成命令
	strcat(cmdbuff, "MOVESP(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%f,",pfDisancelist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%f)",pfDisancelist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //绝对多轴直线插补  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				pfDisancelist		距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveAbs(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float *pfDisancelist)
{
	int i;

	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];		
	//
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
	
    //生成命令
	strcat(cmdbuff, "MOVEABS(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%f,",pfDisancelist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%f)",pfDisancelist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对多轴直线插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				pfDisancelist		距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveAbsSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float *pfDisancelist)
{
	int i;

	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];		
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
		
    //生成命令
	strcat(cmdbuff, "MOVEABSSP(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%f,",pfDisancelist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%f)",pfDisancelist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
//Description:    //运动中修改结束位置  20130901 以后的控制器版本支持
//Input:          //卡链接handle
					轴号 iaxis
					绝对距离 pfDisance
//Output:         //
//Return:         //错误码
/*************************************************************/
int32  ZAux_Direct_MoveModify(ZMC_HANDLE handle, int iaxis, float pfDisance)
{

	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];		
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

    //生成命令
	
	sprintf(cmdbuff, "MOVEMODIFY(%f) AXIS(%d)",pfDisance,iaxis);

	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //相对圆心定圆弧插补运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标
				end2              第二个轴运动坐标
				centre1    第一个轴运动圆心，相对与起始点。
				centre2    第二个轴运动圆心，相对与起始点。
				direction  0-逆时针，1-顺时针
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCirc(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection)
{
	int i;
	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
		
    //生成命令
	sprintf(tempbuff, "MOVECIRC(%f,%f,%f,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //相对圆心定圆弧 插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标
				end2              第二个轴运动坐标
				centre1    第一个轴运动圆心，相对与起始点。
				centre2    第二个轴运动圆心，相对与起始点。
				direction  0-逆时针，1-顺时针
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCircSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection)
{

	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
			
    //生成命令
	sprintf(tempbuff, "MOVECIRCSP(%f,%f,%f,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //绝对圆心圆弧插补运动  20130901 以后的控制器版本支持  无法画整圆
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标，绝对位置
				end2              第二个轴运动坐标，绝对位置
				centre1    第一个轴运动圆心，绝对位置
				centre2    第二个轴运动圆心，绝对位置
				direction  0-逆时针，1-顺时针
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCircAbs(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection)
{

	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
	
    //生成命令
	sprintf(tempbuff, "MOVECIRCABS(%f,%f,%f,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对圆心圆弧插补SP运动  20130901 以后的控制器版本支持  无法画整圆
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标，绝对位置
				end2              第二个轴运动坐标，绝对位置
				centre1    第一个轴运动圆心，绝对位置
				centre2    第二个轴运动圆心，绝对位置
				direction  0-逆时针，1-顺时针
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCircAbsSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection)
{
	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
	
    //生成命令
	sprintf(tempbuff, "MOVECIRCABSSP(%f,%f,%f,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}



/*************************************************************
Description:    //相对3点定圆弧插补运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				mid1       第一个轴中间点，相对起始点距离
				mid2       第二个轴中间点，相对起始点距离
				end1              第一个轴结束点，相对起始点距离
				end2              第二个轴结束点，相对起始点距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCirc2(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2)
{

	int i ;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
		
    //生成命令
	sprintf(tempbuff, "MOVECIRC2(%f,%f,%f,%f)", fmid1,fmid2,fend1,fend2);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //绝对3点定圆弧插补运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				mid1       第一个轴中间点，绝对位置
				mid2       第二个轴中间点，绝对位置
				end1              第一个轴结束点，绝对位置
				end2              第二个轴结束点，绝对位置 
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCirc2Abs(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2)
{

	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
			
    //生成命令
	sprintf(tempbuff, "MOVECIRC2ABS(%f,%f,%f,%f)", fmid1,fmid2,fend1,fend2);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //相对3点定圆弧插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				mid1       第一个轴中间点，相对起始点距离
				mid2       第二个轴中间点，相对起始点距离
				end1              第一个轴结束点，相对起始点距离
				end2              第二个轴结束点，相对起始点距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCirc2Sp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MOVECIRC2SP(%f,%f,%f,%f)", fmid1,fmid2,fend1,fend2);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //绝对3点定圆弧插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				mid1       第一个轴中间点，绝对位置
				mid2       第二个轴中间点，绝对位置
				end1              第一个轴结束点，绝对位置
				end2              第二个轴结束点，绝对位置 
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveCirc2AbsSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
			
    //生成命令
	sprintf(tempbuff, "MOVECIRC2ABSSP(%f,%f,%f,%f)", fmid1,fmid2,fend1,fend2);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}



/*************************************************************
Description:    //相对3轴圆心螺旋插补运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标
				end2              第二个轴运动坐标
				centre1    第一个轴运动圆心，相对与起始点
				centre2    第二个轴运动圆心，相对与起始点		
				direction  0-逆时针，1-顺时针
				distance3第三个轴运动距离。
				mode      第三轴的速度计算:0(缺省)第三轴参与速度计算。1第三轴不参与速度计算。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelical(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MHELICAL(%f,%f,%f,%f,%d,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection, fDistance3, imode);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对3轴圆心螺旋插补运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标
				end2              第二个轴运动坐标
				centre1    第一个轴运动圆心坐标
				centre2    第二个轴运动圆心坐标
				direction  0-逆时针，1-顺时针
				distance3第三个轴运动距离。
				mode      第三轴的速度计算:0(缺省) 第三轴参与速度计算。1第三轴不参与速度计算。
Output:         //
 Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelicalAbs(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
		
    //生成命令
	sprintf(tempbuff, "MHELICALABS(%f,%f,%f,%f,%d,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection, fDistance3, imode);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	//return ZMC_ExecuteNoAck(handle, cmdbuff, g_ZMC_MaxExcuteWaitms);
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //相对3轴圆心螺旋插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标
				end2              第二个轴运动坐标
				centre1    第一个轴运动圆心，相对与起始点	  
				centre2    第二个轴运动圆心，相对与起始点		
				direction  0-逆时针，1-顺时针  
				distance3第三个轴运动距离。
			  mode      第三轴的速度计算: 0(缺省)第三轴参与速度计算。 1第三轴不参与速度计算。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelicalSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
			
    //生成命令
	sprintf(tempbuff, "MHELICALSP(%f,%f,%f,%f,%d,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection, fDistance3, imode);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对3轴圆心螺旋插补SP运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第一个轴运动坐标
				end2              第二个轴运动坐标
				centre1    第一个轴运动圆心坐标
				centre2    第二个轴运动圆心坐标
				direction  0-逆时针，1-顺时针
				distance3第三个轴运动距离。
				mode      第三轴的速度计算:0(缺省) 第三轴参与速度计算。1第三轴不参与速度计算。
Output:         //
 Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelicalAbsSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MHELICALABSSP(%f,%f,%f,%f,%d,%f,%d)", fend1,fend2,fcenter1,fcenter2,idirection, fDistance3, imode);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //相对3轴 3点画螺旋插补运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				mid1       第一个轴中间点  
				mid2       第二个轴中间点	
				end1              第一个轴结束点	  
				end2              第二个轴结束点		
				distance3第三个轴运动距离		  
				mode      第三轴的速度计算:			
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelical2(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
					
    //生成命令
	sprintf(tempbuff, "MHELICAL2(%f,%f,%f,%f,%f,%d)", fmid1,fmid2,fend1,fend2, fDistance3, imode);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对3轴 3点画螺旋插补运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
			imaxaxises			参与运动总轴数
			piAxislist			轴号列表
			mid1       第一个轴中间点
			mid2       第二个轴中间点
			end1              第一个轴结束点
			end2              第二个轴结束点
			distance3   第三个轴运动结束点
			mode      第三轴的速度计算:
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelical2Abs(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MHELICAL2ABS(%f,%f,%f,%f,%f,%d)", fmid1,fmid2,fend1,fend2, fDistance3, imode);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //相对3轴 3点画螺旋插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				mid1       第一个轴中间点  
				mid2       第二个轴中间点	
				end1              第一个轴结束点	  
				end2              第二个轴结束点		
				distance3第三个轴运动距离		  
				mode      第三轴的速度计算:			
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelical2Sp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
					
    //生成命令
	sprintf(tempbuff, "MHELICAL2SP(%f,%f,%f,%f,%f,%d)", fmid1,fmid2,fend1,fend2, fDistance3, imode);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对3轴 3点画螺旋插补SP运动  20130901 以后的控制器版本支持
Input:          //卡链接handle
			imaxaxises			参与运动总轴数
			piAxislist			轴号列表
			mid1       第一个轴中间点
			mid2       第二个轴中间点
			end1              第一个轴结束点
			end2              第二个轴结束点
			distance3   第三个轴运动结束点
			mode      第三轴的速度计算:
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MHelical2AbsSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fmid1, float fmid2, float fend1, float fend2, float fDistance3, int imode)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
						
    //生成命令
	sprintf(tempbuff, "MHELICAL2ABSSP(%f,%f,%f,%f,%f,%d)", fmid1,fmid2,fend1,fend2, fDistance3, imode);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}



/*************************************************************
Description:    //相对椭圆插补 20130901 以后的控制器版本支持
Input:          //卡链接handle
			imaxaxises			参与运动总轴数
			piAxislist			轴号列表
			fend1              终点第一个轴运动坐标，相对于起始点。
			fend2              终点第二个轴运动坐标，相对于起始点。
			fcenter1    中心第一个轴运动坐标，相对于起始点。
			fcenter2    中心第二个轴运动坐标，相对于起始点。
			idirection  0-逆时针，1-顺时针
			fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
			fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipse(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
						
    //生成命令
	sprintf(tempbuff, "MECLIPSE(%f,%f,%f,%f,%d,%f,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对椭圆插补 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				fend1              终点第一个轴运动坐标
				fend2              终点第二个轴运动坐标
				fcenter1    中心第一个轴运动坐标。
				fcenter2    中心第二个轴运动坐标。
				idirection  0-逆时针，1-顺时针
				fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
				fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。

  Output:         //
  Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipseAbs(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MECLIPSEABS(%f,%f,%f,%f,%d,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //相对椭圆插补SP运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
			imaxaxises			参与运动总轴数
			piAxislist			轴号列表
			fend1              终点第一个轴运动坐标，相对于起始点。
			fend2              终点第二个轴运动坐标，相对于起始点。
			fcenter1    中心第一个轴运动坐标，相对于起始点。
			fcenter2    中心第二个轴运动坐标，相对于起始点。
			idirection  0-逆时针，1-顺时针
			fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
			fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipseSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis)
{
	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
					
    //生成命令
	sprintf(tempbuff, "MECLIPSESP(%f,%f,%f,%f,%d,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对椭圆插补SP运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				fend1              终点第一个轴运动坐标
				fend2              终点第二个轴运动坐标
				fcenter1    中心第一个轴运动坐标。
				fcenter2    中心第二个轴运动坐标。
				idirection  0-逆时针，1-顺时针
				fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
				fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。

  Output:         //
  Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipseAbsSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
						
    //生成命令
	sprintf(tempbuff, "MECLIPSEABSSP(%f,%f,%f,%f,%d,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //相对 椭圆 + 螺旋插补运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				fend1              终点第一个轴运动坐标，相对于起始点。
				fend2              终点第二个轴运动坐标，相对于起始点。
				fcenter1    中心第一个轴运动坐标，相对于起始点。
				fcenter2    中心第二个轴运动坐标，相对于起始点。
				idirection  0-逆时针，1-顺时针
				fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
				fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。
				fDistance3	第三个轴的运动距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipseHelical(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis, float fDistance3)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
						
    //生成命令
	sprintf(tempbuff, "MECLIPSE(%f,%f,%f,%f,%d,%f,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis, fDistance3);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对椭圆 + 螺旋插补运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				fend1              终点第一个轴运动坐标
				fend2              终点第二个轴运动坐标
				fcenter1    中心第一个轴运动坐标。
				fcenter2    中心第二个轴运动坐标。
				idirection  0-逆时针，1-顺时针
				fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
				fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。
				fDistance3	第三个轴的运动距离
  Output:         //
  Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipseHelicalAbs(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis,float fDistance3)
{
	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
	//生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MECLIPSEABS(%f,%f,%f,%f,%d,%f,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis,fDistance3);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //相对 椭圆 + 螺旋插补SP运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				fend1              终点第一个轴运动坐标，相对于起始点。
				fend2              终点第二个轴运动坐标，相对于起始点。
				fcenter1    中心第一个轴运动坐标，相对于起始点。
				fcenter2    中心第二个轴运动坐标，相对于起始点。
				idirection  0-逆时针，1-顺时针
				fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
				fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。
				fDistance3	第三个轴的运动距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipseHelicalSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis, float fDistance3)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MECLIPSESP(%f,%f,%f,%f,%d,%f,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis, fDistance3);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //绝对椭圆 + 螺旋插补SP运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				fend1              终点第一个轴运动坐标
				fend2              终点第二个轴运动坐标
				fcenter1    中心第一个轴运动坐标。
				fcenter2    中心第二个轴运动坐标。
				idirection  0-逆时针，1-顺时针
				fADis         第一轴的椭圆半径，半长轴或者半短轴都可。
				fBDis        第二轴的椭圆半径，半长轴或者半短轴都可，AB相等时自动为圆弧或螺旋。
				fDistance3	第三个轴的运动距离
  Output:         //
  Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MEclipseHelicalAbsSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fcenter1, float fcenter2, int idirection, float fADis, float fBDis,float fDistance3)
{
	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MECLIPSEABSSP(%f,%f,%f,%f,%d,%f,%f,%f)", fend1,fend2,fcenter1,fcenter2,idirection, fADis, fBDis,fDistance3);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //空间圆弧 + 螺旋插补运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第1个轴运动距离参数1	相对与起点
				end2              第2个轴运动距离参数1	相对与起点  
				end3              第3个轴运动距离参数1	相对与起点
				centre1    第1个轴运动距离参数2	相对与起点
				centre2    第2个轴运动距离参数2	相对与起点
				centre3    第3个轴运动距离参数2 相对与起点
				mode      指定前面参数的意义
					0 当前点，中间点，终点三点定圆弧，距离参数1为终点距离，距离参数2为中间点距离。
					1 走最小的圆弧，距离参数1为终点距离，距离参数2为圆心的距离。
					2 当前点，中间点，终点三点定圆，距离参数1为终点距离，距离参数2为中间点距离。
					3 先走最小的圆弧，再继续走完整圆，距离参数1为终点距离，距离参数2为圆心的距离。
				fcenter4	第4个轴运动距离参数 
				fcenter5	第5个轴运动距离参数
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MSpherical(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fend3, float fcenter1, float fcenter2, float fcenter3, int imode, float fcenter4, float fcenter5)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MSPHERICAL(%f,%f,%f,%f,%f,%f,%d,%f,%f)", fend1,fend2,fend3,fcenter1,fcenter2,fcenter3,imode,fcenter4,fcenter5);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //空间圆弧 + 螺旋 插补SP运动 20130901 以后的控制器版本支持
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第1个轴运动距离参数1	相对与起点
				end2              第2个轴运动距离参数1	相对与起点  
				end3              第3个轴运动距离参数1	相对与起点
				centre1    第1个轴运动距离参数2	相对与起点
				centre2    第2个轴运动距离参数2	相对与起点
				centre3    第3个轴运动距离参数2 相对与起点
				mode      指定前面参数的意义
					0 当前点，中间点，终点三点定圆弧，距离参数1为终点距离，距离参数2为中间点距离。
					1 走最小的圆弧，距离参数1为终点距离，距离参数2为圆心的距离。
					2 当前点，中间点，终点三点定圆，距离参数1为终点距离，距离参数2为中间点距离。
					3 先走最小的圆弧，再继续走完整圆，距离参数1为终点距离，距离参数2为圆心的距离。
				fcenter4	第4个轴运动距离参数 
				fcenter5	第5个轴运动距离参数
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MSphericalSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float fend1, float fend2, float fend3, float fcenter1, float fcenter2, float fcenter3, int imode, float fcenter4, float fcenter5)
{
	int i;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MSPHERICALSP(%f,%f,%f,%f,%f,%f,%d,%f,%f)", fend1,fend2,fend3,fcenter1,fcenter2,fcenter3,imode, fcenter4, fcenter5);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //渐开线圆弧插补运动，相对移动方式，当起始半径0直接扩散时从0角度开始
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				centre1: 第1轴圆心的相对距离
				centre2: 第2轴圆心的相对距离
				circles:  要旋转的圈数，可以为小数圈，负数表示顺时针.
				pitch:   每圈的扩散距离，可以为负。
				distance3        第3轴螺旋的功能，指定第3轴的相对距离，此轴不参与速度计算。
				distance4        第4轴螺旋的功能，指定第4轴的相对距离，此轴不参与速度计算。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveSpiral(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float centre1, float centre2, float circles, float pitch, float distance3, float distance4)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
				
    //生成命令
	sprintf(tempbuff, "MOVESPIRAL(%f,%f,%f,%f,%f,%f)", centre1,centre2,circles,pitch,distance3,distance4);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //渐开线圆弧插补SP运动，相对移动方式，当起始半径0直接扩散时从0角度开始
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				centre1: 第1轴圆心的相对距离
				centre2: 第2轴圆心的相对距离
				circles:  要旋转的圈数，可以为小数圈，负数表示顺时针.
				pitch:   每圈的扩散距离，可以为负。
				distance3        第3轴螺旋的功能，指定第3轴的相对距离，此轴不参与速度计算。
				distance4        第4轴螺旋的功能，指定第4轴的相对距离，此轴不参与速度计算。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveSpiralSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float centre1, float centre2, float circles, float pitch, float distance3, float distance4)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
					
    //生成命令
	sprintf(tempbuff, "MOVESPIRALSP(%f,%f,%f,%f,%f,%f)", centre1,centre2,circles,pitch,distance3,distance4);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //空间直线插补运动，根据下一个直线运动的绝对坐标在拐角自动插入圆弧，加入圆弧后会使得运动的终点与直线的终点不一致，拐角过大时不会插入圆弧，当距离不够时会自动减小半径
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第1个轴运动绝对坐标
				end2              第2个轴运动绝对坐标
				end3              第3个轴运动绝对坐标
				next1      第1个轴下一个直线运动绝对坐标
				next2      第2个轴下一个直线运动绝对坐标
				next3      第3个轴下一个直线运动绝对坐标
				radius      插入圆弧的半径，当过大的时候自动缩小。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveSmooth(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float end1, float end2, float end3, float next1, float next2, float next3, float radius)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
					
    //生成命令
	sprintf(tempbuff, "MOVESMOOTH(%f,%f,%f,%f,%f,%f,%f)", end1,end2,end3,next1,next2,next3,radius);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck, 2048);
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //空间直线插补SP运动，根据下一个直线运动的绝对坐标在拐角自动插入圆弧，加入圆弧后会使得运动的终点与直线的终点不一致，拐角过大时不会插入圆弧，当距离不够时会自动减小半径
Input:          //卡链接handle
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				end1              第1个轴运动绝对坐标
				end2              第2个轴运动绝对坐标
				end3              第3个轴运动绝对坐标
				next1      第1个轴下一个直线运动绝对坐标
				next2      第2个轴下一个直线运动绝对坐标
				next3      第3个轴下一个直线运动绝对坐标
				radius      插入圆弧的半径，当过大的时候自动缩小。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveSmoothSp(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, float end1, float end2, float end3, float next1, float next2, float next3, float radius)
{

	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
					
    //生成命令
	sprintf(tempbuff, "MOVESMOOTHSP(%f,%f,%f,%f,%f,%f,%f)", end1,end2,end3,next1,next2,next3,radius);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck, 2048);
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //运动暂停		，插补运动暂停主轴。轴列表轴第一个轴
Input:          //卡链接handle  
				轴号 iaxis
				模式 imode	0（缺省） 暂停当前运动。 
							1 在当前运动完成后正准备执行下一条运动指令时暂停。 
							2 在当前运动完成后正准备执行下一条运动指令时，并且两条指令的MARK标识不一样时暂停。这个模式可以用于一个动作由多个指令来实现时，可以在一整个动作完成后暂停。 
  
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MovePause(ZMC_HANDLE handle,int iaxis, int imode)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "MOVE_PAUSE(%d) axis(%d)", imode,iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //取消运动暂停
Input:          //卡链接handle  
					轴号 iaxis
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveResume(ZMC_HANDLE handle,int iaxis)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "MOVE_RESUME AXIS(%d)",iaxis);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //在当前的运动末尾位置增加速度限制，用于强制拐角减速
Input:          //卡链接handle  
				 轴号 iaxis
Output:         //
 Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveLimit(ZMC_HANDLE handle, int iaxis,float limitspeed)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "MOVELIMIT(%f) AXIS(%d)", limitspeed,iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //在运动缓冲中加入输出指令
Input:          //卡链接handle  
				轴号 iaxis
				输出口编号 ioutnum
				输出口状态	ivalue
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveOp(ZMC_HANDLE handle, int iaxis,int ioutnum, int ivalue)
{

	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
   //生成命令
	sprintf(cmdbuff, "BASE(%d)\r\n",iaxis);
				
    //生成命令
	sprintf(tempbuff, "MOVE_OP(%d,%d)", ioutnum, ivalue);
    strcat(cmdbuff, tempbuff);
		
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //在运动缓冲中加入连续输出口输出指令
Input:          //卡链接handle  
				轴号 iaxis
				输出口起始编号 ioutnumfirst
				输出口结束编号 ioutnumend
				对应输出口状态二进制组合值	ivalue
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveOpMulti(ZMC_HANDLE handle, int iaxis, int ioutnumfirst, int ioutnumend, int ivalue)
{

	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "BASE(%d)\r\n",iaxis);
		
    //生成命令
	sprintf(tempbuff, "MOVE_OP(%d,%d,%d)", ioutnumfirst, ioutnumend, ivalue);
    strcat(cmdbuff, tempbuff);	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //在运动缓冲中加入输出指令 ,指定时间后输出状态翻转
Input:          //卡链接handle  
				轴号 iaxis
				输出口编号 ioutnum
				输出口状态	ivalue
				状态反转时间 iofftimems
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveOp2(ZMC_HANDLE handle, int iaxis, int ioutnum, int ivalue, int iofftimems)
{

	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "BASE(%d)\r\n",iaxis);
			
    //生成命令
	sprintf(tempbuff, "MOVE_OP2(%d,%d,%d)", ioutnum, ivalue, iofftimems);
    strcat(cmdbuff, tempbuff);		
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //在运动缓冲中加入AOUT输出指令
Input:          //卡链接handle  
				轴号 iaxis
				DA口编号 ioutnum
				模拟量值 fvalue （4系列以下 12位0-4095）
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveAout(ZMC_HANDLE handle, int iaxis, int ioutnum, float fvalue)
{

	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "BASE(%d)\r\n",iaxis);
				
    //生成命令
	sprintf(tempbuff, "MOVE_AOUT(%d,%f)", ioutnum, fvalue);
    strcat(cmdbuff, tempbuff);			
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //在运动缓冲中加入延时指令
Input:          //卡链接handle  
				轴号 iaxis
				延时时间 itimems 毫秒
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveDelay(ZMC_HANDLE handle, int iaxis, int itimems)
{

	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "BASE(%d)\r\n",iaxis);
				
    //生成命令
	sprintf(tempbuff, "MOVE_WA(%d)", itimems);
    strcat(cmdbuff, tempbuff);		
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //旋转台直线插补运动。  20130901 以后的控制器版本支持
Input:          //卡链接handle
				tablenum			存储旋转台参数的table编号 
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				pfDisancelist		距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveTurnabs(ZMC_HANDLE handle, int tablenum ,int imaxaxises,int *piAxislist, float *pfDisancelist)
{
	int i;
	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
	//
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
	
	sprintf(tempbuff, "MOVE_TURNABS(%d,",tablenum);
	strcat(cmdbuff, tempbuff);
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%f,",pfDisancelist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%f)",pfDisancelist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //旋转台圆弧+螺旋插补运动。  20130901 以后的控制器版本支持
Input:          //卡链接handle
				tablenum       存储旋转参数的table编号
				refpos1    第一个轴参考点，绝对位置
				refpos2    第二个轴参考点，绝对位置
				mode      1-参考点是当前点前面，2-参考点是结束点后面，3-参考点在中间，采用三点定圆的方式。
				end1              第一个轴结束点，绝对位置
				end2              第二个轴结束点，绝对位置
				imaxaxises        参与运动轴数量
				piAxislist		  轴列表
				pfDisancelist	螺旋轴距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_McircTurnabs(ZMC_HANDLE handle, int tablenum ,float refpos1,float refpos2,int mode,float end1,float end2,int imaxaxises,int *piAxislist ,float *pfDisancelist)
{
	int i;
	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
	//
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
  //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
		
    //生成命令
	sprintf(tempbuff, "MCIRC_TURNABS(%d,%f,%f,%d,%f,%f,",tablenum,refpos1,refpos2,mode,end1,end2);
	strcat(cmdbuff, tempbuff);
	
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%f,",pfDisancelist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%f)",pfDisancelist[imaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //电子凸轮 同步运动
Input:          //卡链接handle  
				iaxis			轴号
				istartpoint		起始点TABLE编号
				iendpoint		结束点TABLE编号
				ftablemulti		位置比例，一般设为脉冲当量值
				fDistance		参考运动的距离，用来计算总运动时间
 Output:         //
 Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Cam(ZMC_HANDLE handle,int iaxis, int istartpoint, int iendpoint, float ftablemulti, float fDistance)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "CAM(%d,%d,%f,%f) AXIS(%d)", istartpoint,iendpoint,ftablemulti,fDistance,iaxis);

	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //电子凸轮 同步运动
Input:          //卡链接handle  
				iaxis			轴号
				istartpoint		起始点TABLE编号
				iendpoint		结束点TABLE编号
				ftablemulti		位置比例，一般设为脉冲当量值
				fDistance		参考运动的距离
				ilinkaxis		参考主轴
				ioption			参考轴的连接方式
				flinkstartpos	ioption条件中距离参数
 Output:         //
 Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Cambox(ZMC_HANDLE handle, int iaxis,int istartpoint, int iendpoint, float ftablemulti, float fDistance, int ilinkaxis, int ioption, float flinkstartpos)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "CAMBOX(%d,%d,%f,%f,%d,%d,%f) AXIS(%d)", istartpoint,iendpoint,ftablemulti,fDistance, ilinkaxis, ioption, flinkstartpos,iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //特殊凸轮 同步运动
Input:          //卡链接handle  
				iaxis			参与运动的轴号(跟随轴)
				fDistance		同步过程跟随轴运动距离
				fLinkDis		同步过程参考轴(主轴)运动绝对距离
				fLinkAcc		跟随轴加速阶段，参考轴移动的绝对距离
				fLinkDec		跟随轴减速阶段，参考轴移动的绝对距离
				iLinkaxis		参考轴的轴号
				ioption			连接模式选项
				flinkstartpos	连接模式选项中运动距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Movelink(ZMC_HANDLE handle, int iaxis,float fDistance, float fLinkDis, float fLinkAcc, float fLinkDec,int iLinkaxis, int ioption, float flinkstartpos)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "MOVELINK(%f,%f,%f,%f,%d,%d,%f) AXIS(%d)", fDistance, fLinkDis, fLinkAcc,fLinkDec,iLinkaxis,ioption,flinkstartpos,iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //特殊凸轮 同步运动
Input:          //卡链接handle  
				iaxis			参与运动的轴号(跟随轴)
				fDistance		同步过程跟随轴运动距离
				fLinkDis		同步过程参考轴(主轴)运动绝对距离
				startsp			启动时跟随轴和参考轴的速度比例，units/units单位，负数表示跟随轴负向运动
				endsp			结束时跟随轴和参考轴的速度比例，units/units单位, 负数表示跟随轴负向运动。
				iLinkaxis		参考轴的轴号
				ioption			连接模式选项
				flinkstartpos	连接模式选项中运动距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Moveslink(ZMC_HANDLE handle, int iaxis,float fDistance, float fLinkDis, float startsp, float endsp,int iLinkaxis, int ioption, float flinkstartpos)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "MOVESLINK(%f,%f,%f,%f,%d,%d,%f) AXIS(%d)", fDistance, fLinkDis, startsp,endsp,iLinkaxis,ioption,flinkstartpos,iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //连接 同步运动指令 电子齿轮
Input:          //卡链接handle  
				ratio		比率，可正可负，注意是脉冲个数的比例。	
				link_axis	连接轴的轴号，手轮时为编码器轴
				move_axis	随动轴号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Connect(ZMC_HANDLE handle, float ratio, int link_axis,int move_axis)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "CONNECT(%f,%d) AXIS(%d)", ratio, link_axis,move_axis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //连接 同步运动指令 电子齿轮 将当前轴的目标位置与link_axis轴的插补矢量长度通过电子齿轮连接
Input:          //卡链接handle  
				ratio		比率，可正可负，注意是脉冲个数的比例。	
				link_axis	连接轴的轴号，手轮时为编码器轴
				move_axis	随动轴号
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Connpath(ZMC_HANDLE handle, float ratio, int link_axis,int move_axis)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "CONNPATH(%f,%d) AXIS(%d)", ratio, link_axis,move_axis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //位置锁存指令
Input:          //卡链接handle  
				iaxis	轴号
				imode	锁存模式
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Regist(ZMC_HANDLE handle,int iaxis, int imode)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "REGIST(%d) AXIS(%d)", imode,iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //编码器输入齿轮比，缺省(1,1)
Input:          //卡链接handle  
				iaxis		轴号
				mpos_count	分子，不要超过65535
				input_count	 分母，不要超过65535
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_EncoderRatio(ZMC_HANDLE handle, int iaxis,int mpos_count,int input_count)
{

	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "BASE(%d)\r\n",iaxis);
					
    //生成命令
	sprintf(tempbuff, "ENCODER_RATIO(%d,%d)", mpos_count,input_count);
    strcat(cmdbuff, tempbuff);	
	
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //设置步进输出齿轮比，缺省(1,1)
Input:          //卡链接handle  
				iaxis		轴号
				mpos_count	分子，1-65535
				input_count	 分母，1-65535
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_StepRatio(ZMC_HANDLE handle, int iaxis, int mpos_count,int input_count)
{

	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "BASE(%d)\r\n",iaxis);	
    //生成命令
	sprintf(tempbuff, "STEP_RATIO(%d,%d)", mpos_count,input_count);
    strcat(cmdbuff, tempbuff);		
	
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //所有轴立即停止
Input:          //卡链接handle 
				imode 停止模式
				0（缺省）取消当前运动
				1	取消缓冲的运动
				2	取消当前运动和缓冲运动。
				3	立即中断脉冲发送。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Rapidstop(ZMC_HANDLE handle, int imode)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
    //生成命令
	sprintf(cmdbuff, "RAPIDSTOP(%d)", imode);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //多个轴运动停止
Input:          //卡链接handle  轴号， 距离
				imaxaxises		轴数
				piAxislist		轴列表
				imode	模式
					0（缺省）取消当前运动
					1	取消缓冲的运动
					2	取消当前运动和缓冲运动。
					3	立即中断脉冲发送。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_CancelAxisList(ZMC_HANDLE handle, int imaxaxises, int *piAxislist, int imode)
{
	int i = 0;
	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	sprintf(cmdbuff, "CANCEL(%d) AXIS(%d)\n", imode, piAxislist[i]);

	for(i = 1; i< imaxaxises; i++)
	{
		//
		sprintf(tempbuff, "CANCEL(%d) AXIS(%d)\n", imode, piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}

	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //CONNFRAME机械手逆解指令	2系列以上控制器支持
Input:          //卡链接handle
				Jogmaxaxises	关节轴数量
				JogAxislist		关节轴列表
				frame			机械手类型
				tablenum		机械手参数TABLE起始编号
				Virmaxaxises	关联虚拟轴个数
				VirAxislist		虚拟轴列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Connframe(ZMC_HANDLE handle,int Jogmaxaxises,int *JogAxislist, int frame, int tablenum , int Virmaxaxises , int *VirAxislist)
{
	int i;
	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
	//
	if(0 > Jogmaxaxises || Jogmaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	if(0 > Virmaxaxises || Virmaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
    //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< Jogmaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",JogAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",JogAxislist[Jogmaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
	
    //生成命令

	sprintf(tempbuff, "CONNFRAME(%d,%d,",frame,tablenum);
	strcat(cmdbuff, tempbuff);
	
	for(i = 0; i< Virmaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",VirAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",VirAxislist[Virmaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //CONNREFRAME机械手正解指令	2系列以上控制器支持
Input:          //卡链接handle
				Virmaxaxises	关联虚拟轴个数
				VirAxislist		虚拟轴列表
				frame			机械手类型
				tablenum		机械手参数TABLE起始编号
				Jogmaxaxises	关节轴数量
				JogAxislist		关节轴列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Connreframe(ZMC_HANDLE handle,int Virmaxaxises , int *VirAxislist, int frame, int tablenum , int Jogmaxaxises,int *JogAxislist)
{
	int i;
	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
	//
	//
	if(0 > Jogmaxaxises || Jogmaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	if(0 > Virmaxaxises || Virmaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
   //生成命令
	strcpy(cmdbuff, "BASE(");
	
	for(i = 0; i< Virmaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",VirAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",VirAxislist[Virmaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//换行
	strcat(cmdbuff, "\n");
		

    //生成命令
	
	sprintf(tempbuff, "CONNREFRAME(%d,%d,",frame,tablenum);
	strcat(cmdbuff, tempbuff);
	
	for(i = 0; i< Jogmaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",JogAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	
	//
	sprintf(tempbuff, "%d)",JogAxislist[Jogmaxaxises-1]);
    strcat(cmdbuff, tempbuff);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}



/*********************************单轴运动****************************************************

/*************************************************************
Description:    //轴叠加运动	iaddaxis运动叠加到iaxis轴 ，ADDAX指令叠加的是脉冲个数
Input:          //卡链接handle  
				 iaxis		被叠加轴
				 iaddaxis	叠加轴
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Single_Addax(ZMC_HANDLE handle, int iaxis, int iaddaxis)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "ADDAX(%d) AXIS(%d)", iaddaxis, iaxis);
	
	
	//调用命令执行函数
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
}


/*************************************************************
Description:    //单轴运动停止
Input:          //卡链接handle  
				iaxis 轴号
				imode 模式
					0（缺省）取消当前运动
					1	取消缓冲的运动
					2	取消当前运动和缓冲运动。
					3	立即中断脉冲发送。
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Single_Cancel(ZMC_HANDLE handle, int iaxis, int imode)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "CANCEL(%d) AXIS(%d)", imode, iaxis);
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //单轴连续运动
Input:          //卡链接handle  
				iaxis 轴号
				idir 方向 1正向 -1负向
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Single_Vmove(ZMC_HANDLE handle, int iaxis, int idir)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "VMOVE(%d) AXIS(%d)", idir, iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
}

/*************************************************************
Description:    //控制器方式回零
Input:          //卡链接handle  
				iaxis	轴号
				imode	模式
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Single_Datum(ZMC_HANDLE handle, int iaxis, int imode)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "AXIS_STOPREASON(%d) = 0\r\nDATUM(%d) AXIS(%d)",iaxis,imode,iaxis);	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
}

/*************************************************************
Description:    //回零完成状态
Input:          //卡链接handle  
				iaxis 轴号
Output:         //homestatus 回零完成标志 0-回零异常 1回零成功
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetHomeStatus(ZMC_HANDLE handle,uint32 iaxis,uint32 * homestatus)
{

	int32 iresult;
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	

	if(NULL == homestatus || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	sprintf(cmdbuff, "?IDLE(%d),AXIS_STOPREASON(%d)",iaxis,iaxis,iaxis);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}
	int temparray[4];
	iresult = ZAux_TransStringtoInt(cmdbuffAck,2,&temparray[0]);
	int idlestatus = temparray[0];
	int stopstatus = temparray[1];

	
	if((idlestatus == -1)  && stopstatus == 0)		//停止了
	{
		*homestatus = 1;	//回零完成	
	}
	else
	{
		*homestatus = 0;	//回零未成功
	}
	return ERR_OK;
}

/*************************************************************
Description:    //单轴相对运动
Input:          //卡链接handle  
				iaxis 轴号
				fdistance 距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Single_Move(ZMC_HANDLE handle, int iaxis, float fdistance)
{
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "MOVE(%f) AXIS(%d)", fdistance, iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //单轴绝对运动
Input:          //卡链接handle  
				iaxis 轴号
				fdistance 距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Single_MoveAbs(ZMC_HANDLE handle, int iaxis, float fdistance)
{

	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "MOVEABS(%f) AXIS(%d)", fdistance, iaxis);
	
	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}



/*********************内存操作
/*************************************************************
Description:    //写VR, 
Input:          //卡链接handle 
				vrstartnum		VR起始编号
				numes			写入的数量
				pfValue			写入的数据列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetVrf(ZMC_HANDLE handle,int vrstartnum, int numes, float *pfValue)
{
	//int i;
	//int32 iresult;
	//
	//char  cmdbuff[2048];
	//char  cmdbuffAck[2048];
	//if(NULL == pfValue || numes < 0)
	//{
	//	return  ERR_AUX_PARAERR;
	//}
	//
	//for(i = 0; i< numes; i++)
	//{
	//	//
	//	sprintf(cmdbuff, "VR(%d) =  %f ", (i + vrstartnum) , pfValue[i]);
	//	//调用命令执行函数
	//	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	//	if(ERR_OK != iresult)
	//	{
	//		return iresult;
	//	}
	//}
	//
	//return ERR_OK;

	int i, icur, isend;
	int32 iresult;
	char  tempbuff[2048];
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == pfValue || numes < 0)
	{
		return  ERR_AUX_PARAERR;
	}


	isend = 0;
	while(1)
	{
		//一次发送个数40个
		icur = numes - isend;
		if(icur > 200)
		{
			icur = 200;
		}

		iresult = 	ZMC_RegisterWrite(handle,"VR",32,vrstartnum+isend,icur,(uint8*)(pfValue+isend));
		if(ERR_OK != iresult)
		{
			return iresult;
		}

		isend+= icur;
		if(isend >= numes)
		{
			break;
		}
	}
	return ERR_OK;
		
}


/*************************************************************
Description:    //VR读取, 可以一次读取多个
Input:          //卡链接handle  
				vrstartnum	读取的VR起始地址
				numes		读取的数量
Output:         //pfValue  返回的读取值，多个时必须分配空间.
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetVrf(ZMC_HANDLE handle, int vrstartnum, int numes, float *pfValue)
{
	//int i, icur, isend;
	//int32 iresult;
	//char  tempbuff[2048];
	//char  cmdbuff[2048];
	//char  cmdbuffAck[2048];
	//
	//if(NULL == pfValue || numes < 0)
	//{
	//	return  ERR_AUX_PARAERR;
	//}
	//
	//isend = 0;
	//while(1)
	//{
	//	//一次发送个数15个
	//	icur = numes - isend;
	//	if(icur > 15)
	//	{
	//		icur = 15;
	//	}
	//	
	//	//生成命令
	//	strcpy(cmdbuff, "?");
	//	for(i = 0; i< icur; i++)
	//	{
	//		//
	//		sprintf(tempbuff, "vr(%d) ",vrstartnum+isend+i);
	//		strcat(cmdbuff, tempbuff);
	//	}
	//	
	//	//调用命令执行函数
	//	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	//	if(ERR_OK != iresult)
	//	{
	//		return iresult;
	//	}
	//	
	//	//
	//	if(0 == strlen(cmdbuffAck))
	//	{
	//		return ERR_NOACK;
	//	}
	//	
	//	//
	//	iresult = ZAux_TransStringtoFloat(cmdbuffAck, icur, pfValue+isend);
	//	if(ERR_OK != iresult)
	//	{
	//		return iresult;
	//	}
	//	
	//	isend+= icur;
	//	if(isend >= numes)
	//	{
	//		break;
	//	}
	//}
	//
	//
	//return ERR_OK;

	int i, icur, isend;
	int32 iresult;
	char  tempbuff[2048];
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == pfValue || numes < 0)
	{
		return  ERR_AUX_PARAERR;
	}


	isend = 0;
	while(1)
	{
		//一次发送个数40个
		icur = numes - isend;
		if(icur > 200)
		{
			icur = 200;
		}

		iresult = ZMC_RegisterRead(handle,"VR",32,vrstartnum+isend,icur,(uint8*)(pfValue+isend));
		if(ERR_OK != iresult)
		{
			return iresult;
		}

		isend+= icur;
		if(isend >= numes)
		{
			break;
		}
	}
	return ERR_OK;
}

/*************************************************************
Description:    //VRINT读取， 必须150401以上版本才支持VRINT的DIRECTCOMMAND读取
Input:          //卡链接handle  
				vrstartnum	读取的VR起始地址
				numes		读取的数量
				Output:         //piValue  返回的读取值，多个时必须分配空间.
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetVrInt(ZMC_HANDLE handle, int vrstartnum, int numes, int *piValue)
{
	int i, icur, isend;
	int32 iresult;
	char  tempbuff[2048];
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	
	if(NULL == piValue || numes < 0)
	{
		return  ERR_AUX_PARAERR;
	}
	
	isend = 0;
	while(1)
	{
		//一次发送个数15个
		icur = numes - isend;
		if(icur > 15)
		{
			icur = 15;
		}
		
		//生成命令
		strcpy(cmdbuff, "?");
		for(i = 0; i< icur; i++)
		{
			//
			sprintf(tempbuff, "vr_int(%d) ",vrstartnum+isend+i);
			strcat(cmdbuff, tempbuff);
		}
		
		//调用命令执行函数
		iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
		if(ERR_OK != iresult)
		{
			return iresult;
		}
		
		//
		if(0 == strlen(cmdbuffAck))
		{
			return ERR_NOACK;
		}
		
		//
		iresult = ZAux_TransStringtoInt(cmdbuffAck, icur, piValue+isend);
		if(ERR_OK != iresult)
		{
			return iresult;
		}
		
		isend+= icur;
		if(isend >= numes)
		{
			break;
		}
	}
	
	return ERR_OK;
}

/*************************************************************
Description:    //写table 
Input:          //卡链接handle 
				tabstart	写入的TABLE起始编号
				numes		写入的数量
				pfValue		写入的数据值
Output:         //	
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetTable(ZMC_HANDLE handle,int tabstart, int numes, float *pfValue)
{
	int i ,icur;
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	char  tempbuff[2048];
	
	if(NULL == pfValue || numes < 1)
	{
		return  ERR_AUX_PARAERR;
	}

	int isend = 0;
	while(1)
	{
		//一次发送个数15个
		icur = numes - isend;
		if(icur > 15)
		{
			icur = 15;
		}

		//生成命令
		sprintf(cmdbuff, "table(%d",tabstart+isend);
		for(i = 0; i< icur; i++)
		{
			//
			sprintf(tempbuff, ",%f",pfValue[isend+i]);
			strcat(cmdbuff, tempbuff);
		}
		sprintf(tempbuff, ")");
		strcat(cmdbuff, tempbuff);

		//调用命令执行函数
		iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
		if(ERR_OK != iresult)
		{
			return iresult;
		}

		//
		isend+= icur;
		if(isend >= numes)
		{
			break;
		}
	}
	
	return ERR_OK;
	
	
}

/*************************************************************
Description:    //table读取, 可以一次读取多个
Input:          //卡链接handle  
				tabstart	读取TABLE起始地址
				numes		读取的数量
Output:         //pfValue  多个时必须分配空间.
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetTable(ZMC_HANDLE handle, int tabstart, int numes, float *pfValue)
{
	int i, icur, isend;
	int32 iresult;
	char  tempbuff[2048];
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == pfValue || numes < 0)
	{
		return  ERR_AUX_PARAERR;
	}
	
	
	isend = 0;
	while(1)
	{
		//一次发送个数15个
		icur = numes - isend;
		if(icur > 15)
		{
			icur = 15;
		}
		
		//生成命令
		strcpy(cmdbuff, "?");
		for(i = 0; i< icur; i++)
		{
			//
			sprintf(tempbuff, "TABLE(%d) ",tabstart+isend+i);
			strcat(cmdbuff, tempbuff);
		}
		
		//调用命令执行函数
		iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
		if(ERR_OK != iresult)
		{
			return iresult;
		}
		
		//
		if(0 == strlen(cmdbuffAck))
		{
			return ERR_NOACK;
		}
		
		//
		iresult = ZAux_TransStringtoFloat(cmdbuffAck, icur, pfValue+isend);
		if(ERR_OK != iresult)
		{
			return iresult;
		}
		
		isend+= icur;
		if(isend >= numes)
		{
			break;
		}
	}
	
	
	return ERR_OK;
}



#if 0
//辅助函数
#endif

/*************************************************************
Description:    //字符串转为float
Input:          //卡链接handle 
				pstringin 数据的字符串
				inumes   转换数据个数
Output:         //pfvlaue 转换的数据
Return:         //错误码
*************************************************************/
int32  ZAux_TransStringtoFloat(const char* pstringin, int inumes,  float* pfvlaue)
{
    char *ptemp;
	
    ptemp = (char*)pstringin;
    while(' ' == *ptemp)
    {
        ptemp++;
    }
    
    if(!(isdigit(ptemp[0]) || ('-' == ptemp[0])))
    {
        return ERR_ACKERROR;
    }
	
    char *pstringnew = ptemp;
    
    //                
    for(int i =0; i < inumes;i++)
    {
		
        while((' ' == *pstringnew) || ('\t' == *pstringnew))
        {
            pstringnew++;
        }
        if(('\0' == pstringnew[0]) || ('\r' == pstringnew[0]) || ('\n' == pstringnew[0]) || !(isdigit(pstringnew[0]) || ('-' == pstringnew[0])))
        {
            break;
        }
		
        double dvalue = strtod(pstringnew, &ptemp);
        if((pstringnew == ptemp))
        {
            //break;
            return ERR_ACKERROR;
        }    
		
        pfvlaue[i] = dvalue;
        
        //跳过上次的
        pstringnew = ptemp;
    }
	
	
    return ERR_OK;
}

/*************************************************************
Description:    //字符串转为int
Input:          //卡链接handle 
				pstringin 数据的字符串
				inumes   转换数据个数
Output:         //pivlaue 转换的数据
Return:         //错误码
*************************************************************/
int32  ZAux_TransStringtoInt(const char* pstringin, int inumes,  int* pivlaue)
{
    char *ptemp;
	
    ptemp = (char*)pstringin;
    while(' ' == *ptemp)
    {
        ptemp++;
    }
    
    if(!(isdigit(ptemp[0]) || ('-' == ptemp[0])))
    {
        return ERR_ACKERROR;
    }
	
    char *pstringnew = ptemp;
    
    //               
    for(int i =0; i < inumes;i++)
    {
		
        while((' ' == *pstringnew) || ('\t' == *pstringnew))
        {
            pstringnew++;
        }
        if(('\0' == pstringnew[0]) || ('\r' == pstringnew[0]) || ('\n' == pstringnew[0]) || !(isdigit(pstringnew[0]) || ('-' == pstringnew[0])))
        {
            break;
        }
		
        double dvalue = strtod(pstringnew, &ptemp);
        if((pstringnew == ptemp))
        {
            //break;
            return ERR_ACKERROR;
        }    
		
        pivlaue[i] = dvalue;    //转换成整数
        
        //跳过上次的
        pstringnew = ptemp;
    }
	
	
    return ERR_OK;
}


#if 0
//U盘格式的相关函数
#endif

/*************************************************************
Description:    //把float格式的变量列表存储到文件， 与控制器的U盘文件格式一致.
Input:          //sFilename 文件绝对路径
				pVarlist	写入的数据列表
				inum		数据的长度
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_WriteUFile(const char *sFilename, float *pVarlist, int inum)
{
    FILE *pfile;
	
    //写入文件
    pfile = fopen(sFilename, "wb");
    if(NULL == pfile)
    {
        //
        ZAUX_ERROR("open file:%s err", sFilename);
        return ERR_AUX_FILE_ERROR;
    }
	
    fseek(pfile, 0, SEEK_SET);
	
	if(fwrite(pVarlist, 4, inum, pfile) != inum)
	{
        //
        ZAUX_ERROR("fwrite size != %d", inum);
        fclose(pfile);
        return ERR_AUX_OS_ERR;
	}
	
	//关闭文件
	fclose(pfile);
	return ERR_OK;
}

/*************************************************************
Description:    //读取float格式的变量列表， 与控制器的U盘文件格式一致.
Input:          //sFilename 文件绝对路径
				inum		数据的长度	
Output:         //pVarlist	读取的数据列表
Return:         //错误码
*************************************************************/
int32  ZAux_ReadUFile(const char *sFilename, float *pVarlist, int* pinum)
{
    FILE *pfile;
    uint32 uifilesize;
	
    //读取文件
    pfile = fopen(sFilename, "rb");
    if(NULL == pfile)
    {
        //
        ZAUX_ERROR("open file:%s err", sFilename);
        return ERR_AUX_FILE_ERROR;
    }
    fseek(pfile, 0, SEEK_END);
	
    uifilesize = ftell(pfile);
	
    fseek(pfile, 0, SEEK_SET);	
	if(fread(pVarlist, 1, uifilesize, pfile) != uifilesize)
	{
        //
        ZAUX_ERROR("read size != %d", uifilesize);
        fclose(pfile);
		
        return ERR_AUX_OS_ERR;
	}
	//关闭文件
	fclose(pfile);
	
	*pinum = uifilesize/4;
	return ERR_OK;
}



/*************************************************************
Description:    //modbus寄存器操作 modbus_bit
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
				pdata 设置的位状态  按位存储
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Set0x(ZMC_HANDLE handle, uint16 start, uint16 inum, uint8* pdata)
{
	int32 iresult;
	iresult = ZMC_Modbus_Set0x(handle,start,inum,pdata);
	
	return iresult;
}

/*************************************************************
Description:    //modbus寄存器操作 modbus_bit
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
Output:         //pdata 返回的位状态  按位存储
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Get0x(ZMC_HANDLE handle, uint16 start, uint16 inum, uint8* pdata)
{
	int32 iresult;
	iresult = ZMC_Modbus_Get0x(handle,start,inum, pdata);
	
	return iresult;
}

/*************************************************************
Description:    //modbus寄存器操作		MODBUS_REG
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
				pdata	设置值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Set4x(ZMC_HANDLE handle, uint16 start, uint16 inum, uint16* pdata)
{
	int32 iresult;
	iresult = ZMC_Modbus_Set4x(handle,start,inum, pdata);
	
	return iresult;
}

/*************************************************************
Description:    //modbus寄存器操作 MODBUS_REG
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
Output:         //pdata	读取的REG寄存器值
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Get4x(ZMC_HANDLE handle, uint16 start, uint16 inum, uint16* pdata)
{
	int32 iresult;
	iresult = ZMC_Modbus_Get4x(handle, start, inum, pdata);
	
	return iresult;
}

/*************************************************************
Description:    //modbus寄存器操作		MODBUS_IEEE
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
				pfdata	设置值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Get4x_Float(ZMC_HANDLE handle, uint16 start, uint16 inum, float* pfdata)
{
	int 	i,j;
	int32 	iresult;
	void * pf;
	uint16 * pi;
	uint16  tempdata[2048];
	
	if(NULL == pfdata || inum < 0)
	{
		return  ERR_AUX_PARAERR;
	}
	
	
	pi = tempdata;
	pf = pfdata;
	
	iresult = ZMC_Modbus_Get4x(handle,start,inum*2,tempdata);
	if(iresult == 0)
	{
		
		for(i=1;i<inum +1;i++)
		{
			for(j=0;j<2;j++)
			{
				*((uint16 *)pf + j )  = *(pi +j);
			}
			pf = pfdata +i;
			pi = pi+2;
		}
	}
	
	return iresult;
}

/*************************************************************
Description:    //modbus寄存器操作 MODBUS_IEEE
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
Output:         //pfdata	读取的REG寄存器值
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Set4x_Float(ZMC_HANDLE handle, uint16 start, uint16 inum, float* pfdata)
{
	int 	i,j;
	int32 	iresult;
	void * pf;
	uint16 * pi;
	uint16  tempdata[2048];
	
	if(NULL == pfdata || inum < 0)
	{
		return  ERR_AUX_PARAERR;
	}
	
	pi = tempdata;
	pf = pfdata;
	
	for(i=1;i<inum +1;i++)
	{
		for(j=0;j<2;j++)
		{
			*(pi +j) = *((uint16 *)pf + j);
		}
		pf = pfdata +i;
		pi = pi+2;
	}	
	iresult = ZMC_Modbus_Set4x(handle,start,inum*2,tempdata);
	return iresult;
}

/*************************************************************
Description:    //modbus寄存器操作		MODBUS_LONG
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
				pidata	设置值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Get4x_Long(ZMC_HANDLE handle, uint16 start, uint16 inum, int32 * pidata)
{
	int 	i,j;
	int32 	iresult;
	void * pitemp;
	uint16 * pi;
	uint16  tempdata[2048];
	
	if(NULL == pidata || inum < 0)
	{
		return  ERR_AUX_PARAERR;
	}
	
	pi = tempdata;
	pitemp = pidata;
	
	iresult = ZMC_Modbus_Get4x(handle,start,inum*2,tempdata);
	if(iresult == 0)
	{
		
		for(i=1;i<inum +1;i++)
		{
			for(j=0;j<2;j++)
			{
				*((uint16 *)pitemp + j )  = *(pi +j);
			}
			pitemp = pidata +i;
			pi = pi+2;
		}
	}
	
	return iresult;
}

/*************************************************************
Description:    //modbus寄存器操作 MODBUS_LONG
Input:          //卡链接handle 寄存器地址
				start	起始编号
				inum	数量
Output:         //pidata	读取的REG寄存器值
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Set4x_Long(ZMC_HANDLE handle, uint16 start, uint16 inum, int32 * pidata)
{
	int 	i,j;
	int32 	iresult;
	void * pitemp;
	uint16 * pi;
	uint16  tempdata[2048];
	
	if(NULL == pidata || inum < 0)
	{
		return  ERR_AUX_PARAERR;
	}
	
	pi = tempdata;
	pitemp = pidata;
	
	for(i=1;i<inum +1;i++)
	{
		for(j=0;j<2;j++)
		{
			*(pi +j) = *((uint16 *)pitemp + j);
		}
		pitemp = pidata +i;
		pi = pi+2;
	}	
	iresult = ZMC_Modbus_Set4x(handle,start,inum*2,tempdata);
	return iresult;
}

/*************************************************************
Description:    //读取modbus_string
Input:          //卡链接handle
				start	modbus起始地址
				inum	长度			
Output:         pidata	读取返回的字符串
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Get4x_String(ZMC_HANDLE handle, uint16 start, uint16 inum, char * pidata)
{
	int 	i;
	int32 	iresult;
	uint16  tempdata[2048];
	uint16 charnum;

	if(NULL == pidata || inum < 0 || inum > 2048 )
	{
		return  ERR_AUX_PARAERR;
	}
	
	charnum = (inum - 1)/2 + 1;

	iresult = ZMC_Modbus_Get4x(handle,start,charnum,tempdata);
	if(iresult == 0)
	{	
		for(i=0;i<inum ;i++)
		{
			if(i%2 == 0)
			{
				*(pidata + i) = (char)(tempdata[i/2]);
			}
			else
			{
				*(pidata + i) = (char)(tempdata[i/2]>>8);
			}
		}
	}
	
	return iresult;
}

/*************************************************************
Description:    //设置modbus_string
Input:          //卡链接handle
				start	modbus起始地址
				inum	长度
				pidata	写入的字符串
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Modbus_Set4x_String(ZMC_HANDLE handle, uint16 start, uint16 inum, char * pidata)
{
	int 	i;
	int32 	iresult;
	uint16  tempdata[2048];
	uint16 charnum;

	if(NULL == pidata || inum < 0 || inum > 2048 )
	{
		return  ERR_AUX_PARAERR;
	}

	for(i=0;i<inum ;i++)
		{
			if(i%2 == 0)
			{
				tempdata[i/2] =(uint16)(*(pidata + i));
			}
			else
			{
				tempdata[i/2] =(uint16)(*(pidata + i)) * 256 + tempdata[i/2];
			}
		}
	
	charnum = (inum - 1)/2 + 1;

	iresult = ZMC_Modbus_Set4x(handle,start,charnum,tempdata);

	return iresult;
}



/*************************************************************
Description:    //写用户flash块, float数据
Input:          //卡链接handle
				uiflashid 	flash块号
				uinumes		变量个数
				pfvlue		数据
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_FlashWritef(ZMC_HANDLE handle, uint16 uiflashid, uint32 uinumes, float *pfvlue)
{
	int32 iresult;
	iresult = ZMC_FlashWritef(handle, uiflashid, uinumes,pfvlue);
	
	return iresult;
}


/*************************************************************
Description:    //读取用户flash块, float数据
Input:          //卡链接handle
uiflashid 	flash块号
uibuffnum	缓冲变量个数
Output:         //
puinumesread 读取到的变量个数
Return:         //错误码
*************************************************************/
int32  ZAux_FlashReadf(ZMC_HANDLE handle, uint16 uiflashid, uint32 uibuffnum, float *pfvlue, uint32* puinumesread)
{
	int32 iresult;
	iresult = ZMC_FlashReadf(handle, uiflashid, uibuffnum,pfvlue,puinumesread);
	return iresult;
}


/***********************************2018-08-24 V2.1函数添加****************************************************************************
增加部分特殊功能函数
增加总线相关函数
***************************************************************************************************************************************/

/*************************************************************
Description:    //示波器触发函数 150723以后版本支持
Input:          //卡链接handle  
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Trigger(ZMC_HANDLE handle)
{

	//调用命令执行函数
		char  cmdbuffAck[2048];
		return ZAux_Execute(handle, "Trigger", cmdbuffAck, 2048);
}


//MOVE_PARA,MOVE_PWM, MOVE_SYNMOVE,MOVE_ASYNMOVE
/*************************************************************
Description:    //运动中修改参数. 20170503以上固件支持
Input:          //卡链接handle  
				base_axis 运动主轴
				paraname  修改的轴参数字符串名称
				iaxis	  参数修改轴号
				fvalue	  设置值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MovePara(ZMC_HANDLE handle,uint32 base_axis,char *  paraname,uint32 iaxis,float fvalue)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX || 0 > base_axis|| base_axis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "MOVE_PARA(%s,%d,%f) axis(%d)",paraname,iaxis,fvalue,base_axis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //运动中修改PWM 20170503以上固件支持
Input:          //卡链接handle  
				base_axis	插补主轴编号
				pwm_num		PWM口编号
				pwm_duty	占空比0-1
				pwm_freq	频率 硬件PWM 1M ， 软件PWM  2k 
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MovePwm(ZMC_HANDLE handle,uint32 base_axis,uint32 pwm_num,float pwm_duty,float pwm_freq)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > base_axis|| base_axis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "MOVE_PWM(%d,%f,%f) axis(%d)",pwm_num,pwm_duty,pwm_freq,base_axis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //运动中同步其他轴的运动,. 20170503以上固件支持
Input:          //卡链接handle
				base_axis 运动主轴
				iaxis	  同步轴号
				fdist	  同步轴运动距离
				ifsp	  是否使用SP运动
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveSynmove(ZMC_HANDLE handle,uint32 base_axis,uint32 iaxis,float fdist,uint32 ifsp)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX || 0 > base_axis|| base_axis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "MOVE_SYNMOVE(%d,%f,%d) axis(%d)",iaxis,fdist,ifsp,base_axis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //运动中触发其他轴的运动. 20170503以上固件支持
Input:          //卡链接handle
				base_axis 运动主轴
				iaxis	  触发轴轴号（不能为当前轴）
				fdist	  触发轴运动距离
				ifsp	  是否使用SP运动
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveASynmove(ZMC_HANDLE handle,uint32 base_axis,uint32 iaxis,float fdist,uint32 ifsp)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX || 0 > base_axis|| base_axis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "MOVE_ASYNMOVE(%d,%f,%d) axis(%d)",iaxis,fdist,ifsp,base_axis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //运动中修改TABLE
Input:          //卡链接handle  
				base_axis	插补主轴编号
				table_num	TABLE编号
				fvalue		修改值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveTable(ZMC_HANDLE handle,uint32 base_axis,uint32 table_num,float fvalue)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > base_axis|| base_axis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "MOVE_TABLE(%d,%f) axis(%d)",table_num,fvalue,base_axis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //BASE轴运动缓冲加入一个可变的延时  固件150802以上版本， 或XPLC160405以上版本支持。
Input:          //卡链接handle  
				base_axis	插补主轴编号
				paraname	参数名字符串 DPOS MPOS IN AIN VPSPEED MSPEED MODBUS_REG MODBUS_IEEE MODBUS_BIT NVRAM VECT_BUFFED  REMAIN 
				inum		参数编号或轴号
				Cmp_mode	比较条件 1 >=   0=  -1<=  对IN等BIT类型参数无效。
				fvalue		修改值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveWait(ZMC_HANDLE handle,uint32 base_axis,char * paraname,int inum,int Cmp_mode,float fvalue)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > base_axis|| base_axis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "MOVE_Wait(%s,%d,%d,%f) axis(%d)",paraname,inum,Cmp_mode,fvalue,base_axis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //BASE轴运动缓冲加入一个TASK任务 当任务已经启动时，会报错，但不影响程序执行。
Input:          //卡链接handle			
				base_axis	插补主轴编号
				tasknum   	任务编号 
				labelname		BAS中全局函数名或者标号

Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MoveTask(ZMC_HANDLE handle,uint32 base_axis,uint32 tasknum,char * labelname)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > base_axis|| base_axis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "MOVE_TASK(%d,%s) axis(%d)",tasknum,labelname,base_axis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //位置比较PSWITCH
Input:          //卡链接handle
				//比较器编号 num 0-15
				//比较器使能 enable 0/1
				//比较的轴号 axisnum
				//输出口编号 outnum
				//输出状态	outstate 0/1
				//比较起始位置	setpos
				//输出复位位置	resetpos
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_Pswitch(ZMC_HANDLE handle, int num, int enable, int axisnum, int outnum, int  outstate, float setpos,float resetpos)
{
	if(0 > num || num > 15)
	{
		return  ERR_AUX_PARAERR;
	}	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "PSWITCH(%d,%d,%d,%d,%d,%f,%f)", num,enable,axisnum,outnum,outstate,setpos,resetpos);
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, NULL, 0);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
}

/*************************************************************
Description:    //硬件位置比较输出 4系列产品脉冲轴与编码器轴支持硬件比较输出
Input:          //卡链接handle
				//模式	 mode 1-启动比较器, 2- 停止并删除没完成的比较点.
				//方向 direction 0-坐标负向,  1- 坐标正向  
				//预留 Reserve   预留
				//TABLE起始点 Tablestart  第一个比较点坐标所在TABLE编号
				//TABLE结束点 tableend 最后一个比较点坐标所在TABLE编号
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_HwPswitch(ZMC_HANDLE handle,int Axisnum,int Mode, int Direction, int Reserve, int Tablestart, int  Tableend)
{
	if(0 > Axisnum || Axisnum > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "HW_PSWITCH(%d,%d,%d,%d,%d) AXIS(%d)", Mode, Direction, Reserve, Tablestart, Tableend,Axisnum);
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, NULL, 0);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
}


/*************************************************************
Description:    //硬件位置比较输出剩余缓冲获取 4系列产品脉冲轴与编码器轴支持硬件比较输出
Input:          //卡链接handle
				//模式	 axisnum				轴号
output:			//位置比较输出剩余缓冲数		buff
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetHwPswitchBuff(ZMC_HANDLE handle,int axisnum,int * buff)
{
	if(0 > axisnum || axisnum > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}	
	int32 iresult;
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "?HW_PSWITCH(%d)", axisnum);
	//调用命令执行函数
	//iresult =  ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	iresult =  ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",buff);
	
	return ERR_OK;
}

/*************************************************************
Description:    //硬件定时器用于硬件比较输出后一段时间后还原电平 4系列产品支持
Input:          //卡链接	handle  
				//模式			mode		0停止,  2-启动			
				//周期时间	   cyclonetime  us单位
				//有效时间	   optime		us单位
				//重复次数	   reptimes
				//输出缺省状态 opstate		输出口变为非此状态后开始计时
				// 输出口编号  opnum		必须能硬件比较输出的口
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_HwTimer(ZMC_HANDLE handle,int mode,int cyclonetime,int optime,int reptimes,int opstate,int  opnum )
{
	
	char  cmdbuff[2048];
    //生成命令
	sprintf(cmdbuff, "HW_TIMER(%d,%d,%d,%d,%d,%d)",mode,cyclonetime,optime,reptimes,opstate,opnum);
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, NULL, 0);
	return ZAux_Execute(handle, cmdbuff, NULL,0);
}

/*************************************************************
Description:    //读取轴停止原因
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回状态，对应AXISSTATUS判断对应位
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAxisStopReason(ZMC_HANDLE handle, int iaxis, int *piValue)
{
	int32 iresult;
	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(NULL == piValue || iaxis > MAX_AXIS_AUX || iaxis < 0)
	{
		return  ERR_AUX_PARAERR;
	}
	
    //生成命令
	sprintf(cmdbuff, "?AXIS_STOPREASON(%d)", iaxis);
	
	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
        return iresult;
	}
	
	//
	if(0 == strlen(cmdbuffAck))
	{
        return ERR_NOACK;
	}
	
	//
	sscanf(cmdbuffAck, "%d",piValue);
	
	return ERR_OK;
}

/*************************************************************
Description:    //浮点型读全部轴参数状态
Input:          //卡链接handle 
				sParam 轴参数名称字符串
				imaxaxis 轴数量
Output:         pfValue  返回的轴参数列表
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAllAxisPara(ZMC_HANDLE handle,const char *sParam,int imaxaxis,float * pfValue)
{

	int32 iresult;
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == pfValue || imaxaxis <=0 || imaxaxis > MAX_AXIS_AUX)
	{
	return  ERR_AUX_PARAERR;
	}

	strcpy(cmdbuff, "?*");
	strcat(cmdbuff, sParam);

	//调用命令执行函数
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
			return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
			return ERR_NOACK;
	}

	//
	iresult = ZAux_TransStringtoFloat(cmdbuffAck,imaxaxis,pfValue);
	if(ERR_OK != iresult)
	{
	return iresult;
	}
	return ERR_OK;
}

/*************************************************************
Description:    //浮点型读全部轴参数状态  IdleStatus-运动状态 DposStatus-命令坐标  MposStatus-反馈坐标  AxisStatus-轴状态
Input:          //卡链接handle 
				imaxaxis 轴数量
Output:         IdleStatus 运动状态
				DposStatus 命令坐标
				MposStatus 反馈坐标
				AxisStatus 轴状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetAllAxisInfo(ZMC_HANDLE handle,int imaxaxis,int * IdleStatus,float * DposStatus,float * MposStatus,int * AxisStatus)
{
	int32 iresult = 0;
	if(imaxaxis <=0 || imaxaxis > MAX_AXIS_AUX || NULL == IdleStatus || NULL == DposStatus || NULL == MposStatus || NULL == AxisStatus)
	{
	return  ERR_AUX_PARAERR;
	}
	float pi_idle[MAX_AXIS_AUX],pf_axisstatus[MAX_AXIS_AUX];
	iresult += ZAux_Direct_GetAllAxisPara(handle,"IDLE",imaxaxis,pi_idle);
	iresult += ZAux_Direct_GetAllAxisPara(handle,"DPOS",imaxaxis,DposStatus);
	iresult += ZAux_Direct_GetAllAxisPara(handle,"MPOS",imaxaxis,MposStatus);
	iresult += ZAux_Direct_GetAllAxisPara(handle,"AXISSTATUS",imaxaxis,pf_axisstatus);

	if(iresult == ERR_OK)
	{
		for(int i = 0;i<imaxaxis;i++)
		{
			*(IdleStatus + i ) = (int)pi_idle[i];
			*(AxisStatus + i ) = (int)pf_axisstatus[i];
		}
	}
	return iresult;
}

/*************************************************************
Description:    //设置BASIC自定义全局数组  
Input:          //卡链接handle  
				arrayname 数组名称
				arraystart 数组起始元素
				numes		元素数量
				pfValue     设置值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetUserArray(ZMC_HANDLE handle,char * arrayname ,int arraystart, int numes, float *pfValue)
{
	int i;
	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == pfValue || numes < 0)
	{
		return  ERR_AUX_PARAERR;
	}

	for(i = 0; i< numes; i++)
	{
		//
		sprintf(cmdbuff, "%s(%d) =  %f ",arrayname ,(i + arraystart) , pfValue[i]);
		//调用命令执行函数
		iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
		if(ERR_OK != iresult)
		{
			return iresult;
		}
	}

	return ERR_OK;


}

/*************************************************************
Description:    //读取设置BASIC自定义全局数组 , 可以一次读取多个
Input:          //卡链接handle  
				arrayname 数组名称
				arraystart 数组起始元素
				numes		元素数量
Output:         //pfValue  多个时必须分配空间.
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetUserArray(ZMC_HANDLE handle,char * arrayname ,int arraystart, int numes, float *pfValue)
{
	int i, icur, isend;
	int32 iresult;
	char  tempbuff[2048];
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == pfValue || numes < 0)
	{
		return  ERR_AUX_PARAERR;
	}

	isend = 0;
	while(1)
	{
		//一次发送个数30个
		icur = numes - isend;
		if(icur > 30)
		{
			icur = 30;
		}

		//生成命令
		strcpy(cmdbuff, "?*");

		//
		sprintf(tempbuff, "%s(%d,%d)",arrayname,arraystart+isend,icur);
		strcat(cmdbuff, tempbuff);


		//调用命令执行函数
		iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
		//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
		if(ERR_OK != iresult)
		{
			return iresult;
		}

		//
		if(0 == strlen(cmdbuffAck))
		{
			return ERR_NOACK;
		}

		//
		iresult = ZAux_TransStringtoFloat(cmdbuffAck, icur, pfValue+isend);
		if(ERR_OK != iresult)
		{
			return iresult;
		}

		isend+= icur;
		if(isend >= numes)
		{
			break;
		}
	}

	return ERR_OK;
}

/*************************************************************
Description:    //设置自定义变量, 
Input:          //卡链接handle  
				varname 变量名称字符串
				pfValue	设定值
Output:         //  
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetUserVar(ZMC_HANDLE handle,char * varname , float pfValue)
{
	int32 iresult;
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	//
	sprintf(cmdbuff, "%s = %f",varname,pfValue);


	//调用命令执行函数
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //读取自定义全局变量, 
Input:          //卡链接handle  
				varname 变量名称字符串
Output:         //pfValue  多个时必须分配空间.
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetUserVar(ZMC_HANDLE handle,char * varname , float *pfValue)
{
	int32 iresult;
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == pfValue)
	{
		return  ERR_AUX_PARAERR;
	}

	//
	sprintf(cmdbuff, "?%s",varname);


	//调用命令执行函数
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}

	//
	sscanf(cmdbuffAck, "%f",pfValue);

	return ERR_OK;
}


/*************************************************************
Description:    //与控制器建立链接
Input:          //PCI卡号， 卡号从0开始，实际连接为PCI卡号+1(兼容传统PCI卡规则)
Output:         //卡链接handle
Return:         //错误码
*************************************************************/
int32  ZAux_OpenPci(uint32 cardnum, ZMC_HANDLE * phandle)
{
	int32 iresult;
    char buffer[1024];
	sprintf(buffer, "PCI%d", cardnum + 1);

	iresult = ZMC_Open(ZMC_CONNECTION_PCI,buffer, phandle);
	return iresult;
}

/*************************************************************
Description:    //读取PCI的控制卡个数
Input:          //
Return:         //检查的最大PCI卡个数
*************************************************************/
int32  ZAux_GetMaxPciCards()
{
	return ZMC_GetMaxPciCards();
}

/*************************************************************
Description:    //获取控制器卡信息
Input:          //卡链接handle  
Output:         SoftType 控制器型号类型
				SoftVersion 控制器软件版本（固件版本）
				ControllerId	控制器唯一ID
Return:         //错误码
*************************************************************/
int32  ZAux_GetControllerInfo(ZMC_HANDLE handle ,char * SoftType ,char * SoftVersion ,char *  ControllerId)
{

	int32 iresult;
	char  cmdbuffAck[2048];

	//软件版本 ?CONTROL SoftType   
	iresult = ZAux_Execute(handle, "?CONTROL", cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}
	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}
	//
	sscanf(cmdbuffAck, "%s",SoftType);


	//?VERSION_DATE SoftVersion
	iresult = ZAux_Execute(handle, "?VERSION_DATE", cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}
	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}
	//
	sscanf(cmdbuffAck, "%s",SoftVersion);

	//控制器ID  ?SERIAL_NUMBER
	iresult = ZAux_Execute(handle, "?SERIAL_NUMBER", cmdbuffAck,2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}
	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}
	//
	sscanf(cmdbuffAck, "%s",ControllerId);

	return ERR_OK;
}


/**************************************************总线相关函数***********************************************
大部分函数只支持Execute方式发送  
添加总线节点信息读取
总线回零，及回零状态判断等
***************************************************************************************************************/

/*************************************************************
Description:    //读取卡槽上节点数量
Input:          //卡链接handle 
:				 //slot 槽位号缺省0
Output:         //piValue 返回扫描成功节点数量
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetNodeNum(ZMC_HANDLE handle,int slot,int *piValue)
{
	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == piValue)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "?NODE_COUNT(%d)", slot);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}

	//
	sscanf(cmdbuffAck, "%d",piValue);

	return ERR_OK;
}

/*************************************************************
Description:    //读取节点上的信息
Input:          //卡链接handle 
				slot	槽位号
				node	节点编号
				sel		信息编号	0-厂商编号1-设备编号 2-设备版本 3-别名 10-IN个数 11-OUT个数 
Output:         //piValue 返回信息
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetNodeInfo(ZMC_HANDLE handle,int slot,int node,int sel ,int *piValue)
{
	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == piValue)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "?NODE_INFO(%d,%d,%d)", slot,node,sel);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}
	//
	sscanf(cmdbuffAck, "%d",piValue);
	return ERR_OK;
}

/*************************************************************
Description:    //读取节点总线状态
Input:          //卡链接handle  
				slot 槽位号缺省0
				node 节点编号
Output:         //nodestatus 按位处理 bit0-节点是否存在  bit1-通讯状态   bit2-节点状态
值为1时，bit0为1，bit1和bit2为0，设备通讯正常
值为3时，bit0和bit1为1，bit2为0，设备通讯出错
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetNodeStatus(ZMC_HANDLE handle,uint32 slot,uint32 node,uint32 * nodestatus)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == nodestatus)
	{
		return  ERR_AUX_PARAERR;
	}
	//生成命令
	sprintf(cmdbuff, "?NODE_STATUS(%d,%d)", slot,node);
	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}
	//
	sscanf(cmdbuffAck, "%d",nodestatus);
	return ERR_OK;
}

/*************************************************************
Description:    //读取节点SDO参数信息
Input:          //卡链接handle  
				slot	槽位号 缺省0
				node	节点编号
				index	对象字典编号（注意函数为10进制数据）
				subindex	子编号	（注意函数为10进制数据）
				type	数据类型  1-bool 2-int8 3-int16 4-int32 5-uint8 6-uint16 7-uint32
Output:         //value 读取的数据值
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_SDORead(ZMC_HANDLE handle,uint32 slot,uint32 node,uint32 index,uint32 subindex ,uint32 type ,int32 * value)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == value || node > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "SDO_READ(%d,%d,%d,%d,%d,0)",slot, node,index,subindex,type);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(0 != strlen(cmdbuffAck))
	{
		return ERR_AUX_NOTSUPPORT;
	}
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	float fvalue = 0;
	iresult = ZAux_Direct_GetTable(handle,0,1,&fvalue);
	if(ERR_OK != iresult)
	{
		return iresult;
	}	
	*value = (int)fvalue;
	return ERR_OK;
}

/*************************************************************
Description:    //写节点SDO参数信息
Input:          //卡链接handle  
				slot	槽位号 缺省0
				node	节点编号
				index	对象字典编号（注意函数为10进制数据）
				subindex	子编号	（注意函数为10进制数据）
				type	数据类型  1-bool 2-int8 3-int16 4-int32 5-uint8 6-uint16 7-uint32
				value	设定的数据值
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_SDOWrite(ZMC_HANDLE handle,uint32 slot,uint32 node,uint32 index,uint32 subindex ,uint32 type ,int32  value)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(node > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff,"SDO_WRITE(%d,%d,%d,%d,%d,%d)",slot, node,index,subindex,type,value);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(0 != strlen(cmdbuffAck))
	{
		return ERR_AUX_NOTSUPPORT;
	}

	return iresult;
}

/*************************************************************
Description:    //读取轴SDO参数信息
Input:          //卡链接handle  
				slot	槽位号 缺省0
				node	节点编号
				index	对象字典编号（注意函数为10进制数据）
				subindex	子编号	（注意函数为10进制数据）
				type	数据类型  1-bool 2-int8 3-int16 4-int32 5-uint8 6-uint16 7-uint32
Output:         //value 读取的数据值
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_SDOReadAxis(ZMC_HANDLE handle,uint32 iaxis,uint32 index,uint32 subindex ,uint32 type ,int32 * value)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == value || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "SDO_READ_AXIS(%d,%d,%d,%d,0)",iaxis,index,subindex,type);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(0 != strlen(cmdbuffAck))
	{
		return ERR_AUX_NOTSUPPORT;
	}
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	float fvalue = 0;
	iresult = ZAux_Direct_GetTable(handle,0,1,&fvalue);
	if(ERR_OK != iresult)
	{
		return iresult;
	}	
	*value = (int)fvalue;
	return ERR_OK;
}

/*************************************************************
Description:    //写轴SDO参数信息
Input:          //卡链接handle  
				slot	槽位号 缺省0
				node	节点编号
				index	对象字典编号（注意函数为10进制数据）
				subindex	子编号	（注意函数为10进制数据）
				type	数据类型  1-bool 2-int8 3-int16 4-int32 5-uint8 6-uint16 7-uint32
				value	设定的数据值
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_SDOWriteAxis(ZMC_HANDLE handle,uint32 iaxis,uint32 index,uint32 subindex ,uint32 type ,int32  value)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff,"SDO_WRITE_AXIS(%d,%d,%d,%d,%d)",iaxis,index,subindex,type,value);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(0 != strlen(cmdbuffAck))
	{
		return ERR_AUX_NOTSUPPORT;
	}

	return iresult;
}


/*************************************************************
Description:    //Rtex读取参数信息
Input:          //卡链接handle  
				iaxis	轴号
				ipara	参数分类*256 + 参数编号  Pr7.11-ipara = 7*256+11
Output:         //value 读取的数据值
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_RtexRead(ZMC_HANDLE handle,uint32 iaxis ,uint32 ipara ,float * value)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == value || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "BASE(%d)\r\nDRIVE_READ(%d,0)",iaxis,ipara);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}
	if(0 != strlen(cmdbuffAck))
	{
		return ERR_AUX_NOTSUPPORT;
	}

	//
	iresult = ZAux_Direct_GetVrf(handle,0,1,value);
	if(ERR_OK != iresult)
	{
		return iresult;
	}	
	return ERR_OK;
}

/*************************************************************
Description:    //Rtex写参数信息
Input:          //卡链接handle  
				iaxis	轴号
				ipara	参数分类*256 + 参数编号  Pr7.11-ipara = 7*256+11
				value	设定的数据值
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_RtexWrite(ZMC_HANDLE handle,uint32 iaxis ,uint32 ipara,float  value)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff,"BASE(%d)\r\nDRIVE_WRITE(%d,%f)",iaxis, ipara,value);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(0 != strlen(cmdbuffAck))
	{
		return ERR_AUX_NOTSUPPORT;
	}
	return iresult;
}

/*************************************************************
Description:    //设置回零偏移距离
Input:          //卡链接handle  
				iaxis 轴号
				fValue 偏移距离
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_SetDatumOffpos(ZMC_HANDLE handle, int iaxis, float fValue)
{


	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "DATUM_OFFSET(%d)=%f", iaxis, fValue);

	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取回零偏移距离
Input:          //卡链接handle  
				iaxis 轴号
Output:         //fValue 反馈的偏移距离
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetDatumOffpos(ZMC_HANDLE handle, int iaxis, float *fValue)
{
	int32 iresult;
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == fValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "?DATUM_OFFSET(%d)", iaxis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}

	//
	sscanf(cmdbuffAck, "%f",fValue);
	return ERR_OK;
}

/*************************************************************
Description:    //总线驱动器回零
Input:          //卡链接handle  
				homemode 回零模式，查看驱动器手册
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_Datum(ZMC_HANDLE handle,uint32 iaxis,uint32  homemode)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	int32 iresult;

	sprintf(cmdbuff, "AXIS_STOPREASON(%d) = 0\r\nDATUM(21,%d) AXIS(%d)",iaxis,homemode,iaxis);


	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,0);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //驱动器回零完成状态
Input:          //卡链接handle  
				iaxis 轴号
Output:         //homestatus 回零完成标志 0-回零异常 1回零成功
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetHomeStatus(ZMC_HANDLE handle,uint32 iaxis,uint32 * homestatus)
{

	int32 iresult;
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];	

	if(NULL == homestatus || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	sprintf(cmdbuff, "?DRIVE_STATUS(%d),IDLE(%d),AXIS_STOPREASON(%d)",iaxis,iaxis,iaxis);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,2048);
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}
	int temparray[4];
	iresult = ZAux_TransStringtoInt(cmdbuffAck,4,&temparray[0]);
	int tempstatus = temparray[0];
	int idlestatus = temparray[1];
	int stopstatus = temparray[2];

	
	if((idlestatus == -1) && ((tempstatus >> 12	) & 1) == 1 && stopstatus == 0)		//停止了
	{
		*homestatus = 1;	//回零完成	
	}
	else
	{
		*homestatus = 0;	//回零未成功
	}
	return ERR_OK;
}


/*************************************************************
Description:    //设置清除总线伺服报警
Input:          //卡链接handle 
				iaxis 轴号
				mode 模式 0-清除当前告警  1-清除历史告警  2-清除外部输入告警
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_DriveClear(ZMC_HANDLE handle,uint32 iaxis,uint32  mode)
{

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	
	if(0 > iaxis || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	
	int32 iresult;

	sprintf(cmdbuff, "BASE(%d)\r\nDRIVE_CLEAR(%d)\r\nDATUM(0)",iaxis,mode);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,0);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck,0);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	return ERR_OK;
}

/*************************************************************
Description:    //读取当前总线驱动当前力矩	需要设置对应的DRIVE_PROFILE类型
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 当前转矩
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetDriveTorque(ZMC_HANDLE handle, int iaxis, int *piValue)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "?DRIVE_TORQUE(%d)", iaxis);

	//调用命令执行函数
	//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}

	//
	sscanf(cmdbuffAck, "%d",piValue);
	return ERR_OK;
}

/*************************************************************
Description:    //设置当前总线驱动最大转矩  需要设置对应的DRIVE_PROFILE类型
Input:          //卡链接handle		
				iaxis 轴号
				piValue 最大转矩限制
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_SetMaxDriveTorque(ZMC_HANDLE handle, int iaxis, int piValue)
{
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}
	//生成命令
	sprintf(cmdbuff, "DRIVE_TORQUEMAX(%d)=%d", iaxis, piValue);

	//调用命令执行函数
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck,0);
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}


/*************************************************************
Description:    //读取当前总线驱动最大转矩  需要设置对应的DRIVE_PROFILE类型
Input:          //卡链接handle  
				iaxis 轴号
Output:         //piValue 返回的最大转矩
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetMaxDriveTorque(ZMC_HANDLE handle, int iaxis, int *piValue)
{
	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == piValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "?DRIVE_TORQUEMAX(%d)", iaxis);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}

	//
	sscanf(cmdbuffAck, "%d",piValue);
	return ERR_OK;
}

/*************************************************************
Description:    //设置模拟量输出 力矩、速度模式下可以  总线驱动需要设置对应DRIVE_PROFILE类型 与ATYPE
Input:          //卡链接handle  
				iaxis 轴号
				模拟量 输出值
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetDAC(ZMC_HANDLE handle, int iaxis, float fValue)
{


	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
	if( iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "DAC(%d)=%f", iaxis, fValue);

	//调用命令执行函数
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck, 2048);
	//return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //读取模拟量输出 力矩、速度模式下可以  总线驱动需要设置对应DRIVE_PROFILE类型 与ATYPE
Input:          //卡链接handle  
				iaxis 轴号
Output:         //fValue 模拟量返回值
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetDAC(ZMC_HANDLE handle, int iaxis, float *fValue)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == fValue || iaxis > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//生成命令
	sprintf(cmdbuff, "?DAC(%d)", iaxis);

	//调用命令执行函数
	iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
	//iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	if(ERR_OK != iresult)
	{
		return iresult;
	}

	//
	if(0 == strlen(cmdbuffAck))
	{
		return ERR_NOACK;
	}

	//
	sscanf(cmdbuffAck, "%f",fValue);
	return ERR_OK;
}

/*************************************************************
Description:    //总线扫描初始化  （只针对Zmotion tools 工具软件配置过总线参数控制器使用有效）
Input:          //卡链接handle  
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_InitBus(ZMC_HANDLE handle)
{
	return  ZAux_Direct_SetUserVar(handle,"P_BusCmd",2);
}


/*************************************************************
Description:    //获取总线初始化完成状态  （只针对Zmotion tools 工具软件配置过总线参数控制器使用有效）
Input:          //卡链接handle  
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_BusCmd_GetInitStatus(ZMC_HANDLE handle,int *piValue)
{
	int32 iresult;
	float scan_flag,start_flag;
	if(NULL == piValue)
	{
		return  ERR_AUX_PARAERR;
	}
	iresult = ZAux_Direct_GetUserVar(handle,"Bus_Scan_Status",&scan_flag);
	if(iresult != ERR_OK)
	{
		return iresult;
	}
	iresult = ZAux_Direct_GetUserVar(handle,"Bus_Start_Status",&start_flag);
	if(iresult != ERR_OK)
	{
		return iresult;
	}
	if((int)scan_flag == 1 && (int)start_flag == 1)
	{
		*piValue = 1;
	}
	else
	{
		*piValue = 0;
	}
	return ERR_OK;
}

/*************************************************************
Description:    //读取多个输入信号
Input:          //卡链接handle  
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetInMulti(ZMC_HANDLE handle, int startio , int endio, int32 *piValue)
{

	int32 iresult;

	char  cmdbuff[2048];
	char  cmdbuffAck[2048];

	if(NULL == piValue || startio > endio)
	{
		return  ERR_AUX_PARAERR;
	}

	int32 icur,istart,iend;		//一次最多32个
	icur=0;
	while(1)
	{
		iend = startio + 31;
		if(iend > endio)
		{
			iend = endio;
		}
		//生成命令
		sprintf(cmdbuff, "?IN(%d,%d)", startio,iend);

		//调用命令执行函数
		//iresult = ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
		iresult = ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
		if(ERR_OK != iresult)
		{
			return iresult;
		}

		//
		if(0 == strlen(cmdbuffAck))
		{
			return ERR_NOACK;
		}

		//
		sscanf(cmdbuffAck, "%d",&piValue[icur]);

		startio = iend + 1;		
		icur ++;
		if(iend == endio)			//读取完毕
		{
			break;
		}
	}

	return ERR_OK;
}

/*************************************************************
Description:    //命令的延时等待时间
Input:          //卡链接handle 毫秒时间
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_SetTimeOut(ZMC_HANDLE  handle, uint32 timems)
{
	g_ZMC_MaxExcuteWaitms = timems;
	return ZMC_SetTimeOut(handle,timems);
}


/*************************************************************
Description:    //硬件位置比较输出2 4系列产品, 20170513以上版本支持.  ZMC306E/306N支持
Input:          //卡链接					handle
				//模式						mode		
				//输出口编号				Opnum		4系列 out 0-3为硬件位置比较输出
				//第一个比较点的输出状态	Opstate		0-关闭 1打开										
				//多功能参数				ModePara1	 
				//多功能参数				ModePara2
				//多功能参数				ModePara3
				//多功能参数				ModePara4

mode 1-启动比较器, 
		ModePara1 =  第一个比较点坐标所在TABLE编号
		ModePara2 =	 最后一个比较点坐标所在TABLE编号
		ModePara3 =  第一个点判断方向,  0-坐标负向,  1- 坐标正向,  -1-不使用方向
		ModePara4 =	 预留

mode 2- 停止并删除没完成的比较点. 
		ModePara1 =  预留
		ModePara2 =	 预留
		ModePara3 =  预留
		ModePara4 =	 预留

mode 3- 矢量比较方式
		ModePara1 =  第一个比较点坐标所在TABLE编号
		ModePara2 =	 最后一个比较点坐标所在TABLE编号
		ModePara3 =  预留
		ModePara4 =	 预留

Mode=4 :矢量比较方式, 单个比较点
		ModePara1 =  比较点坐标
		ModePara2 =	 预留
		ModePara3 =  预留
		ModePara4 =	 预留

Mode=5 :矢量比较方式, 周期脉冲模式
		ModePara1 =  比较点坐标
		ModePara2 =	 重复周期, 一个周期内比较两次, 先输出有效状态,再输出无效状态.
		ModePara3 =  周期距离, 每隔这个距离输出Opstate, 输出有效状态的距离（ModePara4）后还原为无效状态.
		ModePara4 =	 输出有效状态的距离,  (ModePara3- ModePara4) 为无效状态距离

Mode=6 :矢量比较方式, 周期模式, 这种模式一般与HW_TIMER一起使用.
		ModePara1 =  比较点坐标
		ModePara2 =	 重复周期, 一个周期只比较一次
		ModePara3 =  周期距离, 每隔这个距离输出一次
		ModePara4 =	 预留
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_HwPswitch2(ZMC_HANDLE handle,int Axisnum,int Mode, int Opnum , int Opstate, float ModePara1, float ModePara2,float ModePara3,float ModePara4)
{
	if(0 > Axisnum || Axisnum > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}	
	char  cmdbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	switch(Mode)
	{
		case 1:
			sprintf(cmdbuff, "HW_PSWITCH2(%d,%d,%d,%f,%f,%f) AXIS(%d)", Mode, Opnum, Opstate, ModePara1,ModePara2,ModePara3,Axisnum);
			break;
		case 2:
			sprintf(cmdbuff, "HW_PSWITCH2(%d) AXIS(%d)", Mode,Axisnum);
			break;
		case 3:
			sprintf(cmdbuff, "HW_PSWITCH2(%d,%d,%d,%f,%f) AXIS(%d)", Mode, Opnum, Opstate, ModePara1,ModePara2,Axisnum);
			break;
		case 4:
			sprintf(cmdbuff, "HW_PSWITCH2(%d,%d,%d,%f) AXIS(%d)", Mode, Opnum, Opstate, ModePara1,Axisnum);
			break;
		case 5:
			sprintf(cmdbuff, "HW_PSWITCH2(%d,%d,%d,%f,%f,%f,%f) AXIS(%d)", Mode, Opnum, Opstate, ModePara1,ModePara2,ModePara3,ModePara4,Axisnum);
			break;
		case 6:
			sprintf(cmdbuff, "HW_PSWITCH2(%d,%d,%d,%f,%f,%f) AXIS(%d)", Mode, Opnum, Opstate, ModePara1,ModePara2,ModePara3,Axisnum);
			break;
		default:
			return  ERR_AUX_PARAERR;			
			break;
	}
	//调用命令执行函数
	//return ZAux_DirectCommand(handle, cmdbuff, NULL, 0);
	return ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
}

/*************************************************************
Description:    //获取控制器最大规格数
Input:          //卡链接handle 
Output:         //Max_VirtuAxises	 最大虚拟轴数
Output:         //Max_motor			 最大电机数量
Output:         //Max_io			 最大IN,OUT,AD,DA数量				
Return:         //错误码
*************************************************************/
int32  ZAux_GetSysSpecification(ZMC_HANDLE  handle, uint16 * Max_VirtuAxises,uint8 * Max_motor,uint8 *Max_io)
{
	int32 iresult;
	struct_SysMaxSpecification specification;

	if(Max_VirtuAxises == NULL || Max_motor == NULL || Max_io == NULL)
	{
		return  ERR_AUX_PARAERR;
	}

	iresult = ZMC_GetSysSpecification(handle,&specification);
	if(ERR_OK != iresult)
	{
		return iresult;
	}
	*Max_motor = specification.m_MaxAxises;
	*Max_VirtuAxises= specification.m_MaxVirtuAxises;
	*Max_io =  specification.m_MaxInController;
	*(Max_io + 1) =  specification.m_MaxOutController;
	*(Max_io + 2) =  specification.m_MaxAdController;
	*(Max_io + 3) =  specification.m_MaxDaController;

	return ERR_OK;
}

/*************************************************************
Description:    //控制器自动上报
Input:          //卡链接	handle 
Input:         //回调函数    pcallback		
		   PZMCAutoUpCallBack函数格式
				Input:          //卡链接handle
				itypecode: 上传类型码
				idatalength: 数据长度
				pdata: 数据指针
Return:         //错误码
*************************************************************/
int32  ZAux_SetAutoUpCallBack(ZMC_HANDLE handle, PZMCAutoUpCallBack pcallback)
{
	return ZMC_SetAutoUpCallBack(handle,pcallback);
}


/*************************************************************
Description:    //IO接口 设置多路输出
Input:          //卡链接handle
Input:          //IO口起始编号  iofirst
Input:          //IO口结束编号  ioend
Input:          //输出口状态    istate按位存储，一个UINT存储32个输出口状态
Output:         //状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_SetOutMulti(ZMC_HANDLE handle, uint16 iofirst,uint16 ioend, uint32 * istate)
{
	int32 iresult;
	if(NULL == istate || iofirst > ioend)
	{
		return  ERR_AUX_PARAERR;
	}

	int32 icur,startio,iend;		//一次最多32个
	startio = iofirst;
	icur=0;
	while(1)
	{
		iend = startio + 31;
		if(iend > ioend)
		{
			iend = ioend;
		}
		iresult = ZMC_SetOutAll(handle, startio,iend, *(istate + icur));
		if(iresult != ERR_OK)
		{
			return  iresult;
		}
		startio = iend + 1;		
		icur ++;
		if(iend == ioend)			//设置完毕
		{
			break;
		}
	}
	return ERR_OK;
}


/*************************************************************
Description:    //IO接口 设置多路输出
Input:          //卡链接handle
Input:          //IO口起始编号  iofirst
Input:          //IO口结束编号  ioend
Input:          //输出口状态    istate按位存储，一个UINT存储32个输出口状态
Output:         //状态
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_GetOutMulti(ZMC_HANDLE handle, uint16 iofirst,uint16 ioend, uint32 * istate)
{
	int32 iresult;
	if(NULL == istate || iofirst > ioend)
	{
		return  ERR_AUX_PARAERR;
	}

	int32 icur,startio,iend;		//一次最多32个
	startio = iofirst;
	icur=0;
	while(1)
	{
		iend = startio + 31;
		if(iend > ioend)
		{
			iend = ioend;
		}
		iresult = ZMC_GetOutAll(handle, startio,iend, &istate[icur]);
		if(iresult != ERR_OK)
		{
			return  iresult;
		}
		startio = iend + 1;		
		icur ++;
		if(iend == ioend)			//设置完毕
		{
			break;
		}
	}
	return ERR_OK;
}


/*************************************************************
Description:    //多条相对多轴直线插补 
Input:          //卡链接handle
				iMoveLen			填写的运动长度
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				pfDisancelist		距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MultiMove(ZMC_HANDLE handle,int iMoveLen, int imaxaxises, int *piAxislist, float *pfDisancelist)
{
	int i,j;
	int32 iresult;
	char  cmdbuff[20480];
	char  tempbuff[20480];
	char  cmdbuffAck[20480];	
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//先读取剩余直线缓冲
	int iBuffLen = 0;
	iresult = ZAux_Direct_GetRemain_LineBuffer(handle,piAxislist[0],&iBuffLen);
	if(iBuffLen <= iMoveLen)
	{
		return 1002;			//缓冲不够
	}


	//生成命令BASE命令
	strcpy(cmdbuff, "BASE(");
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
	strcat(cmdbuff, tempbuff);
	//换行
	strcat(cmdbuff, "\n");

	//生成运动命令
	for(j=0;j<iMoveLen;j++)
	{
		strcat(cmdbuff, "MOVE(");

		for(i = 0; i< imaxaxises-1; i++)
		{
			//
			sprintf(tempbuff, "%f,",pfDisancelist[i +j*imaxaxises]);
			strcat(cmdbuff, tempbuff);
		}

		//
		sprintf(tempbuff, "%f)\n",pfDisancelist[i + j*imaxaxises]);
		strcat(cmdbuff, tempbuff);
	}

	int ilen = strlen(cmdbuff);				//获取命令长度
	if(ilen > 1000)
	{
		return  20002;
	}
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);

}



/*************************************************************
Description:    //多条相对多轴直线插补 
Input:          //卡链接handle
				iMoveLen			填写的运动长度
				imaxaxises			参与运动总轴数
				piAxislist			轴号列表
				pfDisancelist		距离列表
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_MultiMoveAbs(ZMC_HANDLE handle,int iMoveLen, int imaxaxises, int *piAxislist, float *pfDisancelist)
{
	int i,j;
	int32 iresult;
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];	
	if(0 > imaxaxises || imaxaxises > MAX_AXIS_AUX)
	{
		return  ERR_AUX_PARAERR;
	}

	//先读取剩余直线缓冲
	int iBuffLen = 0;
	iresult = ZAux_Direct_GetRemain_LineBuffer(handle,piAxislist[0],&iBuffLen);
	if(iBuffLen <= iMoveLen)
	{
		return 1002;			//缓冲不够
	}


	//生成命令BASE命令
	strcpy(cmdbuff, "BASE(");
	for(i = 0; i< imaxaxises-1; i++)
	{
		//
		sprintf(tempbuff, "%d,",piAxislist[i]);
		strcat(cmdbuff, tempbuff);
	}
	sprintf(tempbuff, "%d)",piAxislist[imaxaxises-1]);
	strcat(cmdbuff, tempbuff);
	//换行
	strcat(cmdbuff, "\n");

	//生成运动命令
	for(j=0;j<iMoveLen;j++)
	{
		strcat(cmdbuff, "MOVEABS(");

		for(i = 0; i< imaxaxises-1; i++)
		{
			//
			sprintf(tempbuff, "%f,",pfDisancelist[i +j*imaxaxises]);
			strcat(cmdbuff, tempbuff);
		}

		//
		sprintf(tempbuff, "%f)\n",pfDisancelist[i + j*imaxaxises]);
		strcat(cmdbuff, tempbuff);
	}


	int ilen = strlen(cmdbuff);				//获取命令长度
	if(ilen > 1000)
	{
		return  20002;
	}
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
}

/*************************************************************
Description:    //机械手坐标系旋转
Input:          //卡链接handle
				iaxis					轴号 关节轴/虚拟轴
				pfRotatePara			平移旋转参数
Output:         //
Return:         //错误码
*************************************************************/
int32  ZAux_Direct_FrameRotate(ZMC_HANDLE handle,int iaxis, float *pfRotatePara)
{
	int i;	
	char  cmdbuff[2048];
	char  tempbuff[2048];
	char  cmdbuffAck[2048];
    //生成命令
	sprintf(cmdbuff, "BASE(%d)\n",iaxis);
			
    //生成命令
	sprintf(tempbuff, "FRAME_ROTATE(%f,%f,%f,%f,%f,%f)", pfRotatePara[0],pfRotatePara[1],pfRotatePara[2],pfRotatePara[3],pfRotatePara[4], pfRotatePara[5]);
    strcat(cmdbuff, tempbuff);	
	//调用命令执行函数
	return ZAux_DirectCommand(handle, cmdbuff, cmdbuffAck, 2048);
	//return ZAux_Execute(handle, cmdbuff, cmdbuffAck,2048);
}

/*************************************************************
Description:    //获取CAN扩展资源规格
Input:          //卡链接handle 
Output:         //CanNum			 当前连接的CAN从站数量
Output:         //CanId_List		 当前连接的CAN从站ID列表
Output:         //CanIn_List		 节点输入点数量
Output:         //CanOut_List		 节点输出点数量
Output:         //CanAin_List		 节点AD数量	
Output:         //CanAOut_List		 节点DA数量	
Output:         //CanAxis_List		 节点轴数量				
Return:         //错误码
*************************************************************/
int32  ZAux_GetCanInfo(ZMC_HANDLE  handle, uint8 * CanNum,uint16 * CanId_List,uint8 *CanIn_List,uint8 *CanOut_List,uint8 *CanAin_List,uint8 *CanAOut_List,uint8 *CanAxis_List)
{
	int32 iresult;
	uint16 Canid = 0;
	uint8 Temp_CanNum = 0;
	struct_ChildCardInfo *m_CardInfo = (struct_ChildCardInfo *)malloc(sizeof(struct_ChildCardInfo));

	iresult = ZMC_EnumChildCard(handle,-1, m_CardInfo);
	Canid = m_CardInfo->m_cardid;
	while(Canid < 128 && (ERR_OK == iresult))
	{
		*(CanId_List + Temp_CanNum) = Canid;
		*(CanIn_List + Temp_CanNum) = m_CardInfo->m_imaxin;
		*(CanOut_List + Temp_CanNum) = m_CardInfo->m_imaxout;
		*(CanAin_List + Temp_CanNum) = m_CardInfo->m_imaxad;
		*(CanAOut_List + Temp_CanNum) = m_CardInfo->m_imaxda;
		*(CanAxis_List + Temp_CanNum) = m_CardInfo->m_iAxises;

		iresult = ZMC_EnumChildCard(handle, Canid, m_CardInfo);
		Canid = m_CardInfo->m_cardid;

		Temp_CanNum ++;
	}

	*CanNum = Temp_CanNum;
	return ERR_OK;
}




