#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
 }

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
 }

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes();
 }

size_t TCPConnection::time_since_last_segment_received() const { 
    return _current_time - _seg_time;
 }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    _seg_time = _current_time;
    const TCPHeader &head = seg.header();
    
    _receiver.segment_received(seg);
    if (head.rst /*|| (seg.header().ack && !_receiver.ackno().has_value())*/) {
        inbound_stream().set_error();
        _sender.stream_in().set_error();
        return;
    }
    if (head.syn && !_syn) 
        connect();
    if (head.ack && _sender.ack_received(head.ackno, head.win)) {
        if (_last_ack)
            _fin = true;
    }
    
    if (head.fin) {
        _receiver.stream_out().end_input();
        // seems can end input of sender
        if (!_fin) {
            _last_ack = true;
            _linger_after_streams_finish = false;
        }
    }
    
    // if the incoming segment occupied any sequence numbers, 
    // the TCPConnection makes sure that at least one segment is sent in reply, 
    // to reflect an update in the ackno and window size.
    // NOT SEND ACK IN FIN_WAIT_2
    if (seg.length_in_sequence_space() > 0 && _segments_out.empty())
        _sender.send_empty_segment();
    if (seg.length_in_sequence_space() == 0 && _receiver.ackno().has_value() && seg.header().seqno == _receiver.ackno().value() - 1)
        _sender.send_empty_segment();
    write_segment();
 }

bool TCPConnection::active() const {
    if (_receiver.stream_out().error() || _sender.stream_in().error())
        return false;
    if (!_receiver.ackno().has_value())
        return true;

    if (_sender.stream_in().eof() && _receiver.stream_out().eof() 
        && _segments_out.empty() 
        && _receiver.ackno().value().raw_value() >= _sender.next_seqno().raw_value()
        && !_linger_after_streams_finish && _fin) {
            return false;
        }
    return true;
}

size_t TCPConnection::write(const string &data) {
    size_t sz = _sender.stream_in().write(data);
    _sender.fill_window();
    write_segment();
    return sz;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _current_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _stop_send = true;
    }
    write_segment();
    if (_current_time >= _seg_time + 10 * _cfg.rt_timeout && _sender.stream_in().eof() && _receiver.stream_out().eof()) {
        _linger_after_streams_finish = false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    if (!_last_ack)
        _fin = true;
    write_segment();
}

void TCPConnection::connect() {
    _syn = true;
    _sender.fill_window();
    write_segment();
}

void TCPConnection::write_segment() {
    if (_stop_send)
        return;
    while (!_sender.segments_out().empty()) {
        TCPSegment &seg = _sender.segments_out().front();
        if (seg.header().rst) {
            _stop_send = true;
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            break;
        }
        if (_receiver.ackno().has_value()) {
            seg.header().ack = 1;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(_sender.segments_out().front());
        _sender.segments_out().pop();
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            TCPSegment seg;
            seg.header().rst = 1;
            _segments_out.push(seg);
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
