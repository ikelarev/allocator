// MIT License
//
// Copyright (c) 2019 Ivan Kelarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "curl.h"

#include <cassert>
#include <curl/curl.h>

Curl::Curl(const std::string &proxy) : proxy_(proxy)
{
}

std::string Curl::HttpGet(const std::string &url) const
{
  std::string buffer;

  CURL *curl = curl_easy_init();
  if (curl)
  {
    CURLcode code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    assert(code == CURLE_OK);

    if (!proxy_.empty())
    {
      code = curl_easy_setopt(curl, CURLOPT_PROXY, proxy_.c_str());
      assert(code == CURLE_OK);
    }

    code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    assert(code == CURLE_OK);

    code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &AppendBuffer);
    assert(code == CURLE_OK);

    code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    assert(code == CURLE_OK);

    code = curl_easy_perform(curl);
    if (code != CURLE_OK)
    {
      buffer.clear();

      // TODO: that's not good to print anything here - error handling mechanism is required
      fprintf(stderr, "curl_easy_perform() failed: %s (%d)\n", curl_easy_strerror(code), code);
    }

    curl_easy_cleanup(curl);
  }

  return std::move(buffer);
}

size_t Curl::AppendBuffer(char *data, size_t size, size_t count, std::string *buffer)
{
  assert(buffer);
  buffer->append(data, size * count);
  return size * count;
}
