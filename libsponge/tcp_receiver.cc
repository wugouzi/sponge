  #include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <cstdint>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
  if (!_isn_recv && !seg.header().syn)
    return;
  if (seg.header().syn) {
    _isn = seg.header().seqno;
    _isn_recv = true;
  }
  
  uint64_t ackno = _reassembler.stream_out().bytes_read() + 1;
  uint64_t idx = unwrap(seg.header().seqno, _isn, ackno) - 1 + seg.header().syn;

  // uint64_t idx = _reassembler.stream_out().bytes_read() + seg.length_in_sequence_space() - seg.header().syn;
  _reassembler.push_substring(seg.payload().copy(), idx, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
  if (!_isn_recv) {
    return nullopt;
  }
  uint64_t n = _reassembler.stream_out().bytes_written() + 1;
  if (_reassembler.stream_out().input_ended())
    n++;
  return wrap(n, _isn);
}

size_t TCPReceiver::window_size() const { 
  return _capacity - _reassembler.stream_out().buffer_size();
}
