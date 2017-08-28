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

#ifndef NEKIT_TUNNEL_BUFFER_SIZE
#define NEKIT_TUNNEL_BUFFER_SIZE 8192
#endif

#ifndef NEKIT_TUNNEL_MAX_BUFFER_SIZE
#define NEKIT_TUNNEL_MAX_BUFFER_SIZE (NEKIT_TUNNEL_BUFFER_SIZE * 2)
#endif

// There is no good choice there, the server defaults (e.g., Apache, nginx) are
// usually quite large and only define the maximum length of each line instead
// of the whole header. In Node.js it is defined as 80 * 1024. Enlarge it if the
// default is too small.
#ifndef NEKIT_HTTP_HEADER_MAX_LENGTH
#define NEKIT_HTTP_HEADER_MAX_LENGTH 8192
#endif

#ifndef NEKIT_HTTP_HEADER_MAX_FIELD
#define NEKIT_HTTP_HEADER_MAX_FIELD 100
#endif
