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
	//����
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
** Descriptions:      ���ݽ��յ��������ͽ��зֱ���
****************************************************************************************************/
void PcSetHost(void)
{
	#define	HT_BUF_SZ		530
	uint8_t hostBuf[HT_BUF_SZ];
	uint16_t len, tmplen;
	

	if (ProtoRx(hostBuf, &len, SetRx, HT_BUF_SZ) == 0) //���ݷ�����ȷ
	{
		uint8_t cmd_type;
		if (hostBuf[0] != gInterFace_id)  //��ַ��ƥ��
			return ;
		ESPLog(("Receive PC's data ok\r\n"));
		cmd_type = hostBuf[2];
			
		//�ж����������Ͳ����в�ͬ�Ĳ���
		switch(cmd_type)
		{
			case 0x01:	//���Ķ�ȡģ�龲̬����
				BrmStaticTest(hostBuf[5]);
				break;
			case 0x02:	//Ӧ������̬����
				YdqStaticTest(hostBuf[5]);
				break;
			case 0x03:	//���ñ��Ĵ���ģ���ַ
				SetBrmID(hostBuf[5]);
				break;
			case 0x04:  //��ȡ����
				ReadBrm(hostBuf[5]);
				break;
			case 0x05:	//��ȡ�հ���
				GetBrmSndRate(hostBuf[5]);
				break;
			case 0x06:	//Ӧ��������д��
				tmplen = (hostBuf[3]<<8 | hostBuf[4]);
				YdqWrite(&hostBuf[5], tmplen);
				break;
			case 0x07: //
				SigSwitch(hostBuf[5]);
				break;
			case 0x0E:
				GetBrmSndRateMul(hostBuf[5]); //�������շ�����
				break;
			default:   //��������֮���֡������
				break;					
		}			
	}
}



