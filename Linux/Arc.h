#pragma once

#include <cstdint>

constexpr auto ARC_MAX_BITS = 12;

constexpr auto CT_NONE = 1;
constexpr auto CT_7_BIT = 2;
constexpr auto CT_8_BIT = 3;

#pragma pack(push)
#pragma pack(1)

class CArcEntry
{
  public:
    CArcEntry *next;
    uint16_t basecode;
    uint8_t ch;
    uint8_t pad;
};

class CArcCtrl //control structure
{
  public:
    CArcCtrl(uint32_t expand, uint32_t compression_type);
    ~CArcCtrl();

    void ArcEntryGet();
    void ArcExpandBuf();
  public:
    uint32_t src_pos;
    uint32_t src_size;
    uint32_t dst_pos;
    uint32_t dst_size;
    uint8_t *src_buf;
    uint8_t *dst_buf;
    uint32_t min_bits;
    uint32_t min_table_entry;
    CArcEntry *cur_entry;
    CArcEntry *next_entry;
    uint32_t cur_bits_in_use, next_bits_in_use;
    uint8_t *stk_ptr, *stk_base;
    uint32_t free_idx;
    uint32_t free_limit;
    uint32_t saved_basecode;
    uint32_t entry_used;
    uint32_t last_ch;
    CArcEntry compress[1 << ARC_MAX_BITS];
    CArcEntry *hash[1 << ARC_MAX_BITS];
};

class CArcCompress
{
  public:
    uint8_t *ExpandBuf();

  public:
    uint32_t compressed_size;
	uint32_t compressed_size_hi;
	uint32_t expanded_size;
	uint32_t expanded_size_hi;
    uint8_t compression_type;
    uint8_t body[1];
};

#pragma pack(pop)
