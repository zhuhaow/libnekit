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

#pragma once

#include <boost/algorithm/searching/knuth_morris_pratt.hpp>
#include <boost/noncopyable.hpp>

#include "../data_flow/data_flow_interface.h"

namespace nekit {
namespace utils {
class StreamReader : public AsyncInterface, private boost::noncopyable {
 public:
  StreamReader(data_flow::DataFlowInterface* data_flow);
  ~StreamReader();

  utils::Cancelable ReadToLength(
      size_t length, data_flow::DataFlowInterface::DataEventHandler handler);

  utils::Cancelable ReadToPattern(
      std::string pattern,
      data_flow::DataFlowInterface::DataEventHandler handler);

  utils::Buffer ConsumeRemainData();

  Runloop* GetRunloop() override;

 private:
  void DoReadLength();
  void DoReadPattern();

  data_flow::DataFlowInterface* data_flow_;

  utils::Buffer buffer_;
  utils::Cancelable cancelable_, data_flow_cancelable_;

  data_flow::DataFlowInterface::DataEventHandler handler_;

  size_t length_to_read_;

  size_t last_pos_;
  size_t pattern_size_;
  std::unique_ptr<
      boost::algorithm::knuth_morris_pratt<std::string::const_iterator>>
      pattern_matcher_;
};
}  // namespace utils
}  // namespace nekit
