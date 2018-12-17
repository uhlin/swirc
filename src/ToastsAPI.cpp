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
#include "ToastsAPI.hpp"
#include "errHand.h"

#define RETURN_IF_FAILED(hr)\
	do {\
		HRESULT _hrTemp = hr;\
\
		if (FAILED(_hrTemp)) {\
			return _hrTemp;\
		}\
	} while (false)

/**
 * Clear all toasts
 */
HRESULT
Toasts::ClearToasts(void)
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
 * Create the toast XML from a template
 */
#if 0
HRESULT
Toasts::CreateToastXml(IXmlDocument **toastXml)
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
#endif

/**
 * Display the toast using classic COM. Note that is also possible to
 * create and display the toast using the new C++ /ZW options.
 */
#if 0
HRESULT
Toasts::DisplayToast(void)
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
#endif

HRESULT
Toasts::SendBasicToast(PCWSTR message)
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

/**
 * Set the value of the "src" attribute of the "image" node
 */
HRESULT
Toasts::SetImageSrc(PCWSTR imagePath, IXmlDocument *toastXml)
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

HRESULT
Toasts::SetNodeValueString(
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
 * Set the values of each of the text nodes
 */
HRESULT
Toasts::SetTextValues(
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

/**
 * Create and display the toast
 */
HRESULT
Toasts::ShowToast(IXmlDocument *xml)
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

void
Toasts::SendTestNotification(void)
{
    ComPtr<IToastNotification> toast;
    ComPtr<IToastNotifier> notifier;
    HRESULT hr;
    IXmlDocument *doc;

    hr = DesktopNotificationManagerCompat::CreateXmlDocumentFromString(
	L"<toast>"
	L"<visual>"
	L"<binding template='ToastGeneric'>"
	L"<text>The universal IRC client</text>"
	L"</binding>"
	L"</visual>"
	L"</toast>",
	&doc);
    if (FAILED(hr)) {
	err_log(0, "In SendTestNotification: "
	    "CreateXmlDocumentFromString failed");
	return;
    }

    hr = DesktopNotificationManagerCompat::CreateToastNotifier(&notifier);
    if (FAILED(hr)) {
	err_log(0, "In SendTestNotification: CreateToastNotifier failed");
	return;
    }

    hr = DesktopNotificationManagerCompat::CreateToastNotification(doc, &toast);
    if (FAILED(hr)) {
	err_log(0, "In SendTestNotification: CreateToastNotification failed");
	return;
    }

    /*
     * And show it!
     */
    (void) notifier->Show(toast.Get());
}
