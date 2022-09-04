#include "stream_reassembler.hh"
#include <utility>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),
                                                              _map(), _cur_idx(0),
                                                              _end_idx(-1) {
}

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
    if (data.size() + index <= _cur_idx) {
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

size_t StreamReassembler::unassembled_bytes() {
  size_t tmp_idx = _cur_idx;
  size_t ans = 0;

  for (auto it = _map.begin(); it != _map.end();) {
    size_t idx = it->first, sz = it->second.size();
    if (idx >= tmp_idx) {
      ans += sz;
      tmp_idx = idx + sz;
      it++;
    } else if (tmp_idx < idx + sz) {
      ans += idx + sz - tmp_idx;
      tmp_idx = idx + sz;
      it++;
    } else {
      it = _map.erase(it);
    }
  }
  return ans;
}

bool StreamReassembler::empty() const { return _map.size(); }