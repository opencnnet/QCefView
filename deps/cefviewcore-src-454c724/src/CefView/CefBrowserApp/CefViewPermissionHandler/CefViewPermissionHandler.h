//
//  CefViewPermissionHandler.h
//  CefViewCore
//

#ifndef CefViewPermissionHandler_h
#define CefViewPermissionHandler_h
#pragma once

#include <CefViewCoreGlobal.h>

// The "local network access" permission is only known to CEF 136 and newer
// (CEF_PERMISSION_TYPE_LOCAL_NETWORK_ACCESS, guarded by CEF_API_ADDED(13600)).
#if CEF_VERSION_MAJOR >= 136

/// <summary>
/// Answers the permission prompts that CEF asks the client for.
///
/// Since Chromium 138 the local network access checks are enabled by default
/// (base::Feature kLocalNetworkAccessChecks with base::FEATURE_ENABLED_BY_DEFAULT),
/// so every request from the web content to a local/private endpoint
/// (127.0.0.1, 192.168.x.x, 10.x.x.x, *.local, ...) needs the new
/// "local network access" permission.
///
/// CEF asks the client with CefPermissionHandler::OnShowPermissionPrompt.
/// If the client does not answer the prompt, the default handling applies,
/// which is CEF_PERMISSION_RESULT_IGNORE with the Alloy runtime style, so the
/// request fails (the permission behaves as "denied by default").
/// This handler grants the permission automatically and therefore preserves
/// the behavior of CEF/Chromium before 138.
/// </summary>
class CefViewPermissionHandler : public CefPermissionHandler
{
  IMPLEMENT_REFCOUNTING(CefViewPermissionHandler);

public:
  /// <summary>
  ///
  /// </summary>
  CefViewPermissionHandler() = default;

  /// <summary>
  /// Called when a page should show a permission prompt.
  /// Return true and call the callback to answer the prompt here,
  /// return false to let CEF continue with its default handling.
  /// </summary>
  /// <param name="browser">The browser that requests the permission.</param>
  /// <param name="prompt_id">The id of the prompt.</param>
  /// <param name="requesting_origin">The origin that requests the permission.</param>
  /// <param name="requested_permissions">A combination of values from
  /// cef_permission_request_types_t.</param>
  /// <param name="callback">The callback used to complete the request.</param>
  /// <returns></returns>
  bool OnShowPermissionPrompt(CefRefPtr<CefBrowser> browser,
                              uint64_t prompt_id,
                              const CefString& requesting_origin,
                              uint32_t requested_permissions,
                              CefRefPtr<CefPermissionPromptCallback> callback) override;
};

#endif // CEF_VERSION_MAJOR >= 136

#endif /* CefViewPermissionHandler_h */
