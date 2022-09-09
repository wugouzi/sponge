#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

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

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_fly; }

void TCPSender::fill_window() {
    if (_fin)
        return;

    size_t win_size = _ack_seqno + (_win_size == 0 ? 1 : _win_size) - next_seqno_absolute();
    // std::cout << "win: " << win_size << std::endl;
    while (win_size > 0 && !_fin) {
        TCPSegment seg;
        TCPHeader &head = seg.header();
        if (next_seqno_absolute() == 0) {
            head.syn = true;
            win_size--;
        }
        head.seqno = next_seqno();
        seg.payload() = stream_in().read(min(win_size, min(TCPConfig::MAX_PAYLOAD_SIZE, stream_in().buffer_size())));
        win_size -= seg.payload().size();
        if (stream_in().eof() && win_size > 0) {
            head.fin = true;
            win_size--;
            _fin = true;
        }
        size_t len = seg.length_in_sequence_space();
        if (len == 0)
            return;
        segments_out().push(seg);
        _segments_waiting.push(seg);
        if (!_timer_start) {
            _timer_start = true;
            _current_time = 0;
        }
        _next_seqno += len;
        _bytes_in_fly += len;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ack_seqno = unwrap(ackno, _isn, next_seqno_absolute());
    _win_size = window_size;
    if (ack_seqno > _next_seqno)
        return;
    _ack_seqno = ack_seqno;
    _retransmission_timeout = _initial_retransmission_timeout;

    _retransmission_cnt = 0;
    bool has_new = false;
    while (!_segments_waiting.empty()) {
        TCPSegment &seg = _segments_waiting.front();
        size_t len = seg.length_in_sequence_space();
        uint64_t seg_seqno_abs = unwrap(seg.header().seqno, _isn, next_seqno_absolute());
        if (seg_seqno_abs + len > _ack_seqno)
            break;
        _bytes_in_fly -= len;
        _segments_waiting.pop();
        _ack_seqno = ack_seqno;
        has_new = true;
    }

    // if (_segments_waiting.empty())
    //     _timer_start = false;

    // requirement 5
    if (has_new) {
        if (!_segments_waiting.empty()) {
            _timer_start = true;
            _current_time = 0;
        } else {
            _timer_start = false;
        }
        _retransmission_cnt = 0;
        _retransmission_timeout = _initial_retransmission_timeout;
    }

    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_start)
        return;
    _current_time += ms_since_last_tick;

    if (_current_time >= _retransmission_timeout) {
        if (!_segments_waiting.empty()) {
            _segments_out.push(_segments_waiting.front());
            // _segments_waiting.push(_segments_waiting.front());
            // _segments_waiting.pop();
            if (_win_size != 0) {
                _retransmission_cnt++;
                _retransmission_timeout *= 2;
            }
        }
        _current_time = 0;
        _timer_start = true;
    }

    // if (!_segments_waiting.empty() && _current_time >= _retransmission_timeout) {
    //     _segments_out.push(_segments_waiting.front());
    //     // _segments_waiting.push(_segments_waiting.front());
    //     // _segments_waiting.pop();
    //     if (!_win_zero_flag) {
    //         _retransmission_cnt++;
    //         _retransmission_timeout *= 2;
    //     }
    //     _current_time = 0;
    // }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_cnt; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
