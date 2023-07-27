/**************************************************************************/
/*  func_redirect.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             EIRTeam.FFmpeg                             */
/*                         https://ph.eirteam.moe                         */
/**************************************************************************/
/* Copyright (c) 2023-present Álex Román (EIRTeam) & contributors.        */
/*                                                                        */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef FUNC_REDIRECT_H
#define FUNC_REDIRECT_H

#ifdef GDEXTENSION
#define STREAM_FUNCNAME(funcname) _##funcname
#else
#define STREAM_FUNCNAME(funcname) funcname
#endif

#define STREAM_FUNC_REDIRECT_0_(retval, funcname, const) \
	virtual retval STREAM_FUNCNAME(funcname)() const override { return funcname##_internal(); };

#define STREAM_FUNC_REDIRECT_1_(retval, funcname, const, arg0type, arg0name) \
	virtual retval STREAM_FUNCNAME(funcname)(arg0type arg0name) const override { return funcname##_internal(arg0name); };

#define STREAM_FUNC_REDIRECT_0(retval, funcname) STREAM_FUNC_REDIRECT_0_(retval, funcname, );
#define STREAM_FUNC_REDIRECT_0_CONST(retval, funcname) STREAM_FUNC_REDIRECT_0_(retval, funcname, const);

#define STREAM_FUNC_REDIRECT_1(retval, funcname, arg0type, arg0name) STREAM_FUNC_REDIRECT_1_(retval, funcname, , arg0type, arg0name);
#define STREAM_FUNC_REDIRECT_1_CONST(retval, funcname, arg0type, arg0name) STREAM_FUNC_REDIRECT_1_(retval, funcname, const, arg0type, arg0name);

#endif // FUNC_REDIRECT_H
