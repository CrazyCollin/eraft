// MIT License

// Copyright (c) 2022 eraft dev group

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

#include <network/file_sys.h>

namespace network {

bool FileSys::IsAbsolutePath(const std::string &path) {
  // NOT SUPPORT
  return false;
}

std::string FileSys::GetExeImagePath() {
  // NOT SUPPORT
  return "";
}

std::string FileSys::GetExeParentDirectory() {
  // NOT SUPPORT
  return "";
}

void FileSys::ListDirectory(const std::string &directory,
                            std::vector<std::string> &list) {
  // NOT SUPPORT
}

bool FileSys::CreateDirectory(const std::string &directory) {
  // NOT SUPPORT
  return true;
}

bool FileSys::DeleteDirectory(const std::string &directory) {
  // NOT SUPPORT
  return true;
}

}  // namespace network
