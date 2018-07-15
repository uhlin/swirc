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

#pragma once
#include <string>
#include <memory>
#include <Windows.h>
#include <windows.ui.notifications.h>
#include <wrl.h>
#define TOAST_ACTIVATED_LAUNCH_ARG L"-ToastActivated"

using namespace ABI::Windows::UI::Notifications;

class DesktopNotificationHistoryCompat;

namespace DesktopNotificationManagerCompat {
    /**
     * If not running under the Desktop Bridge, you must call this
     * method to register your AUMID with the Compat library and to
     * register your COM CLSID and EXE in LocalServer32 registry. Feel
     * free to call this regardless, and we will no-op if running
     * under Desktop Bridge. Call this upon application startup,
     * before calling any other APIs.
     *
     * @param aumid An AUMID that uniquely identifies your application.
     * @param clsid The CLSID of your NotificationActivator class.
     */
    HRESULT RegisterAumidAndComServer(const wchar_t *aumid, GUID clsid);

    /**
     * Registers your module to handle COM activations. Call this upon
     * application startup.
     */
    HRESULT RegisterActivator();

    /**
     * Creates a toast notifier. You must have called
     * RegisterActivator first (and also RegisterAumidAndComServer if
     * you're a classic Win32 app), or this will throw an exception.
     */
    HRESULT CreateToastNotifier(IToastNotifier** notifier);

    /**
     * Creates an XmlDocument initialized with the specified
     * string. This is simply a convenience helper method.
     */
    HRESULT CreateXmlDocumentFromString(
	const wchar_t *xmlString,
	ABI::Windows::Data::Xml::Dom::IXmlDocument** doc);

    /**
     * Creates a toast notification. This is simply a convenience
     * helper method.
     */
    HRESULT CreateToastNotification(
	ABI::Windows::Data::Xml::Dom::IXmlDocument* content,
	IToastNotification** notification);

    /**
     * Gets the DesktopNotificationHistoryCompat object.
     *
     * You must have called RegisterActivator first (and also
     * RegisterAumidAndComServer if you're a classic Win32 app), or
     * this will throw an exception.
     */
    HRESULT get_History(
	std::unique_ptr<DesktopNotificationHistoryCompat>* history);

    /**
     * Gets a boolean representing whether http images can be used
     * within toasts. This is true if running under Desktop Bridge.
     */
    bool CanUseHttpImages();
}

class DesktopNotificationHistoryCompat {
public:

    /**
     * Removes all notifications sent by this app from action center.
     */
    HRESULT Clear();

    /**
     * Gets all notifications sent by this app that are currently
     * still in Action Center.
     */
    HRESULT GetHistory(
	ABI::Windows::Foundation::Collections::
	IVectorView<ToastNotification*>** history);

    /**
     * Removes an individual toast, with the specified tag label, from
     * action center.
     *
     * @param tag The tag label of the toast notification to be removed.
     */
    HRESULT Remove(const wchar_t *tag);

    /**
     * Removes a toast notification from the action using the
     * notification's tag and group labels.
     *
     * @param tag The tag label of the toast notification to be removed.
     * @param group The group label of the toast notification to be removed.
     */
    HRESULT RemoveGroupedTag(const wchar_t *tag, const wchar_t *group);

    /**
     * Removes a group of toast notifications, identified by the
     * specified group label, from action center.
     *
     * @param group The group label of the toast notifications to be removed.
     */
    HRESULT RemoveGroup(const wchar_t *group);

    /**
     * Do not call this.  Instead, call
     * DesktopNotificationManagerCompat.get_History() to obtain an
     * instance.
     */
    DesktopNotificationHistoryCompat(
	const wchar_t *aumid,
	Microsoft::WRL::ComPtr<IToastNotificationHistory> history);

private:
    std::wstring m_aumid;
    Microsoft::WRL::ComPtr<IToastNotificationHistory> m_history = nullptr;
};
