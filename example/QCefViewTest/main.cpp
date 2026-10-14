#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include <QCefContext.h>

#include "MainWindow.h"

#ifdef Q_OS_WINDOWS
#if defined(ENABLE_GPU_OPTIMUS) && ENABLE_GPU_OPTIMUS
#include <windows.h>
extern "C"
{
  // http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
  __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

  // https://gpuopen.com/learn/amdpowerxpressrequesthighperformance
  _declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif
#endif

int
main(int argc, char* argv[])
{
#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
  // For off-screen rendering, Qt::AA_EnableHighDpiScaling must be enabled. If not,
  // then all devicePixelRatio methods will always return 1.0,
  // so CEF will not scale the web content
  // NOET: There is bugs in Qt 6.2.4, the HighDpi doesn't work
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
  QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif

  // create QApplication instance
  QApplication a(argc, argv);

  // build QCefConfig
  QCefConfig config;
  // set user agent
  // config.setUserAgent("QCefViewTest");
  // set log level
  config.setLogLevel(QCefConfig::LOGSEVERITY_DEFAULT);
  // set JSBridge object name (default value is CefViewClient)
  config.setBridgeObjectName("CallBridge");
  // set Built-in scheme name (default value is CefView)
  config.setBuiltinSchemeName("CefView");
  // port for remote debugging (default is 0 and means to disable remote debugging)
  config.setRemoteDebuggingPort(9000);
  // set background color for all browsers
  // (QCefSetting.setBackgroundColor will overwrite this value for specified browser instance)
  // config.setBackgroundColor(Qt::lightGray);

  // windowlessRenderingEnabled is set to true by default,
  // set to false to disable the OSR mode
  config.setWindowlessRenderingEnabled(true);
  config.setStandaloneMessageLoopEnabled(true);

  // disable sandbox
  // this is a bit complicated, please refer to:
  // https://developer.apple.com/documentation/xcode/configuring-the-macos-app-sandbox
  config.setSandboxDisabled(true);

  // add command line args, you can any cef supported switches or parameters
  config.addCommandLineSwitch("use-mock-keychain");
  config.addCommandLineSwitch("enable-aggressive-domstorage-flushing");
  // allow remote debugging
  config.addCommandLineSwitchWithValue("remote-allow-origins", "*");
  // Chromium 138 enables the local network access (LNA) checks by default, so
  // web content can not reach local/private endpoints (127.0.0.1, 10.x.x.x,
  // 192.168.x.x, *.local ...) any more. In CEF such a request is blocked
  // without raising a permission prompt, hence a CefPermissionHandler never
  // gets the chance to grant it. Keep the checks but let them only produce
  // DevTools warnings.
  // Use addCommandLineSwitchWithValue("disable-features",
  // "LocalNetworkAccessChecks") instead to turn the checks off completely.
  // NOTE: enable-features/disable-features can only be set once (the value of
  // the same switch is overwritten), merge the features into a single value.
  config.addCommandLineSwitchWithValue(
    "enable-features", "LocalNetworkAccessChecks:LocalNetworkAccessChecksWarn/true");

  // set cache folder
  config.setCachePath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));

  // create QCefContext instance with config,
  // the lifecycle of cefContext must be the same as QApplication instance
  QCefContext cefContext(&a, argc, argv, &config);

  // application window
  MainWindow w;
  w.show();

  // flying
  return a.exec();
}
