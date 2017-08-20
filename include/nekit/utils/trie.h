// MIT License

// Copyright (c) 2017 Zhuhao Wang

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

#include <array>
#include <string>

#include <boost/range/adaptors.hpp>
#include <boost/noncopyable.hpp>

namespace nekit {
namespace utils {
template <typename CharT, CharT offset, CharT node_size, bool reverse = false>
class Trie : boost::noncopyable {
 public:
  bool AddPrefix(const std::basic_string<CharT>& prefix) {
    if (prefix.size() == 0) {
      return false;
    }

    for (const auto& ch : prefix) {
      if (ch < offset || ch >= offset + node_size) {
        return false;
      }
    }

    TrieNode* current_node = &root_;
    if (reverse) {
      for (const auto& ch : boost::adaptors::reverse(prefix)) {
        if (!current_node->nodes_[ch - offset]) {
          current_node->nodes_[ch - offset] = std::make_unique<TrieNode>();
        }
        current_node = current_node->nodes_[ch - offset].get();
      }
    } else {
      for (const auto& ch : prefix) {
        if (!current_node->nodes_[ch - offset]) {
          current_node->nodes_[ch - offset] = std::make_unique<TrieNode>();
        }
        current_node = current_node->nodes_[ch - offset].get();
      }
    }
    current_node->end_ = true;
    return true;
  }

  bool MatchPrefixWith(const std::basic_string<CharT>& literal) {
    if (literal.size() == 0) {
      return false;
    }

    TrieNode* current_node = &root_;
    if (reverse) {
      for (const auto& ch : boost::adaptors::reverse(literal)) {
        if (ch < offset || ch >= offset + node_size) {
          return false;
        }

        current_node = current_node->nodes_[ch - offset].get();
        if (!current_node) {
          return false;
        }

        if (current_node->end_) {
          return true;
        }
      }
    } else {
      for (const auto& ch : literal) {
        if (ch < offset || ch >= offset + node_size) {
          return false;
        }

        current_node = current_node->nodes_[ch - offset].get();
        if (!current_node) {
          return false;
        }

        if (current_node->end_) {
          return true;
        }
      }
    }

    return false;
  }

 private:
  class TrieNode : boost::noncopyable {
   public:
    std::array<std::unique_ptr<TrieNode>, node_size> nodes_;
    bool end_{false};
  };

  TrieNode root_;
};

template <bool reverse = false>
using DomainTrie = Trie<char, '-', 78, reverse>;

using ReverseDomainTrie = DomainTrie<true>;
}  // namespace utils
}  // namespace nekit
