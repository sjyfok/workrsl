#include "app.h"
#include "isr.h"
#include "crc.h"
#include "protocom.h"


/*GetFrmInerLen
*获得帧内包含的长度值  长度值低字节在前 高字节在后
*输入长度相对帧头的偏移长度域占用的字节个数
*返回帧中包含的长度值
*/
uint32_t GetFrmInerLen_Small(ptrPROTORX Protorx)
{
	int len, i = 0;
	uint16_t head = *Protorx->pHead;
	uint16_t Buf_len = Protorx->BufLen;
	uint8_t *rxbuf = Protorx->pRxBuf;
	uint16_t offset = Protorx->len_offset;
	uint16_t cnt = Protorx->len_bycnt;
	
	
	head += offset;
	head &= Buf_len-1;
	len = 0;
	for (i = 0; i < cnt; i ++)
	{	
		len |= rxbuf[head]<<(i<<3);
		head ++;
		head &= Buf_len-1;
		
//		len <<= 8;
//		len |= rxbuf[head];
//		head ++;
//		head &= Buf_len-1;
	}
	return len;
}

/*GetFrmInerLen_Big 
*获得帧内包含的长度值 长度值高字节在前，低字节在后
*输入长度相对帧头的偏移长度域占用的字节个数
*返回帧中包含的长度值
*/
uint32_t GetFrmInerLen_Big(ptrPROTORX Protorx)
{
	int len, i = 0;
	uint16_t head = *Protorx->pHead;
	uint16_t Buf_len = Protorx->BufLen;
	uint8_t *rxbuf = Protorx->pRxBuf;
	uint16_t offset = Protorx->len_offset;
	uint16_t cnt = Protorx->len_bycnt;
	
	
	head += offset;
	head &= Buf_len-1;
	len = 0;
	for (i = 0; i < cnt; i ++)
	{	
		len <<= 8;
		len |= rxbuf[head];
		head ++;
		head &= Buf_len-1;
	}
	return len;
}


/****************************************************************************************************
** Function Name:     ProtoRx
** Input Parameters : uint8_t *pRet 用来返回正确数据，返回值为0时有效
					  PROTORX Protorx  协议描述 函数根据此做处理
** Output Parameters: None
** Descriptions:      接收处理,先要扫描包头，之后寻找包尾，然后校验，校验成功后，
**					将接收的数据去掉头和尾后返回给调用者
** Return 1:没有找到完整的帧 0： 找到完整的帧
****************************************************************************************************/
int ProtoRx(uint8_t *pRet, uint16_t *pLen, ptrPROTORX Protorx, uint16_t ret_buf_sz)
{
	uint16_t Head = *Protorx->pHead;
	uint16_t Tail = *Protorx->pEnd;
	uint16_t Buf_len = Protorx->BufLen;
	uint16_t defval = Protorx->def_val;
	uint8_t *rxbuf = Protorx->pRxBuf;
	*pLen = 0;
	
	//缓存的数据长度可以进行处理
	if (((Tail-Head)&(Buf_len -1)) > defval)
	{
		int i = 0;
		while(i < Protorx->HeadCnt) 
		{
			if (rxbuf[Head] != Protorx->FrmHead[i])
			{
				break;
			}	
			i ++;
			Head ++;
			Head &= (Buf_len -1);
		}
		if (i >= Protorx->HeadCnt)  //找到帧头 
		{
			//寻找帧尾
			while(((Tail-Head)&(Buf_len-1)) >= Protorx->HeadCnt) //可以进行找包尾
			{
				uint16_t tHead = Head;
				i = 0;
				while(i < Protorx->HeadCnt)
				{
					if (rxbuf[tHead] != Protorx->FrmEnd[i])
					{
						break;
					}
					i ++;
					tHead ++;
					tHead &= (Buf_len -1);
				}
				if (i >= Protorx->HeadCnt)   //找到帧尾
				{
					//判断帧尾是否是真正的帧尾
					int tmplen = tHead - *Protorx->pHead;  //整个一包数据的长度
					int frmlen = 0;
					tmplen &= (Buf_len -1);
					tmplen -= Protorx->ext_len;
					frmlen = (int)Protorx->GetInnerLen(Protorx);
					if (frmlen > Buf_len || frmlen > ret_buf_sz)
					{
						//找到的帧的长度超过队列的长度 找到的不是正确的帧
						Head = *Protorx->pHead;
						*Protorx->pHead = (Head+1)&(Buf_len -1);
						break;
					}
					else {
						if (tmplen < frmlen)
						{  
							//尚未收满一帧数据 接着收
							Head ++;
							Head &= (Buf_len -1);
						}
						else if (tmplen == frmlen) //正好收到完整一包
						{
							//copy 数据进行校验
							uint16_t idx = (*Protorx->pHead)+Protorx->HeadCnt;
							i = 0;
							idx &= (Buf_len -1);
							while (idx != Head)
							{
								pRet[i++] = Protorx->pRxBuf[idx];
								idx ++;
								idx &= (Buf_len-1);
							}
							if (Protorx->checksum != 0)
							{
								if (Protorx->checksum (pRet, i) == 0)
								{
									*Protorx->pHead = tHead;
									*pLen = i;
									return 0;
								}
								else
								{
									Head = *Protorx->pHead;
									*Protorx->pHead = (Head+1)&(Buf_len -1);
									break;
								}
							}
							else
							{
								*Protorx->pHead = tHead;
								*pLen = i;
								return 0;						
							}
						} 
						else  //头尾之间的数据太长  作废找到的头重新扫描
						{
							Head = *Protorx->pHead;
							*Protorx->pHead = (Head+1)&(Buf_len -1);
							break;
						}
					}
				}
				else  //没有找到帧尾修改头指针
				{
					Head ++;
					Head &= (Buf_len -1);
				}
			}
		}
		else   //没有找到头
		{
			//
			Head = *Protorx->pHead;
			*Protorx->pHead = (Head+1)&(Buf_len -1);
		}		
	}
	return 1;
}

