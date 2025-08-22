#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32{static_cast<uint32_t>((n + zero_point.raw_value_) % (1ull << 32))};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // step1: 计算 base offset（在一个 2^32 周期内的位置）
  uint64_t base = (static_cast<uint64_t>(raw_value_) - zero_point.raw_value_) & 0xFFFFFFFFull;

  // step2: 找到 checkpoint 所在的周期
  uint64_t epoch = checkpoint & ~0xFFFFFFFFull;

  // step3: 构造三个候选值
  uint64_t candidate1 = epoch + base;                  // 当前周期
  uint64_t candidate2 = candidate1 + (1ull << 32);     // 下一周期
  uint64_t candidate3 = (candidate1 >= (1ull << 32)) ? candidate1 - (1ull << 32) : UINT64_MAX;  // 上一周期

  // step4: 从候选里选与 checkpoint 最近的
  uint64_t result = candidate1;
  if (llabs((long long)candidate2 - (long long)checkpoint) < llabs((long long)result - (long long)checkpoint))
    result = candidate2;
  if (candidate3 != UINT64_MAX && llabs((long long)candidate3 - (long long)checkpoint) < llabs((long long)result - (long long)checkpoint))
    result = candidate3;

  return result;
}


