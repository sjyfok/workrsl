#include "app.h"
#include "isr.h"
#include "protocom.h"
#include "pchost.h"
#include "crc.h"
#include "carrier.h"
#include "innerflash.h"
#include "93c66.h"
#include "brm.h"

#define	HEADCNT 2
static const uint8_t frm_head[HEADCNT] = {0xA5, 0xA5};
static const uint8_t frm_end[HEADCNT] = {0x5A, 0x5A};
struct PROTO_RX PCSetRx = {	
	HEADCNT,	
	frm_head,
	frm_end,
	&g_USART1_RxHead,
	&g_USART1_RxTail,
	g_USART1_RxBuf,
	USART1_RXBUF_LEN,

	HNLX_CRC16,
	9,
	//长度
	5,
	2,
	11,
	GetFrmInerLen_Big,
};

ptrPROTORX SetRx = &PCSetRx;


/****************************************************************************************************
** Function Name:     PcSetHost
** Input Parameters : None
** Output Parameters: None
** Descriptions:      根据接收到数据类型进行分别处理
****************************************************************************************************/
void PcSetHost(void)
{
	#define	HT_BUF_SZ		530
	uint8_t hostBuf[HT_BUF_SZ];
	uint16_t len, tmplen;
	

	if (ProtoRx(hostBuf, &len, SetRx, HT_BUF_SZ) == 0) //数据返回正确
	{
		uint8_t cmd_type;
		if (hostBuf[0] != gInterFace_id)  //地址不匹配
			return ;
		ESPLog(("Receive PC's data ok\r\n"));
		cmd_type = hostBuf[2];
			
		//判断命令字类型并进行不同的操作
		switch(cmd_type)
		{
			case 0x01:	//报文读取模块静态测试
				BrmStaticTest(hostBuf[5]);
				break;
			case 0x02:	//应答器静态测试
				YdqStaticTest(hostBuf[5]);
				break;
			case 0x03:	//设置报文传输模块地址
				SetBrmID(hostBuf[5]);
				break;
			case 0x04:  //读取报文
				ReadBrm(hostBuf[5]);
				break;
			case 0x05:	//读取收包率
				GetBrmSndRate(hostBuf[5]);
				break;
			case 0x06:	//应答器报文写入
				tmplen = (hostBuf[3]<<8 | hostBuf[4]);
				YdqWrite(&hostBuf[5], tmplen);
				break;
			case 0x07: //
				SigSwitch(hostBuf[5]);
				break;
			case 0x0E:
				GetBrmSndRateMul(hostBuf[5]); //持续接收发包数
				break;
			default:   //命令类型之外的帧不处理
				break;					
		}			
	}
}



