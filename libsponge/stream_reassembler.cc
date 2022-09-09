#include "stream_reassembler.hh"
#include <utility>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _capacity(capacity), _output(_capacity) {
  _stream.resize(_capacity);
}



// //! \details This function accepts a substring (aka a segment) of bytes,
// //! possibly out-of-order, from the logical stream, and assembles any newly
// //! contiguous substrings and writes them into the output stream in order.
// void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
//   size_t buf_end = _cur_idx + _output.remaining_capacity();
//   if (index >= buf_end)
//     return;

//   if (eof && index + data.size() <= buf_end) {
//     _end_idx = index + data.size();
//   }

//   for (auto i = max(index, _cur_idx); i < index + data.size() && i < buf_end && i < _end_idx; i++) {
//     if (!_occupied[i]) {
//       _stream[i - _cur_idx] = data[i - index];
//       _occupied[i] = true;
//       _unassembled++;
//     }
//   }
//   string bytes;
//   while (_occupied[_cur_idx]) {
//     bytes.push_back(_stream.front());
//     _occupied.erase(_cur_idx);
//     _stream.pop_front();
//     _stream.push_back(0);
//     _cur_idx++;
//   }
//   size_t len = bytes.size();
//   if (bytes.size() > 0) {
//     _output.write(bytes);
//     _unassembled -= len;
//   }

//   if (_cur_idx >= _end_idx)
//     _output.end_input();
// }

// size_t StreamReassembler::unassembled_bytes() const {
//   return _unassembled;
// }

// bool StreamReassembler::empty() const { return _unassembled == 0; }

bool StreamReassembler::push_substring_helper(const std::string &data, const size_t index) {
  if (index > _cur_idx) {
    return false;
  }

  string ww = data.substr(_cur_idx - index);
  // std::cout << "WRITE::" << ww << std::endl;
  size_t read = _output.write(ww);
  if (read != data.size() - _cur_idx + index) {
    _map[_cur_idx + read] = data.substr(_cur_idx + read - index);
  }
  _cur_idx += read;
  return true;
}
//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
  if (data.size() > 0) {
    if (data.size() + index <= _cur_idx || _cur_idx + _capacity <= index) {
      return;
    }
    if (push_substring_helper(data, index)) {
      for (auto it = _map.begin(); it != _map.end();) {
        if (it->first+it->second.size() <= _cur_idx) {
          it = _map.erase(it);
        } else if (!push_substring_helper(it->second, it->first)) {
          break;
        } else {
          it++;
        }
      }
    } else {
      if (_map.find(index) == _map.end() || _map[index].size() < data.size())
        _map[index] = data;
    }
  }
  if (eof) {
    //_eof = eof;
    _end_idx = index + data.size();
    //_output.end_input();
  }
  if (_cur_idx >= _end_idx) {
    _output.end_input();
  }
}


size_t StreamReassembler::unassembled_bytes() const {
  size_t tmp_idx = _cur_idx;
  size_t ans = 0;

  for (auto it = _map.begin(); it != _map.end(); it++) {
    size_t idx = it->first, sz = it->second.size();
    if (idx >= tmp_idx) {
      ans += sz;
      tmp_idx = idx + sz;
    } else if (tmp_idx < idx + sz) {
      ans += idx + sz - tmp_idx;
      tmp_idx = idx + sz;
    }
  }
  return ans;
}

bool StreamReassembler::empty() const { return _map.size(); }
