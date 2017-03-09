#include "app.h"
#include "isr.h"
#include "crc.h"
#include "protocom.h"


/*GetFrmInerLen
*���֡�ڰ����ĳ���ֵ  ����ֵ���ֽ���ǰ ���ֽ��ں�
*���볤�����֡ͷ��ƫ�Ƴ�����ռ�õ��ֽڸ���
*����֡�а����ĳ���ֵ
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
*���֡�ڰ����ĳ���ֵ ����ֵ���ֽ���ǰ�����ֽ��ں�
*���볤�����֡ͷ��ƫ�Ƴ�����ռ�õ��ֽڸ���
*����֡�а����ĳ���ֵ
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
** Input Parameters : uint8_t *pRet ����������ȷ���ݣ�����ֵΪ0ʱ��Ч
					  PROTORX Protorx  Э������ �������ݴ�������
** Output Parameters: None
** Descriptions:      ���մ���,��Ҫɨ���ͷ��֮��Ѱ�Ұ�β��Ȼ��У�飬У��ɹ���
**					�����յ�����ȥ��ͷ��β�󷵻ظ�������
** Return 1:û���ҵ�������֡ 0�� �ҵ�������֡
****************************************************************************************************/
int ProtoRx(uint8_t *pRet, uint16_t *pLen, ptrPROTORX Protorx, uint16_t ret_buf_sz)
{
	uint16_t Head = *Protorx->pHead;
	uint16_t Tail = *Protorx->pEnd;
	uint16_t Buf_len = Protorx->BufLen;
	uint16_t defval = Protorx->def_val;
	uint8_t *rxbuf = Protorx->pRxBuf;
	*pLen = 0;
	
	//��������ݳ��ȿ��Խ��д���
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
		if (i >= Protorx->HeadCnt)  //�ҵ�֡ͷ 
		{
			//Ѱ��֡β
			while(((Tail-Head)&(Buf_len-1)) >= Protorx->HeadCnt) //���Խ����Ұ�β
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
				if (i >= Protorx->HeadCnt)   //�ҵ�֡β
				{
					//�ж�֡β�Ƿ���������֡β
					int tmplen = tHead - *Protorx->pHead;  //����һ�����ݵĳ���
					int frmlen = 0;
					tmplen &= (Buf_len -1);
					tmplen -= Protorx->ext_len;
					frmlen = (int)Protorx->GetInnerLen(Protorx);
					if (frmlen > Buf_len || frmlen > ret_buf_sz)
					{
						//�ҵ���֡�ĳ��ȳ������еĳ��� �ҵ��Ĳ�����ȷ��֡
						Head = *Protorx->pHead;
						*Protorx->pHead = (Head+1)&(Buf_len -1);
						break;
					}
					else {
						if (tmplen < frmlen)
						{  
							//��δ����һ֡���� ������
							Head ++;
							Head &= (Buf_len -1);
						}
						else if (tmplen == frmlen) //�����յ�����һ��
						{
							//copy ���ݽ���У��
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
						else  //ͷβ֮�������̫��  �����ҵ���ͷ����ɨ��
						{
							Head = *Protorx->pHead;
							*Protorx->pHead = (Head+1)&(Buf_len -1);
							break;
						}
					}
				}
				else  //û���ҵ�֡β�޸�ͷָ��
				{
					Head ++;
					Head &= (Buf_len -1);
				}
			}
		}
		else   //û���ҵ�ͷ
		{
			//
			Head = *Protorx->pHead;
			*Protorx->pHead = (Head+1)&(Buf_len -1);
		}		
	}
	return 1;
}

