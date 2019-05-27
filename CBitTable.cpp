
#include <cstddef>


#include <memory>

#include "CBitTable.h"

#define BYTES_IN_U32    sizeof(u32)
#define BITS_IN_U32     (BYTES_IN_U32 * 8)

#define DEF_OFFSET_BY_INDEX(index)           if (index >= m_TabSize) return CBT_BAD_INDEX; u32 offset_u32 = (index) / BITS_IN_U32; u32 offset_bit = (index) % BITS_IN_U32;

#define SET_BIT_IN_U32(u32val, offset_bit)   u32val |= (1 << (offset_bit));
#define CLR_BIT_IN_U32(u32val, offset_bit)   u32val &= ~(1 << (offset_bit));

#define BIT_IN_U32_IS_1(u32val, offset_bit)  ((u32val) & (1 << (offset_bit)))
#define BIT_IN_U32_IS_0(u32val, offset_bit)  (!BIT_IN_U32_IS_1(u32val, offset_bit))


CBitTable::~CBitTable()
{
	ReleaseTable();
}

void CBitTable::ReleaseTable()
{
	if (m_pTable)
	{
		delete[] m_pTable;
		m_pTable = NULL;
	}

	m_TabSize = 0;
	m_MemSize = 0;
	m_CountOfBit1 = 0;
}

CBitTable::CBitTable(u32 bits)
{
	m_pTable = NULL;
	m_TabSize = 0;
	m_MemSize = 0;
	m_CountOfBit1 = 0;

	InitTable(bits);
}

void CBitTable::InitTable(u32 bits)
{
	m_TabSize = bits;

	//需要的u32数
	u32 s = bits / BITS_IN_U32 + 1;
	if (!(bits % BITS_IN_U32)) s--;

	//字节数
	m_MemSize = s * BYTES_IN_U32;

	if (m_pTable)
	{
		delete[] m_pTable;
		m_pTable = NULL;
	}

	m_pTable = new u32[s];
	if (NULL == m_pTable)
	{
		ReleaseTable();
		return;
	}

	//初始化
	memset(m_pTable, 0, m_MemSize);
	m_CountOfBit1 = 0;
	return;
}


u32 CBitTable::getSize()
{
	return m_TabSize;
}


u32 CBitTable::getMemSize()
{
	return m_MemSize;
}


u32 * CBitTable::getMemAddr()
{
	return m_pTable;
}


u32 CBitTable::SetBit(u32 index)
{
	std::lock_guard<std::mutex> mtx_locker(m_Mutex);

	DEF_OFFSET_BY_INDEX(index);

	if (BIT_IN_U32_IS_0(m_pTable[offset_u32], offset_bit))
	{
		SET_BIT_IN_U32(m_pTable[offset_u32], offset_bit);
		m_CountOfBit1++;
	}
	return 0;
}

u32 CBitTable::ClrBit(u32 index)
{
	std::lock_guard<std::mutex> mtx_locker(m_Mutex);

	DEF_OFFSET_BY_INDEX(index);

	if (BIT_IN_U32_IS_1(m_pTable[offset_u32], offset_bit))
	{
		CLR_BIT_IN_U32(m_pTable[offset_u32], offset_bit);
		m_CountOfBit1--;
	}
	return 0;
}

u32 CBitTable::GetBit(u32 index)
{
	std::lock_guard<std::mutex> mtx_locker(m_Mutex);

	DEF_OFFSET_BY_INDEX(index);

	if (BIT_IN_U32_IS_1(m_pTable[offset_u32], offset_bit))
		return CBT_BIT_1;
	else
		return CBT_BIT_0;

}

u32 CBitTable::FindNextBitInU32(u32 u32val, u32 start_offset, u32 bit_val)
{
	int i;

	for (i = start_offset; i < BITS_IN_U32; i++)
	{
		if (bit_val == CBT_BIT_0)
		{
			if (BIT_IN_U32_IS_0(u32val, i))
				break;
		}
		else
		{
			if (BIT_IN_U32_IS_1(u32val, i))
				break;
		}
	}
	if (i < BITS_IN_U32)
	{
		return i;
	}
	else
	{
		return CBT_NOT_FOUND;
	}
}

u32 CBitTable::FindNextBit(u32 start_index, u32 bit_val)
{
	int i;
	u32 find_index;

	std::lock_guard<std::mutex> mtx_locker(m_Mutex);

	//先快速排除极端情况
	if (bit_val == CBT_BIT_0 && m_CountOfBit1 == m_TabSize)
		return CBT_NOT_FOUND;
	if (bit_val == CBT_BIT_1 && m_CountOfBit1 == 0)
		return CBT_NOT_FOUND;

	DEF_OFFSET_BY_INDEX(start_index);

	if (offset_bit)
	{
		//先在当前u32中查找
		find_index = FindNextBitInU32(m_pTable[offset_u32], offset_bit, bit_val);
		if (find_index != CBT_NOT_FOUND)
		{
			//找到了
			find_index = offset_u32 * BITS_IN_U32 + find_index;
			if (find_index >= m_TabSize)
				return CBT_NOT_FOUND;
			else
				return find_index;
		}
		else
		{
			//从下一个完整的u32继续
			offset_u32++;
			offset_bit = 0;
		}
	}

	//找到下一个不是全1/全0的u32
	for (i = offset_u32; i < m_MemSize / BYTES_IN_U32; i++)
	{
		if (bit_val == CBT_BIT_0)
		{
			if (m_pTable[i] != CBT_U32_FULL)
				break;
		}
		else
		{
			if (m_pTable[i] != 0)
				break;
		}
	}
	if (i == m_MemSize / BYTES_IN_U32)
	{
		return CBT_NOT_FOUND;
	}

	//在当前u32中查找
	find_index = FindNextBitInU32(m_pTable[i], 0, bit_val);
	if (find_index != CBT_NOT_FOUND)
	{
		//找到了
		find_index = i * BITS_IN_U32 + find_index;
		if (find_index >= m_TabSize)
			return CBT_NOT_FOUND;
		else
			return find_index;
	}
	else
	{
		return CBT_NOT_FOUND;
	}
}


u32 CBitTable::FindNextBit0(u32 start_index)
{
	return FindNextBit(start_index, CBT_BIT_0);
}


u32 CBitTable::FindNextBit1(u32 start_index)
{
	return FindNextBit(start_index, CBT_BIT_1);
}