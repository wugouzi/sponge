#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): _capacity(capacity) {
}

size_t ByteStream::write(const string &data) {
  if (_end) {
    return 0;
  }
  size_t res = min(data.size(), _capacity - _buf_size);
  _queue.append(BufferList(move(string().assign(data.begin(), data.begin() + res))));
  _buf_size += res;
  _byte_write += res;
  return res;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
  string str = _queue.concatenate();
  return string().assign(str.begin(), str.begin() + min(len, str.size()));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
  const size_t sz = min(len, _buf_size);
  _byte_read += sz;
  _buf_size -= sz;
  _queue.remove_prefix(sz);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
  const size_t sz = min(len, _buf_size);
  _buf_size -= sz;
  _byte_read += sz;
  string str = _queue.concatenate();
  _queue.remove_prefix(sz);
  return str.assign(str.begin(), str.begin() + sz);
}

void ByteStream::end_input() {
  _end = true;
}

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return _buf_size; }

bool ByteStream::buffer_empty() const { return _buf_size == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _byte_write; }

size_t ByteStream::bytes_read() const { return _byte_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buf_size; }
