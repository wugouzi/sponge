#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _ackno(_isn)
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{_initial_retransmission_timeout}
    , _current_time(0)
    , _retransmission_cnt(0)
    , _timer_start(false)
    , _stream(capacity)
    , _next_seqno(0) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _ack_seqno; }

void TCPSender::fill_window() {
    if (_fin || _win_size == 0)
        return;
    if (_syn) {
        _syn = false;
        TCPSegment seg;
        seg.header().syn = 1;
        seg.header().seqno = wrap(_next_seqno, _isn);
        _win_size--;
        _next_seqno++;
        _segments_out.push(seg);
        _segments_waiting.push(seg);
    } else if (_stream.eof() && _stream.buffer_empty()) {
        _fin = true;
        TCPSegment seg;
        seg.header().fin = 1;
        seg.header().seqno = wrap(_next_seqno, _isn);
        
        _win_size--;
        _next_seqno++;
        _segments_out.push(seg);
        _segments_waiting.push(seg);
    } else {
        while (_win_size > 0 && !_stream.buffer_empty()) {
            TCPSegment seg;
            size_t seg_sz = min(_win_size, min(TCPConfig::MAX_PAYLOAD_SIZE, _stream.buffer_size()));
            seg.payload() = _stream.read(seg_sz);
            if (_stream.buffer_empty() && _stream.eof()&& _win_size > seg.length_in_sequence_space()) {
                seg.header().fin = 1;
                _fin = true;
            }
            seg.header().seqno = wrap(_next_seqno, _isn);
            _win_size -= seg_sz;
            _next_seqno += seg.length_in_sequence_space();
            _segments_out.push(seg);
            _segments_waiting.push(seg);
        }
    }
    if (!_timer_start) {
        _timer_start = true;
        _current_time = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _before_ack = false;
    uint64_t ack_seqno = unwrap(ackno, _isn, _next_seqno);
    _win_size = window_size + ack_seqno - _next_seqno;
    if (ack_seqno <= _ack_seqno || ack_seqno > _next_seqno)
        return;
    // _ack_seqno = ack_seqno;
    _retransmission_timeout = _initial_retransmission_timeout;
    
    _retransmission_cnt = 0;
    while (!_segments_waiting.empty()) {
        uint64_t seg_seqno_abs = unwrap(_segments_waiting.front().header().seqno, _isn, _next_seqno) + _segments_waiting.front().length_in_sequence_space();
        if (seg_seqno_abs <= ack_seqno) {
            _segments_waiting.pop();
            _ack_seqno = ack_seqno;
        } else
            break;
    }

    // requirement 5
    if (!_segments_waiting.empty()) {
        _timer_start = true;
        _current_time = 0;
    }

    if (window_size == 0) {
        _win_size = 1 + ack_seqno - _next_seqno;
        _win_zero_flag = true;
    } else {
        _win_zero_flag = false;
    }

    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_start)
        return;
    _current_time += ms_since_last_tick;

    if (!_segments_waiting.empty() && _current_time >= _retransmission_timeout) {
        _segments_out.push(_segments_waiting.front());
        // _segments_waiting.push(_segments_waiting.front());
        // _segments_waiting.pop();
        if (!_win_zero_flag) {
            _retransmission_cnt++;
            _retransmission_timeout *= 2;
        }
        _current_time = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_cnt; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
