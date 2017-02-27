#ifndef __PROTO_COM_H__
#define	__PROTO_COM_H__
#include "stm32f10x.h"
typedef struct PROTO_RX  *ptrPROTORX;


struct PROTO_RX
{
	//帧头和帧尾
	uint8_t HeadCnt;
	const uint8_t *FrmHead;
	const uint8_t *FrmEnd;

	//接收缓冲区
	uint16_t *pHead;
	uint16_t *pEnd;
	uint8_t *pRxBuf;
	uint16_t BufLen;  
	uint32_t (*checksum)(uint8_t *, uint16_t );
	//当缓冲区包含这么多个数据时开始处理
	uint16_t def_val;
	
	//帧长度相关部分
	uint16_t len_offset;  
	uint16_t len_bycnt;
	uint16_t ext_len;  //帧中长度域无法包含的字节个数 
	uint32_t (*GetInnerLen)(ptrPROTORX);
	
	
};


uint32_t GetFrmInerLen(ptrPROTORX Protorx);
int ProtoRx(uint8_t *pRet, uint16_t *pLen, ptrPROTORX Protorx, uint16_t ret_buf_sz);
#endif
