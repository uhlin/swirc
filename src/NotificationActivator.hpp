#ifndef NOTIFICATION_ACTIVATOR_HPP
#define NOTIFICATION_ACTIVATOR_HPP

#include "ToastActivator.h"

using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;

/**
 * Implement a handler for toast activation, so that when the user
 * clicks on your toast, your app can do something. (The UUID CLSID
 * must be unique to your app.)
 */
class DECLSPEC_UUID("62337340-CB78-4AE9-A524-685424C52DC7")
    NotificationActivator WrlSealed WrlFinal : public RuntimeClass
    <RuntimeClassFlags<ClassicCom>, INotificationActivationCallback>
{
  public:
	virtual HRESULT STDMETHODCALLTYPE
	Activate(LPCWSTR appUserModelId, LPCWSTR invokedArgs,
	    const NOTIFICATION_USER_INPUT_DATA *data, ULONG dataCount) override
	{
		/*
		 * TODO: Handle activation
		 */
		return S_OK;
	}
};

/*
 * Flag class as COM creatable
 */
CoCreatableClass(NotificationActivator);

#endif
