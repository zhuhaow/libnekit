// MIT License

// Copyright (c) 2018 Zhuhao Wang

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "nekit/utils/stream_reader.h"

#include <boost/asio/buffer.hpp>

namespace nekit {
namespace utils {
StreamReader::StreamReader(data_flow::DataFlowInterface* data_flow)
    : data_flow_{data_flow} {}

StreamReader::~StreamReader() {
  data_flow_cancelable_.Cancel();
  cancelable_.Cancel();
}

Cancelable StreamReader::ReadToLength(
    size_t length, data_flow::DataFlowInterface::DataEventHandler handler) {
  BOOST_ASSERT(length);

  cancelable_ = Cancelable();
  handler_ = handler;
  length_to_read_ = length;

  DoReadLength();

  return cancelable_;
}

void StreamReader::DoReadLength() {
  if (buffer_.size() >= length_to_read_) {
    Buffer b = std::move(buffer_);
    if (b.size() != length_to_read_) {
      buffer_ = b.Break(length_to_read_);
    };

    GetRunloop()->Post([cancelable{cancelable_}, handler{handler_},
                        buffer{std::move(b)}]() mutable {
      if (cancelable.canceled()) {
        return;
      }

      handler(std::move(buffer));
    });

    handler_ = nullptr;
    return;
  } else {
    data_flow_cancelable_ = data_flow_->Read([this](Result<Buffer>&& buffer) {
      if (cancelable_.canceled()) {
        return;
      }

      if (!buffer) {
        handler_(MakeErrorResult(std::move(buffer).error()));
        return;
      }

      buffer_.InsertBack(*std::move(buffer));
      DoReadLength();
    });
  }
}

Cancelable StreamReader::ReadToPattern(
    std::string pattern,
    data_flow::DataFlowInterface::DataEventHandler handler) {
  BOOST_ASSERT(pattern.size());

  cancelable_ = Cancelable();
  handler_ = handler;
  pattern_matcher_ = std::make_unique<
      boost::algorithm::knuth_morris_pratt<std::string::const_iterator>>(
      pattern.begin(), pattern.end());
  last_pos_ = 0;
  pattern_size_ = pattern.size();

  DoReadPattern();

  return cancelable_;
}

void StreamReader::DoReadPattern() {
  std::vector<boost::asio::const_buffer> buffers;
  buffer_.WalkInternalChunk(
      [&](const void* data, size_t len, void* context) {
        (void)context;
        buffers.emplace_back(data, len);
        return true;
      },
      last_pos_, nullptr);

  auto result = (*pattern_matcher_)(boost::asio::buffers_begin(buffers),
                                    boost::asio::buffers_end(buffers))
                    .first;
  if (result == boost::asio::buffers_end(buffers)) {
    last_pos_ = buffer_.size() - std::min(buffer_.size(), pattern_size_);

    data_flow_cancelable_ = data_flow_->Read([this](Result<Buffer>&& buffer) {
      if (cancelable_.canceled()) {
        return;
      }

      if (!buffer) {
        handler_(MakeErrorResult(std::move(buffer).error()));
        return;
      }
      buffer_.InsertBack(*std::move(buffer));
      DoReadPattern();
    });
  } else {
    size_t len = std::distance(boost::asio::buffers_begin(buffers), result) +
                 pattern_size_ + last_pos_;

    Buffer b = std::move(buffer_);
    if (b.size() != len) {
      buffer_ = b.Break(len);
    };

    GetRunloop()->Post([cancelable{cancelable_}, handler{handler_},
                        buffer{std::move(b)}]() mutable {
      if (cancelable.canceled()) {
        return;
      }

      handler(std::move(buffer));
    });

    handler_ = nullptr;
  }
}

Buffer StreamReader::ConsumeRemainData() { return std::move(buffer_); }

}  // namespace utils
}  // namespace nekit
