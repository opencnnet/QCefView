//
//  CefViewPermissionHandler.cpp
//  CefViewCore
//

#include "CefViewPermissionHandler.h"

#if CEF_VERSION_MAJOR >= 136

#include <Common/CefViewCoreLog.h>

bool
CefViewPermissionHandler::OnShowPermissionPrompt(CefRefPtr<CefBrowser> browser,
                                                 uint64_t prompt_id,
                                                 const CefString& requesting_origin,
                                                 uint32_t requested_permissions,
                                                 CefRefPtr<CefPermissionPromptCallback> callback)
{
  CEF_REQUIRE_UI_THREAD();

  const uint32_t localNetworkAccess = static_cast<uint32_t>(CEF_PERMISSION_TYPE_LOCAL_NETWORK_ACCESS);

  // Grant the permission only when it is the single permission of the request.
  // A request that also asks for other permissions falls back to the default
  // handling. Use "requested_permissions & localNetworkAccess" instead if the
  // local network access permission should be granted in such requests as well.
  if (requested_permissions == localNetworkAccess) {
    logI("Grant the local network access permission to %s (browser id: %d)",
         requesting_origin.ToString().c_str(),
         browser ? browser->GetIdentifier() : -1);

    callback->Continue(CEF_PERMISSION_RESULT_ACCEPT);
    return true;
  }

  // let CEF handle every other permission request
  return false;
}

#endif // CEF_VERSION_MAJOR >= 136
