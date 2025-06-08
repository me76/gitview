#pragma once

#include <array>

template<size_t itemSize, size_t itemCount>
class StaticMemoryPool
{
	struct MemBlock
	{
		bool mOccupied = false;
		char mBlock[itemSize];
	};

	std::array<MemBlock, itemCount> mPool;

public:
	void* allocate()
	{
		for(auto& block: mPool)
		{
			if(!block.mOccupied)
			{
				block.mOccupied = true;
				return block.mBlock;
			}
		}

		return malloc(itemSize);
	}

	void deallocate(void* p)
	{
		for(auto& block: mPool)
		{
			if(block.mBlock == p)
			{
				block.mOccupied = false;
				return;
			}
		}

		free(p);
	}
};

template<typename FileIt, size_t itemCount>
using FileIterPool = StaticMemoryPool<sizeof(FileIt), itemCount>;