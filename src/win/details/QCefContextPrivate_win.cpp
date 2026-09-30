#include "../../details/QCefContextPrivate.h"

#define NO_SHLWAPI_ISOS
#include <Shlwapi.h>
#undef NO_SHLWAPI_ISOS

#include <QDebug>
#include <QDir>

#include <CefViewCoreProtocol.h>

#include "../../details/QCefConfigPrivate.h"

namespace {
/// <summary>
/// The layout of RTL_OSVERSIONINFOW, declared locally to avoid including
/// winternl.h which conflicts with a number of other Windows headers.
/// </summary>
struct OsVersionInfo
{
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  WCHAR szCSDVersion[128];
};

/// <summary>
/// Checks whether the current Windows version is able to run the bundled CEF runtime.
/// </summary>
/// <returns>True if CEF may be usable; otherwise false</returns>
/// <remarks>
/// RtlGetVersion is used instead of GetVersionEx/VersionHelpers because those are
/// affected by the "supportedOS" entries of the application manifest and may
/// report a wrong version.
///
/// CEF is based on Chromium 110 or later, which dropped support for Windows 7,
/// Windows 8 and Windows 8.1. Checking this before anything else is essential:
/// when the loader is asked to map libcef.dll on those systems it reports the
/// missing entry point in a modal hard error dialog (for example "The procedure
/// entry point DiscardVirtualMemory could not be located in the dynamic link
/// library KERNEL32.dll"), and that dialog is shown even though the failed load
/// itself is handled by the caller.
///
/// If the version can not be determined, true is returned and the load probe
/// decides instead.
/// </remarks>
bool
isOperatingSystemSupported()
{
  using RtlGetVersionFn = LONG(WINAPI*)(OsVersionInfo*);

  auto ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
    return true;

  auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(::GetProcAddress(ntdll, "RtlGetVersion"));
  if (!rtlGetVersion)
    return true;

  OsVersionInfo osvi = {};
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  if (rtlGetVersion(&osvi) != 0) // STATUS_SUCCESS
    return true;

  // Windows 7 = 6.1, Windows 8 = 6.2, Windows 8.1 = 6.3, Windows 10/11 = 10.0
  return osvi.dwMajorVersion >= 10;
}

/// <summary>
/// Probes whether the CEF runtime can be loaded in the current process.
/// </summary>
/// <param name="libCefPath">The full path of libcef.dll</param>
/// <returns>True if the CEF runtime is usable; otherwise false</returns>
/// <remarks>
/// This covers the cases which the Windows version check does not predict, for
/// example a missing dependency or an unusable deployment of the CEF binaries.
/// Because QCefView delay-loads libcef.dll, a failed load would otherwise raise
/// a structured exception which terminates the process instead of returning an
/// error. Loading it explicitly here turns that into an ordinary failure.
///
/// The module is intentionally kept loaded, the delay-loaded imports will
/// resolve to this very module.
/// </remarks>
bool
probeCefRuntime(const std::wstring& libCefPath)
{
  // Suppress the loader's hard error message box. Without this a failed load
  // pops up a modal dialog naming the missing entry point before LoadLibraryExW
  // ever returns NULL.
  DWORD previousErrorMode = 0;
  ::SetThreadErrorMode(SEM_FAILCRITICALERRORS, &previousErrorMode);

  HMODULE handle = ::LoadLibraryExW(libCefPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
  DWORD lastError = ::GetLastError();

  if (!handle) {
    // Keep the suppressed error mode, a later load attempt must not be able to
    // bring up the loader's hard error dialog either.
    qWarning() << "Failed to load libcef.dll from" << QString::fromStdWString(libCefPath) << "error:" << lastError;
    return false;
  }

  ::SetThreadErrorMode(previousErrorMode, nullptr);

  return true;
}
} // namespace

bool
QCefContextPrivate::initializeCef(const QCefConfig* config)
{
  // Refuse to continue on systems which can not run the bundled CEF runtime.
  // This has to happen before libcef.dll is touched at all, see
  // isOperatingSystemSupported() for the reason.
  if (!isOperatingSystemSupported()) {
    qWarning() << "The current Windows version can not run the bundled CEF runtime";
    return false;
  }

  std::vector<wchar_t> modPath(MAX_PATH * 4);
  ::GetModuleFileNameW(nullptr, modPath.data(), static_cast<DWORD>(modPath.size()));
  ::PathRemoveFileSpecW(modPath.data());
  ::PathCombineW(modPath.data(), modPath.data(), L"CefView");
  ::SetDllDirectoryW(modPath.data());

  // Probe the CEF runtime before touching any CEF API. The version check above
  // only covers the systems we know about, this covers everything else. A
  // delay-load failure would terminate the process instead of reporting the
  // failure.
  {
    std::wstring libCefPath(modPath.data());
    libCefPath += L"\\libcef.dll";
    if (!probeCefRuntime(libCefPath)) {
      return false;
    }
  }

#if CEF_VERSION_MAJOR < 112
  // Enable High-DPI support on Windows 7 or newer.
  CefEnableHighDPISupport();
#endif

  // Build CefSettings
  CefSettings cef_settings;
  QCefConfigPrivate::CopyToCefSettings(config, &cef_settings);

#if CEF_VERSION_MAJOR >= 125 && CEF_VERSION_MAJOR <= 127
  //  https://github.com/chromiumembedded/cef/issues/3685
  cef_settings.chrome_runtime = true;
#endif

  // fixed values
#if CEF_VERSION_MAJOR < 128
  cef_settings.pack_loading_disabled = false;
#endif

  // external message pump
  if (cef_settings.multi_threaded_message_loop) {
    cef_settings.external_message_pump = false;
  } else {
    cef_settings.external_message_pump = true;
  }

  // path values
  if (CefString(&cef_settings.browser_subprocess_path).empty()) {
    QString strExePath = QDir(QString::fromWCharArray(modPath.data())).filePath(kCefViewRenderProcessName);
    CefString(&cef_settings.browser_subprocess_path) = QDir::toNativeSeparators(strExePath).toStdString();
  }

  // create job object
  DWORD dwProcessId = ::GetProcessId(::GetCurrentProcess());
  windowsJobName_ = QString("CefView-Job-{f0a3c1e3-ff89-4581-8a45-f0bfd74c4bb0}-%1").arg(dwProcessId);
  windowsJobHandle_ = ::CreateJobObjectA(nullptr, windowsJobName_.toStdString().c_str());
  if (nullptr == windowsJobHandle_) {
    qWarning() << "Failed to create windows job object:" << ::GetLastError();
  } else {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    ::memset(&info, 0, sizeof(info));
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    if (!::SetInformationJobObject(windowsJobHandle_, JobObjectExtendedLimitInformation, &info, sizeof(info))) {
      qWarning() << "Failed to set information for windows job object:" << GetLastError();
    }
  }

  // Initialize CEF
  auto cmdArgs = QCefConfigPrivate::GetCommandLineArgs(config);
  cmdArgs[kCefViewWindowsJobNameKey] = windowsJobName_.toStdString();
  auto appDelegate = std::make_shared<CCefAppDelegate>(this, cmdArgs);
  auto builtinSchemeName = config ? config->builtinSchemeName().toStdString() : std::string();
  auto bridgeObjectName = config ? config->bridgeObjectName().toStdString() : std::string();
  auto app = new CefViewBrowserApp(builtinSchemeName, bridgeObjectName, appDelegate);

  void* sandboxInfo = nullptr;
#if defined(CEF_USE_SANDBOX)
  // Manage the life span of the sandbox information object. This is necessary
  // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
  static CefScopedSandboxInfo scopedSandbox;
  sandboxInfo = scopedSandbox.sandbox_info();
#endif

  CefMainArgs main_args(::GetModuleHandle(nullptr));
  if (!CefInitialize(main_args, cef_settings, app, sandboxInfo)) {
    qWarning() << "Failed to initialize the CEF runtime";
    return false;
  }

  pApp_ = app;
  pAppDelegate_ = appDelegate;

  return true;
}

void
QCefContextPrivate::uninitializeCef()
{
  if (!pApp_)
    return;

  pAppDelegate_ = nullptr;
  pApp_ = nullptr;

  // shutdown the cef
  CefShutdown();

  // clean job object
  if (windowsJobHandle_) {
    ::CloseHandle(windowsJobHandle_);
    windowsJobHandle_ = nullptr;
  }
}
