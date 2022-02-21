/*
 * Copyright (c) Microsoft. All rights reserved.
 * This code is licensed under the MIT License (MIT).
 *
 * THE CODE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE CODE OR THE USE OR OTHER DEALINGS IN THE CODE.
 */

#ifndef TOASTS_API_HPP
#define TOASTS_API_HPP

#include "NotificationActivator.hpp"

namespace Toasts {
	HRESULT ClearToasts(void);
#if 0
	HRESULT CreateToastXml(IXmlDocument **);
	HRESULT DisplayToast(void);
#endif
	HRESULT SendBasicToast(PCWSTR);
#ifdef HAVE_ATLSTR_H
	HRESULT SetImageSrc(PCWSTR, IXmlDocument *);
#endif
	HRESULT SetNodeValueString(HSTRING, IXmlNode *, IXmlDocument *);
	HRESULT SetTextValues(const PCWSTR *, UINT32, IXmlDocument *);
	HRESULT ShowToast(IXmlDocument *);
	void SendTestNotification(void);
}

#endif
