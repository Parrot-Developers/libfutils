/******************************************************************************
 * Copyright (c) 2022 Parrot S.A.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Parrot Company nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE PARROT COMPANY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file string.hpp
 *
 * @brief String utilities for C++
 *
 *****************************************************************************/

#pragma once

#include <string>

namespace futils
{

namespace string
{

/**
 * @brief Check if the string fullstring starts with a given string
 * @param fullstring String to be checked
 * @param prefix String to test if it starts with
 * @return true if fullString starts with prefix or false otherwise
 */
bool startsWith(const std::string &fullString, const std::string &prefix);

/**
 * @brief Check if the string fullstring ends with a given string
 * @param fullstring String to be checked
 * @param suffix String to test if it ends with
 * @return true if fullString ends with suffix or false otherwise
 */
bool endsWith(const std::string &fullString, const std::string &suffix);

/**
 * @brief Convert all characters of a given string to lowercase
 * @param s String to-be-converted
 * @return A string with the same characters as s but in lowercase
 */
std::string convertToLowerCase(const std::string& s);

} // string

} // futils
