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

#include "common.h"

#include <SDKDDKVer.h>
#include <atlstr.h>
#include <string>
#include <windows.ui.notifications.h>
#include <wrl.h>
#include <wrl\wrappers\corewrappers.h>

#include "DesktopNotificationManagerCompat.hpp"
#include "DesktopToastsApp.hpp"
#include "errHand.h"

#define RETURN_IF_FAILED(hr)\
	do {\
		HRESULT _hrTemp = hr;\
\
		if (FAILED(_hrTemp)) {\
			return _hrTemp;\
		}\
	} while (false)

DesktopToastsApp *DesktopToastsApp::s_currentInstance = nullptr;

HRESULT
DesktopToastsApp::OpenWindowIfNeeded()
{
    const bool no_window_exists = m_hwnd == nullptr;

    if (no_window_exists) {
	const bool not_on_main_ui_thread = m_threadId != GetCurrentThreadId();

	if (not_on_main_ui_thread) {
	    /*
	     * We have to initialize on UI thread so that the message
	     * loop is handled correctly
	     */

	    HANDLE h = CreateEvent(nullptr, 0, 0, nullptr);
	    PostThreadMessage(
		m_threadId,
		DesktopToastsApp::WM_USER_OPENWINDOWIFNEEDED,
		0,
		(LPARAM) h);
	    WaitForSingleObject(h, INFINITE);
	    return S_OK;
	} else {
	    /*
	     * Otherwise, create the window
	     */

	    return Initialize(m_hInstance);
	}
    } else { /* no_window_exists */
	/*
	 * Otherwise, ensure window is unminimized and in the foreground
	 */

	::ShowWindow(m_hwnd, SW_RESTORE);
	::SetForegroundWindow(m_hwnd);

	return S_OK;
    }
}

bool
DesktopToastsApp::HasWindow()
{
    return m_hwnd != nullptr;
}

DesktopToastsApp::DesktopToastsApp()
{
    s_currentInstance = this;
    m_threadId = GetCurrentThreadId();
}

DesktopToastsApp::~DesktopToastsApp()
{
    s_currentInstance = nullptr;
}

/**
 * Prepare the main window
 */
HRESULT
DesktopToastsApp::Initialize(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = { sizeof (wcex) };

    /* Register window class */
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = DesktopToastsApp::WndProc;
    wcex.cbWndExtra    = sizeof (LONG_PTR);
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DesktopToastsApp";
    wcex.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    ::RegisterClassEx(&wcex);

    /*
     * Create window
     */
    m_hwnd = CreateWindow(
	L"DesktopToastsApp",
	L"Swirc",
	WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT, /* x */
	CW_USEDEFAULT, /* y */
	350, /* nWidth */
	200, /* nHeight */
	nullptr,
	nullptr,
	hInstance,
	this);

    if (!m_hwnd)
	return E_FAIL;

    ::CreateWindow(
        L"BUTTON",
        L"View Text Toast",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
        12, 12, 150, 25,
        m_hwnd, reinterpret_cast<HMENU>(HM_POPTOASTBUTTON),
        hInstance, nullptr);
    ::CreateWindow(
        L"BUTTON",
        L"Clear toasts",
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
        174, 12, 150, 25,
        m_hwnd, reinterpret_cast<HMENU>(HM_CLEARTOASTSBUTTON),
        hInstance, nullptr);
    m_hEdit = ::CreateWindow(
	L"EDIT",
	L"Whatever action you take on the displayed toast will be shown here.",
	ES_LEFT | ES_MULTILINE | ES_READONLY | WS_CHILD | WS_VISIBLE | WS_BORDER,
	12, 49, 300, 50,
	m_hwnd, nullptr,
	hInstance, nullptr);

    ::ShowWindow(m_hwnd, SW_SHOWNORMAL);
    ::UpdateWindow(m_hwnd);
    ::SetForegroundWindow(m_hwnd);

    return S_OK;
}

/**
 * Standard message loop
 */
void
DesktopToastsApp::RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0) != 0) {
	if (msg.message == DesktopToastsApp::WM_USER_OPENWINDOWIFNEEDED) {
	    OpenWindowIfNeeded();
	    SetEvent((HANDLE) msg.lParam);
	} else {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }
}

void
DesktopToastsApp::SetMessage(PCWSTR message)
{
    ::SendMessage(m_hEdit,
		  WM_SETTEXT,
		  reinterpret_cast<WPARAM>(nullptr),
		  reinterpret_cast<LPARAM>(message));
}

/**
 * Display the toast using classic COM. Note that is also possible to
 * create and display the toast using the new C++ /ZW options.
 */
HRESULT
DesktopToastsApp::DisplayToast()
{
    ComPtr<IXmlDocument> toastXml;

    /*
     * Create the toast content
     */
    RETURN_IF_FAILED(CreateToastXml(&toastXml));

    /*
     * And show it...
     */
    return ShowToast(toastXml.Get());
}

/**
 * Create the toast XML from a template
 */
HRESULT
DesktopToastsApp::CreateToastXml(IXmlDocument **toastXml)
{
    ComPtr<IXmlDocument> doc;

    RETURN_IF_FAILED(
	DesktopNotificationManagerCompat::CreateXmlDocumentFromString(
	    L"<toast>"
	    L"<visual>"
	    L"<binding template=\"ToastGeneric\">"
	    L"<text></text>"
	    L"<text></text>"
	    L"<text></text>"
	    L"</binding>"
	    L"</visual>"
	    L"</toast>",
	    &doc));

    PCWSTR textValues[] = {
	L"Foo",
	L"Bar",
	L"Baz"
    };

    SetTextValues(textValues, ARRAYSIZE(textValues), doc.Get());

    return doc.CopyTo(toastXml);
}

/**
 * Set the value of the "src" attribute of the "image" node
 */
HRESULT
DesktopToastsApp::SetImageSrc(PCWSTR imagePath, IXmlDocument *toastXml)
{
    wchar_t imageSrcUri[MAX_PATH];
    DWORD size = ARRAYSIZE(imageSrcUri);

    RETURN_IF_FAILED(::UrlCreateFromPath(imagePath, imageSrcUri, &size, 0));

    ComPtr<IXmlNodeList> nodeList;
    RETURN_IF_FAILED(
	toastXml->GetElementsByTagName(HStringReference(L"image").Get(),
				       &nodeList));

    ComPtr<IXmlNode> imageNode;
    RETURN_IF_FAILED(nodeList->Item(0, &imageNode));

    ComPtr<IXmlNamedNodeMap> attributes;
    RETURN_IF_FAILED(imageNode->get_Attributes(&attributes));

    ComPtr<IXmlNode> srcAttribute;
    RETURN_IF_FAILED(
	attributes->GetNamedItem(HStringReference(L"src").Get(),
				 &srcAttribute));

    return SetNodeValueString(
	HStringReference(imageSrcUri).Get(),
	srcAttribute.Get(),
	toastXml);
}

/**
 * Set the values of each of the text nodes
 */
HRESULT
DesktopToastsApp::SetTextValues(
    const PCWSTR *textValues,
    UINT32 textValuesCount,
    IXmlDocument *toastXml)
{
    ComPtr<IXmlNodeList> nodeList;

    RETURN_IF_FAILED(
	toastXml->GetElementsByTagName(HStringReference(L"text").Get(),
				       &nodeList));

    UINT32 nodeListLength;
    RETURN_IF_FAILED(nodeList->get_Length(&nodeListLength));

    /*
     * If a template was chosen with fewer text elements, also change
     * the amount of strings passed to this method.
     */

    RETURN_IF_FAILED(textValuesCount <= nodeListLength ? S_OK : E_INVALIDARG);

    for (UINT32 i = 0; i < textValuesCount; i++) {
	ComPtr<IXmlNode> textNode;

	RETURN_IF_FAILED(nodeList->Item(i, &textNode));
	RETURN_IF_FAILED(SetNodeValueString(
			     HStringReference(textValues[i]).Get(),
			     textNode.Get(),
			     toastXml));
    }

    return S_OK;
}

HRESULT
DesktopToastsApp::SetNodeValueString(
    HSTRING inputString,
    IXmlNode *node,
    IXmlDocument *xml)
{
    ComPtr<IXmlText> inputText;
    RETURN_IF_FAILED(xml->CreateTextNode(inputString, &inputText));

    ComPtr<IXmlNode> inputTextNode;
    RETURN_IF_FAILED(inputText.As(&inputTextNode));

    ComPtr<IXmlNode> appendedChild;
    return node->AppendChild(inputTextNode.Get(), &appendedChild);
}

/**
 * Create and display the toast
 */
HRESULT
DesktopToastsApp::ShowToast(IXmlDocument *xml)
{
    /*
     * Create the notifier
     * (Classic Win32 apps MUST use the compat method to create the notifier)
     */
    ComPtr<IToastNotifier> notifier;
    RETURN_IF_FAILED(
	DesktopNotificationManagerCompat::CreateToastNotifier(&notifier));

    /*
     * And create the notification itself...
     */
    ComPtr<IToastNotification> toast;
    RETURN_IF_FAILED(
	DesktopNotificationManagerCompat::CreateToastNotification(xml, &toast));

    /*
     * And show it!
     */
    return notifier->Show(toast.Get());
}

/**
 * Clear all toasts
 */
HRESULT
DesktopToastsApp::ClearToasts()
{
    std::unique_ptr<DesktopNotificationHistoryCompat> history;

    /*
     * Get the history object
     * (Classic Win32 apps MUST use the compat method to obtain history)
     */
    RETURN_IF_FAILED(DesktopNotificationManagerCompat::get_History(&history));

    /*
     * And clear the toasts...
     */
    return history->Clear();
}

/**
 * Standard window procedure
 */
LRESULT CALLBACK
DesktopToastsApp::WndProc(
    HWND hwnd,
    UINT32 message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (message == WM_CREATE) {
	auto pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
	auto app = reinterpret_cast<DesktopToastsApp *>(pcs->lpCreateParams);

	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));

	return 1;
    }

    auto app = reinterpret_cast<DesktopToastsApp *>
	(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (app) {
	switch (message) {
	case WM_COMMAND: {
	    int wmId = LOWORD(wParam);

	    switch (wmId) {
	    case DesktopToastsApp::HM_POPTOASTBUTTON:
		app->DisplayToast();
		break;
	    case DesktopToastsApp::HM_CLEARTOASTSBUTTON:
		app->ClearToasts();
		break;
	    default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	    }

	    break;
	}
	case WM_PAINT: {
	    PAINTSTRUCT ps;

	    BeginPaint(hwnd, &ps);
	    EndPaint(hwnd, &ps);

	    return 0;
	}
	case WM_DESTROY:
	    PostQuitMessage(0);
	    return 1;
	}
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

HRESULT
DesktopToastsApp::SendBasicToast(PCWSTR message)
{
    ComPtr<IXmlDocument> doc;

    RETURN_IF_FAILED(
	DesktopNotificationManagerCompat::CreateXmlDocumentFromString(
	    L"<toast>"
	    L"<visual>"
	    L"<binding template=\"ToastGeneric\">"
	    L"<text></text>"
	    L"</binding>"
	    L"</visual>"
	    L"</toast>",
	    &doc));

    PCWSTR textValues[] = {
        message
    };

    RETURN_IF_FAILED(SetTextValues(textValues,
				   ARRAYSIZE(textValues),
				   doc.Get()));

    return ShowToast(doc.Get());
}
