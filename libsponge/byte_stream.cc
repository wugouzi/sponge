#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): capacity_(capacity), queue_(std::deque<char>()),
                                               wc_(0), rc_(0), end_(false) {
}

size_t ByteStream::write(const string &data) {
  if (end_) {
    return 0;
  }
  int res = 0;
  for (auto& c : data) {
    if (queue_.size() < capacity_) {
      queue_.push_back(c);
      wc_++;
      res++;
    }
  }
  return res;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
  string ans;
  for (size_t i = 0; i < len; i++) {
    ans.push_back(queue_[i]);
  }
  return ans;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t _len) {
  int len = _len;
  while (len && !queue_.empty()) {
    queue_.pop_front();
    len--;
    rc_++;
  }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t _len) {
  string ans;
  int len = _len;
  while (len-- && !queue_.empty()) {
    ans.push_back(queue_.front());
    queue_.pop_front();
    rc_++;
  }
  return ans;
}

void ByteStream::end_input() {
  end_ = true;
}

bool ByteStream::input_ended() const { return end_; }

size_t ByteStream::buffer_size() const { return queue_.size(); }

bool ByteStream::buffer_empty() const { return queue_.empty(); }

bool ByteStream::eof() const { return end_ && queue_.empty(); }

size_t ByteStream::bytes_written() const { return wc_; }

size_t ByteStream::bytes_read() const { return rc_; }

size_t ByteStream::remaining_capacity() const { return capacity_-queue_.size(); }
