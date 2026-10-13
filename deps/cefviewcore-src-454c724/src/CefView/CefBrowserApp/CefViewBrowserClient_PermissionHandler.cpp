#include <CefViewBrowserClient.h>

#include <Common/CefViewCoreLog.h>

#if CEF_VERSION_MAJOR >= 136

#include "CefViewPermissionHandler/CefViewPermissionHandler.h"

CefRefPtr<CefPermissionHandler>
CefViewBrowserClient::GetPermissionHandler()
{
  return permission_handler_;
}

#endif // CEF_VERSION_MAJOR >= 136
