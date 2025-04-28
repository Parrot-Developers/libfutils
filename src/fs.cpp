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
 * @file fs.cpp
 *
 * @brief C++ filesystem utilities
 *
 ******************************************************************************/

#include <libgen.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <string>

#include "futils/futils.hpp"

namespace futils
{

namespace fs
{

std::string dirname(const std::string &path)
{
	char *str = strdup(path.c_str());
	std::string res(::dirname(str));
	free(str);
	return res;
}

/* libgen.h redefine basename to __xpg_basename, if we want to have the correct
 * symbol name without the preprocessor messing up stuff, first setup a wrapper
 * and then undef the symbol
 */

static char *wrap_basename(char *path)
{
	return ::basename(path);
}
#undef basename

std::string basename(const std::string &path)
{
	char *str = strdup(path.c_str());
	std::string res(wrap_basename(str));
	free(str);
	return res;
}

}

}
