#include "wrapping_integers.hh"
#include <cstdint>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
  uint64_t base = n % (1ul << 32);
  if ((1ul << 32) - base < isn.raw_value())
    return WrappingInt32(isn.raw_value() + base - (1ul << 32));
  return WrappingInt32(isn.raw_value() + base);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
  uint64_t tmp = 0;
  uint64_t base = checkpoint % (1ul << 32);
  uint64_t add = checkpoint >> 32 << 32;
  if (n.raw_value() >= isn.raw_value()) {
    tmp = n.raw_value() - isn.raw_value();
  } else {
    tmp = (1ul << 32) + n.raw_value() - isn.raw_value();
  }

  if (base <= tmp) {
    if (add > 0 && base + (1ul << 32) - tmp < tmp - base)
      return add + tmp - (1ul << 32);
    return add + tmp;
  }
  if (base - tmp < tmp + (1ul << 32) - base)
    return add + tmp;
  return add + tmp + (1ul << 32);
  
}
