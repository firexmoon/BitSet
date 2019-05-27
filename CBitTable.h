#pragma once

#include <mutex>


#define u32 unsigned int

#define CBT_BIT_1      1
#define CBT_BIT_0      0
#define CBT_U32_FULL   0xffffffff
#define CBT_NOT_FOUND  (CBT_U32_FULL - 1)
#define CBT_BAD_INDEX  (CBT_U32_FULL - 2)


class CBitTable
{
	std::mutex m_Mutex;

	u32 * m_pTable;
	u32 m_TabSize;		//bit数
	u32 m_MemSize;		//占用的内存字节数
	u32 m_CountOfBit1;	//bit-1的数量


	void InitTable(u32 bits);
	void ReleaseTable();
	u32 FindNextBit(u32 start_index, u32 bit_val);
	u32 FindNextBitInU32(u32 u32val, u32 start_offset, u32 bit_val);

public:

	~CBitTable();
	CBitTable(u32 size);

	u32 SetBit(u32 index);
	u32 ClrBit(u32 index);
	u32 GetBit(u32 index);
	u32 FindNextBit0(u32 start_index);
	u32 FindNextBit1(u32 start_index);

	u32 getSize();
	u32 * getMemAddr();
	u32 getMemSize();

};