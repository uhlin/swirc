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

#ifndef DESKTOPTOASTSAPP_HPP
#define DESKTOPTOASTSAPP_HPP

#include "NotificationActivator.hpp"

class DesktopToastsApp {
public:
    static const UINT WM_USER_OPENWINDOWIFNEEDED = WM_USER;

    static DesktopToastsApp *GetInstance() {
	return s_currentInstance;
    }

    static HRESULT DesktopToastsApp::SendBasicToast(PCWSTR message);
    static void DesktopToastsApp::SendTestNotification();

    DesktopToastsApp();
    ~DesktopToastsApp();
    HRESULT Initialize(HINSTANCE hInstance);
    HRESULT OpenWindowIfNeeded();
    bool HasWindow();
    void RunMessageLoop();
    void SetMessage(PCWSTR message);

    void SetHInstance(HINSTANCE hInstance) {
	m_hInstance = hInstance;
    }

private:
    static LRESULT CALLBACK
    WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT DisplayToast();
    HRESULT ClearToasts();

    HRESULT CreateToastXml(IXmlDocument **toastXml);

    static HRESULT ShowToast(IXmlDocument *xml);
    static HRESULT
    DesktopToastsApp::SetImageSrc(PCWSTR imagePath, IXmlDocument *toastXml);
    static HRESULT DesktopToastsApp::SetTextValues(
	const PCWSTR *textValues,
	UINT32 textValuesCount,
	IXmlDocument *toastXml);
    static HRESULT DesktopToastsApp::SetNodeValueString(
	HSTRING onputString,
	IXmlNode *node,
	IXmlDocument *xml);

    HINSTANCE m_hInstance;
    HWND m_hwnd = nullptr;
    HWND m_hEdit = nullptr;
    DWORD m_threadId;

    static const WORD HM_POPTOASTBUTTON = 1;
    static const WORD HM_CLEARTOASTSBUTTON = 2;

    static DesktopToastsApp *s_currentInstance;
};

#endif
