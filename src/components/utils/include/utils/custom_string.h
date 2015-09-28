/*
 * Copyright (c) 2015, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_COMPONENTS_UTILS_INCLUDE_UTILS_CUSTOM_STRING_H_
#define SRC_COMPONENTS_UTILS_INCLUDE_UTILS_CUSTOM_STRING_H_

#include <string>

namespace utils {
namespace custom_string {
/* UTF8 formats:
 * UTF8 1 byte consists from one-byte sequence
 * bit representation of one character: 0xxxxxxx
 * UTF8 2 bytes consists from two-byte sequence
 * bit representation of one character: 110xxxxx 10xxxxxx
 * UTF8 3 bytes consists from three-byte sequence
 * bit representation of one character: 1110xxxx 10xxxxxx 10xxxxxx
 * UTF8 4 bytes consists from four-byte sequence
 * bit representation of one character: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
enum FormatOfUTF {
  kByteOfUTF8 = 0x80,  // 10xxxxxx
  kHigestByteOfUTF8Byte2 = 0xc0,  // 110xxxxx
  kHigestByteOfUTF8Byte3 = 0xe0,  // 1110xxxx
  kHigestByteOfUTF8Byte4 = 0xf0  // 11110xxx
};

class CustomString {
 public:
  CustomString();
  explicit CustomString(const std::string& str);
  explicit CustomString(const char* str);
  CustomString(size_t n, char c);

  /**
  * @brief Returns the length of the string, in terms of characters.
  */
  size_t size() const;

  /**
   * @brief Returns the length of the string, in terms of characters.
   */
  size_t length() const;

  /**
   * @brief Returns the length of the string, in terms of bytes.
   */
  size_t length_bytes() const;

  /**
   * @brief Returns TRUE if CustomString contains ASCII string
   * otherwise returns FALSE.
   */
  bool is_ascii_string() const;

  /**
   * @brief Returns TRUE if CustomString contains empty string
   * otherwise returns FALSE.
   */
  bool empty() const;

  /**
   * @brief Compares the value of the string (case sensitive).
   * @param Contains string for comparing
   * @return Returns TRUE if strings is equal otherwise returns FALSE.
   */
  bool operator==(const CustomString& str) const;

  /**
   * @brief Compares the value of the string (case sensitive).
   * @param Contains string for comparing
   * @return Returns TRUE if strings is equal otherwise returns FALSE.
   */
  bool operator==(const std::string& str) const;

  /**
   * @brief Concatenates string from CustomString with string from argument.
   * @param Contains string for concatenation.
   * @return Returns result of concatenation.
   */
  CustomString operator+(const CustomString& str) const;

  /**
   * @brief Concatenates string from CustomString with string from argument.
   * @param Contains string for concatenation.
   * @return Returns result of concatenation.
   */
  CustomString operator+(const std::string& str) const;

  /**
   * @brief Returns a reference to the character at position pos in the string.
   * @param pos value with the position of a character within the string.
   */
  char& at(size_t pos);

  /**
   * @brief Compares the value of the string (case sensitive).
   * @param Contains string for comparing
   * @return Returns TRUE if strings is equal otherwise returns FALSE.
   */
  int compare(const char* str) const;

  /**
   * @brief Compares the value of the string (case sensitive).
   * @param Contains string for comparing
   * @return Returns TRUE if strings is equal otherwise returns FALSE.
   */
  int compare(const std::string& str) const;

  /**
   * @brief Compares the value of the string (case insensitive).
   * @param Contains string for comparing
   * @return Returns TRUE if strings is equal otherwise returns FALSE.
   */
  bool CompareIgnoreCase(const CustomString& str) const;

  /**
   * @brief Compares the value of the string (case insensitive).
   * @param Contains string for comparing
   * @return Returns TRUE if strings is equal otherwise returns FALSE.
   */
  bool CompareIgnoreCase(const char* str) const;

  /**
   * @brief Returns a pointer to string from CustomString.
   */
  const char* c_str() const;

  /**
   * @brief Convert string to unicode string.
   * @return Returns unicode string.
   */
  std::wstring AsWString() const;

  /**
   * @brief Returns copy of string as multibyte string.
   */
  std::string AsMBString() const;

  /**
   * @brief Convert string to lower case unicode string.
   * @return Returns unicode string.
   */
  std::wstring AsWStringLowerCase() const;

 private:
  /**
   * @brief Convert string to lower case unicode string.
   * @param Will contain converted string.
   */
  void ConvertWStringToLowerCase(std::wstring& str) const;

  /**
   * @brief Convert string to unicode string.
   * @param Contains string for converting.
   * @return Returns unicode string.
   */
  std::wstring AsWString(const char* str) const;

  /**
   * @brief Convert string to unicode string.
   * @param Contains string for converting.
   * @return Returns unicode string.
   */
  size_t CalculateLengthOfString(const char* str) const;

  /**
   * @brief Initiate members of CustomString
   */
  void InitData();

  std::string mb_string_;
  size_t size_;
  bool is_ascii_string_;
};

}  // namespace custom_string
}  // namespace utils

#endif  // SRC_COMPONENTS_UTILS_INCLUDE_UTILS_CUSTOM_STRING_H_
