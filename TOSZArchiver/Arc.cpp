#include "Arc.h"
#include <cstdlib>
#include <cstring>

void CArcCtrl::ArcEntryGet()
{
  CArcCtrl *c = this;
  uint32_t i;
  CArcEntry *tmp, *tmp1;

  if (this->entry_used)
  {
    i = this->free_idx;

    this->entry_used = false;
    this->cur_entry = this->next_entry;
    this->cur_bits_in_use = this->next_bits_in_use;
    if (this->next_bits_in_use < ARC_MAX_BITS)
    {
      this->next_entry = &this->compress[i++];
      if (i == this->free_limit)
      {
        this->next_bits_in_use++;
        this->free_limit = 1 << this->next_bits_in_use;
      }
    }
    else
    {
      do
        if (++i == this->free_limit)
          i = this->min_table_entry;
      while (this->hash[i]);
      tmp = &this->compress[i];
      this->next_entry = tmp;
      tmp1 = (CArcEntry *)&this->hash[tmp->basecode];
      while (tmp1 && tmp1->next != tmp)
        tmp1 = tmp1->next;
      if (tmp1)
        tmp1->next = tmp->next;
    }
    this->free_idx = i;
  }
}


static inline int Bt(int bit_num, uint8_t *bit_field)
{
	bit_field += bit_num >> 3;
	bit_num &= 7;
	return (*bit_field & (1 << bit_num)) ? 1 : 0;
}

static inline int Bts(int bit_num, uint8_t *bit_field)
{
	int res;
	bit_field += bit_num >> 3;
	bit_num &= 7;
	res = *bit_field & (1 << bit_num);
	*bit_field |= (1 << bit_num);
	return (res) ? 1 : 0;
}

static uint32_t BFieldExtDWORD(uint8_t *src, uint32_t pos, uint32_t bits)
{
	uint32_t i, res = 0;
	for (i = 0; i < bits; i++)
		if (Bt(pos + i, src))
			Bts(i, (uint8_t *)&res);
	return res;
}

void CArcCtrl::ArcExpandBuf()
{
  uint8_t *dst_ptr, *dst_limit;
  uint32_t basecode, lastcode, code;
  CArcEntry *tmp, *tmp1;

  dst_ptr = this->dst_buf + this->dst_pos;
  dst_limit = this->dst_buf + this->dst_size;

  while (dst_ptr < dst_limit && this->stk_ptr != this->stk_base)
    *dst_ptr++ = *--this->stk_ptr;

  if (this->stk_ptr == this->stk_base && dst_ptr < dst_limit)
  {
    if (this->saved_basecode == 0xFFFFFFFFl)
    {
      lastcode = BFieldExtDWORD(this->src_buf, this->src_pos,
                                this->next_bits_in_use);
      this->src_pos = this->src_pos + this->next_bits_in_use;
      *dst_ptr++ = lastcode;
      this->ArcEntryGet();
      this->last_ch = lastcode;
    }
    else
      lastcode = this->saved_basecode;
    while (dst_ptr < dst_limit && this->src_pos + this->next_bits_in_use <= this->src_size)
    {
      basecode = BFieldExtDWORD(this->src_buf, this->src_pos,
                                this->next_bits_in_use);
      this->src_pos = this->src_pos + this->next_bits_in_use;
      if (this->cur_entry == &this->compress[basecode])
      {
        *this->stk_ptr++ = this->last_ch;
        code = lastcode;
      }
      else
        code = basecode;
      while (code >= this->min_table_entry)
      {
        *this->stk_ptr++ = this->compress[code].ch;
        code = this->compress[code].basecode;
      }
      *this->stk_ptr++ = code;
      this->last_ch = code;

      this->entry_used = true;
      tmp = this->cur_entry;
      tmp->basecode = lastcode;
      tmp->ch = this->last_ch;
      tmp1 = (CArcEntry *)&this->hash[lastcode];
      tmp->next = tmp1->next;
      tmp1->next = tmp;

      this->ArcEntryGet();
      while (dst_ptr < dst_limit && this->stk_ptr != this->stk_base)
        *dst_ptr++ = *--this->stk_ptr;
      lastcode = basecode;
    }
    this->saved_basecode = lastcode;
  }
  this->dst_pos = dst_ptr - this->dst_buf;
}

CArcCtrl::CArcCtrl(uint32_t expand, uint32_t compression_type)
{
  if (expand)
  {
    stk_base = (uint8_t *)malloc(1 << ARC_MAX_BITS);
    stk_ptr = this->stk_base;
  }
  if (compression_type == CT_7_BIT)
    min_bits = 7;
  else
    min_bits = 8;
  min_table_entry = 1 << this->min_bits;
  free_idx = this->min_table_entry;
  next_bits_in_use = this->min_bits + 1;
  free_limit = 1 << this->next_bits_in_use;
  saved_basecode = 0xFFFFFFFFl;
  entry_used = true;
  ArcEntryGet();
  entry_used = true;
}

CArcCtrl::~CArcCtrl()
{
  free(this->stk_base);
}

uint8_t *CArcCompress::ExpandBuf()
{
  CArcCtrl *c;
  uint8_t *res;

  if (!(CT_NONE <= this->compression_type && this->compression_type <= CT_8_BIT) ||
	  this->expanded_size >= 0x20000000l)
    return NULL;

  res = (uint8_t *)malloc(this->expanded_size + 1);
  res[this->expanded_size] = 0; //terminate
  switch (this->compression_type)
  {
  case CT_NONE:
    memcpy(res, this->body, this->expanded_size);
    break;
  case CT_7_BIT:
  case CT_8_BIT:
    c = new CArcCtrl(true, this->compression_type);
    c->src_size = this->compressed_size * 8;
    c->src_pos = (sizeof(CArcCompress) - 1) * 8;
    c->src_buf = (uint8_t *)this;
    c->dst_size = this->expanded_size;
    c->dst_buf = res;
    c->dst_pos = 0;
    c->ArcExpandBuf();
    delete c;
    break;
  }
  return res;
}