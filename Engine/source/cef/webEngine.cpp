//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "webEngine.h"
#include "webApp.h"
#include "T3D/gameBase/gameProcess.h"
#include "gui/core/guiCanvas.h"
#include "sim/actionMap.h"
#include "SDL_keyboard.h"

#include "include/cef_client.h"
#include "include/cef_render_handler.h"

IMPLEMENT_STATIC_CLASS(WebEngine, , "@brief \n\n");

MODULE_BEGIN(WebEngine)

MODULE_INIT_AFTER(ProcessList)

MODULE_SHUTDOWN_BEFORE(ProcessList)
MODULE_SHUTDOWN_AFTER(Sim)

MODULE_INIT
{
   gWebEngine = new WebEngine;
   gWebEngine->init();
}

MODULE_SHUTDOWN
{
   if (gWebEngine)
   {
      gWebEngine->shutdown();
      SAFE_DELETE(gWebEngine);
   }
}

MODULE_END;

#include "webEngineMappings.h"

//------------------------------------------------------------------------------
ImplementEnumType(CefLogModeType, "Cef log mode types")
   { WebEngine::ModeDefault, "Default" },
   { WebEngine::ModeVerbose, "Verbose" },
   { WebEngine::ModeInfo, "Info" },
   { WebEngine::ModeWarning, "Warning" },
   { WebEngine::ModeError, "Error" },
   { WebEngine::ModeNone, "None" },
EndImplementEnumType;

//------------------------------------------------------------------------------
WebEngine *gWebEngine = NULL;
StringTableEntry WebEngine::mRootCachePath = StringTable->EmptyString();
StringTableEntry WebEngine::mCachePath = StringTable->EmptyString();
StringTableEntry WebEngine::mUserDataPath = StringTable->EmptyString();
StringTableEntry WebEngine::mLocaleStr = StringTable->EmptyString();
StringTableEntry WebEngine::mLogFile = StringTable->EmptyString();
StringTableEntry WebEngine::mUserAgent = StringTable->EmptyString();
S32 WebEngine::mLogSeverity = WebEngine::LogModeType::ModeWarning;
StringTableEntry WebEngine::mResourcePath = StringTable->EmptyString();
StringTableEntry WebEngine::mLocalesPath = StringTable->EmptyString();
S32 WebEngine::mRemoteDebuggingPort = 0;

#if defined TORQUE_OS_WIN
   #ifdef TORQUE_DEBUG
      StringTableEntry WebEngine::mSubProcessPath = StringTable->insert("cef_process_DEBUG.exe");
   #else
      StringTableEntry WebEngine::mSubProcessPath = StringTable->insert("cef_process.exe");
   #endif
#elif defined TORQUE_OS_LINUX
   #ifdef TORQUE_DEBUG
      StringTableEntry WebEngine::mSubProcessPath = StringTable->insert("./cef_process_DEBUG");
   #else
      StringTableEntry WebEngine::mSubProcessPath = StringTable->insert("./cef_process");
   #endif
#elif defined TORQUE_OS_MAC
   //#ifdef TORQUE_DEBUG
   //   StringTableEntry WebEngine::mSubProcessPath = StringTable->insert("cef_process_DEBUG.?");
   //#else
   //   StringTableEntry WebEngine::mSubProcessPath = StringTable->insert("cef_process.?");
   //#endif
#endif

//------------------------------------------------------------------------------
WebEngine::WebEngine()
{
   mProcessList = NULL;
   mCefInitialized = false;
   mResourceMutex = Mutex::createMutex();
}

//------------------------------------------------------------------------------
WebEngine::~WebEngine()
{
   Mutex::destroyMutex(mResourceMutex);
}

//------------------------------------------------------------------------------
bool WebEngine::init()
{
   // Build our key mapping arrays
   _buildKeyMaps();

   Con::addVariable("$Cef::rootCachePath", TypeString, &mRootCachePath,
      "The root directory that all CefSettings.cache_path and "
      "CefRequestContextSettings.cache_path values must have in common. If this "
      "value is empty and CefSettings.cache_path is non-empty then it will "
      "default to the CefSettings.cache_path value. If this value is non-empty "
      "then it must be an absolute path. Failure to set this value correctly may "
      "result in the sandbox blocking read/write access to the cache_path "
      "directory.\n");

   Con::addVariable("$Cef::cachePath", TypeString, &mCachePath,
      "The location where data for the global browser cache will be stored on "
      "disk. If this value is non-empty then it must be an absolute path that is "
      "either equal to or a child directory of CefSettings.root_cache_path. If "
      "this value is empty then browsers will be created in \"incognito mode\" where "
      "in-memory caches are used for storage and no data is persisted to disk. "
      "HTML5 databases such as localStorage will only persist across sessions if a "
      "cache path is specified. Can be overridden for individual CefRequestContext "
      "instances via the CefRequestContextSettings.cache_path value.\n");

   Con::addVariable("$Cef::userDataPath", TypeString, &mUserDataPath,
      "The location where user data such as spell checking dictionary files will "
      "be stored on disk. If this value is empty then the default "
      "platform-specific user data directory will be used (\"~/.cef_user_data\" "
      "directory on Linux, \"~/Library/Application Support/CEF/User Data\" directory "
      "on Mac OS X, \"Local Settings/Application Data/CEF/User Data\" directory "
      "under the user profile directory on Windows). If this value is non-empty "
      "then it must be an absolute path.\n");

   Con::addVariable("$Cef::localeString", TypeString, &mLocaleStr,
      "The locale string that will be passed to Blink. If empty the default locale of "
      "\"en - US\" will be used. This value is ignored on Linux where locale is "
      "determined using environment variable parsing with the precedence order: "
      "LANGUAGE, LC_ALL, LC_MESSAGES and LANG.\n");

   Con::addVariable("$Cef::logPath", TypeString, &mLogFile,
      "The directory and file name to use for the debug log. If empty, the default name "
      "of \"debug.log\" will be used and the file will be written to the application directory.\n");

   Con::addVariable("$Cef::userAgent", TypeString, &mUserAgent,
      "Value that will be returned as the User-Agent HTTP header. If empty the default User-Agent string will be used.\n");

   Con::addVariable("$Cef::logSeverity", TYPEID< LogModeType >(), &mLogSeverity,
      "The log severity. Only messages of this severity level or higher will be logged. "
      "Options are: \"Default\" (Same as Info), \"Verbose\", \"Info\", \"Warning\", \"Error\" and \"None\".\n");

   Con::addVariable("$Cef::resourcePath", TypeString, &mResourcePath,
      "The fully qualified path for the resources directory. If this value is empty the "
      "cef.pak and/or devtools_resources.pak files must be located in the module directory "
      "on Windows/Linux or the app bundle Resources directory on Mac OS X.\n");

   Con::addVariable("$Cef::localesPath", TypeString, &mLocalesPath,
      "The fully qualified path for the locales directory. If this value is empty the "
      "locales directory must be located in the module directory. This value is ignored "
      "on Mac OS X where pack files are always loaded from the app bundle Resources directory.\n");

   Con::addVariable("$Cef::remoteDebuggingPort", TypeS32, &mRemoteDebuggingPort,
      "Set to a value between 1024 and 65535 to enable remote debugging on the specified "
      "port. For example, if 8080 is specified the remote debugging URL will be "
      "http://localhost:8080. CEF can be remotely debugged from any CEF or Chrome browser window.\n");

   return true;
}

//------------------------------------------------------------------------------
void WebEngine::removeCefLogFile()
{
   char scriptFilenameBuffer[1024];
   String cleanfilename(Torque::Path::CleanSeparators(mLogFile));
   Con::expandScriptFilename(scriptFilenameBuffer, sizeof(scriptFilenameBuffer), cleanfilename.c_str());

   Torque::Path givenPath(Torque::Path::CompressPath(scriptFilenameBuffer));
   if (Torque::FS::IsFile(givenPath))
      Torque::FS::Remove(givenPath);
}

//------------------------------------------------------------------------------
bool WebEngine::initCef()
{
   CefMainArgs main_args;

   if (mCefInitialized)
      return true;

   // Clear the log from the last run
   removeCefLogFile();

   // Locate the sub-process executable
   char exePath[1024];
   dSprintf(exePath, 1024, "%s/%s", Platform::getMainDotCsDir(), mSubProcessPath);

   // Populate this structure to customize CEF behavior.
   CefSettings settings;
   CefString(&settings.browser_subprocess_path).FromASCII(exePath);
   settings.command_line_args_disabled = true;
   settings.no_sandbox = false;
   settings.multi_threaded_message_loop = false;
   settings.external_message_pump = true;
   settings.windowless_rendering_enabled = true;

   settings.log_severity = (cef_log_severity_t) mLogSeverity;
   CefString(&settings.log_file).FromASCII(mLogFile);

   CefString(&settings.root_cache_path).FromASCII(mRootCachePath);
   CefString(&settings.cache_path).FromASCII(mCachePath);
   CefString(&settings.user_data_path).FromASCII(mUserDataPath);
   CefString(&settings.locale).FromASCII(mLocaleStr);
   CefString(&settings.resources_dir_path).FromASCII(mResourcePath);
   CefString(&settings.locales_dir_path).FromASCII(mLocalesPath);
   CefString(&settings.user_agent).FromASCII(mUserAgent);
   settings.remote_debugging_port = mRemoteDebuggingPort;

   // Initialize CEF in the main process.
   CefEnableHighDPISupport();
   CefRefPtr<WebApp> app = new WebApp;
   mCefInitialized = CefInitialize(main_args, settings, app.get(), NULL);
   if (!mCefInitialized)
   {
      Con::errorf("WebEngine::initCef() - CefInitialize failed!");
      return false;
   }

   // Set to receive tick signals
   mProcessList = ClientProcessList::get();
   mProcessList->preTickSignal().notify(this, &WebEngine::process);

   return mCefInitialized;
}

//------------------------------------------------------------------------------
void WebEngine::shutdown()
{
   // Shut down CEF.
   if (mCefInitialized)
   {
      CefDoMessageLoopWork();
      CefShutdown();
   }

   // Remove from the process list
   if (mProcessList)
   {
      mProcessList->preTickSignal().remove(this, &WebEngine::process);
      mProcessList = NULL;
   }
}

//------------------------------------------------------------------------------
void WebEngine::process()
{
   // Process any cef messages
   CefDoMessageLoopWork();

   // If there are any pending file resource requests, open them.
   if (Mutex::lockMutex(mResourceMutex, false))
   {  // Don't block if we can't lock the mutex, we'll try again next tick.
      while (mResourceHandlers.size() > 0)
      {
         CefRefPtr<FileRequestHandler> handler = mResourceHandlers.first();
         handler->tryOpenFile();
         mResourceHandlers.pop_front();
      }

      Mutex::unlockMutex(mResourceMutex);
   }
}

//------------------------------------------------------------------------------
U16 WebEngine::asciiFromTorqueCode(S32 keyCode, bool shiftDown)
{
   if (keyCode >= KEY_ANYKEY)
      return 0;

   switch (keyCode)
   {
   case KEY_RETURN:
      return (U16) '\r';
   case KEY_TAB:
      return (U16) '\t';
   case KEY_SPACE:
      return (U16) ' ';
   case KEY_BACKSPACE:
      return (U16) '\b';
   default:
      break;
   }

   U32 SDLKey = SDL_GetKeyFromScancode( (SDL_Scancode)keyCode );
   const char *text = SDL_GetKeyName( SDLKey );
   if(!text || text[1] != 0)
      return 0;
   U8 ret = text[0];

   if( dIsalpha(ret) )
      return shiftDown ? dToupper( ret ) : dTolower( ret );

   return ret;
}

//------------------------------------------------------------------------------
bool WebEngine::mapDeviceEvent(const char* deviceInst, const char* deviceAction, const char* keyboardAction)
{
   // Determine the device
   U32 deviceType;
   U32 deviceNum;

   if (!ActionMap::getDeviceTypeAndInstance(deviceInst, deviceType, deviceNum))
   {
      Con::printf("WebEngine::mapDeviceEvent: unknown device: %s", deviceInst);
      return false;
   }

   EventDescriptor devDescriptor;
   if (!ActionMap::createEventDescriptor(deviceAction, &devDescriptor))
   {
      Con::printf("WebEngine::mapDeviceEvent: Could not create a description for device binding: %s", deviceAction);
      return false;
   }

   EventDescriptor keyDescriptor;
   S32 cefEventId = 0;
   unsigned int flags = EVENTFLAG_NONE;
   U16 asciiChar = 0;
   if (dStricmp(keyboardAction, "goback") == 0)
      cefEventId = browserBack;
   else if (dStricmp(keyboardAction, "goforward") == 0)
      cefEventId = browserForward;
   else if (dStricmp(keyboardAction, "reload") == 0)
      cefEventId = browserReload;
   else if (dStricmp(keyboardAction, "stop") == 0)
      cefEventId = browserStop;
   else
   {
      if (!ActionMap::createEventDescriptor(keyboardAction, &keyDescriptor))
      {
         Con::printf("WebEngine::mapDeviceEvent: Could not create a description for keyboard binding: %s", keyboardAction);
         return false;
      }
      if (keyDescriptor.eventType != SI_KEY)
      {
         Con::printf("WebEngine::mapDeviceEvent: Could not locate key for binding: %s", keyboardAction);
         return false;
      }
      cefEventId = getVKCodeFromTorque(keyDescriptor.eventCode);

      if (keyDescriptor.flags & SI_CTRL)
         flags |= EVENTFLAG_CONTROL_DOWN;
      if (keyDescriptor.flags & SI_SHIFT)
         flags |= EVENTFLAG_SHIFT_DOWN;
      if (keyDescriptor.flags & SI_ALT)
         flags |= EVENTFLAG_ALT_DOWN;
      if (keyDescriptor.flags & SI_MAC_OPT)
         flags |= EVENTFLAG_COMMAND_DOWN;

      asciiChar = asciiFromTorqueCode(keyDescriptor.eventCode, (keyDescriptor.flags & SI_SHIFT));
   }

   addMappedEvent(deviceType, deviceNum, devDescriptor.eventCode, cefEventId, flags, asciiChar);
   return true;
}

//------------------------------------------------------------------------------
void WebEngine::addMappedEvent(U32 deviceType, U32 deviceNum, U32 deviceCode, S32 keyCode, U32 flags, U16 asciiChar)
{
   U32 deviceKey = (deviceType << 8) + deviceNum;
   KeyDeviceMap::Iterator devItr = mMappedDevices.find(deviceKey);
   if (devItr == mMappedDevices.end())
   {
      devItr = mMappedDevices.insert(deviceKey, KeyEventMap());
      if (devItr == mMappedDevices.end())
      {
         Con::errorf("WebEngine::addMappedEvent - Failed adding device map.");
         return;
      }
   }

   KeyEventMap::Iterator  codeItr = devItr->value.find(deviceCode);
   if (codeItr == devItr->value.end())
   {
      codeItr = devItr->value.insert(deviceCode, CefKeyEvent(keyCode, flags, asciiChar));
      if (codeItr == devItr->value.end())
         Con::errorf("WebEngine::addMappedEvent - Failed adding device code map.");
   }
   else
   {
      codeItr->value.code = keyCode;
      codeItr->value.flags = flags;
      codeItr->value.ascii = asciiChar;
   }
}

bool WebEngine::getMappedCefEvent(const U32 deviceType, const U32 deviceNum, const U32 deviceCode, S32& keyCode, U32& flags, U16& asciiChar)
{
   U32 deviceKey = (deviceType << 8) + deviceNum;
   KeyDeviceMap::Iterator devItr = mMappedDevices.find(deviceKey);
   if (devItr == mMappedDevices.end())
      return false;

   KeyEventMap::Iterator  codeItr = devItr->value.find(deviceCode);
   if (codeItr == devItr->value.end())
      return false;

   keyCode = codeItr->value.code;
   flags = codeItr->value.flags;
   asciiChar = codeItr->value.ascii;
   return true;
}

//------------------------------------------------------------------------------
DefineEngineStaticMethod(WebEngine, initializeCEF, void, (), ,
   "@brief Initializes the cef process.\n\n"
   "Call this function to initialize cef after all of your \"$Cef::...\" paths and "
   "defaults have been configured.\n" )
{
   if (gWebEngine)
      gWebEngine->initCef();
   return;
}

//------------------------------------------------------------------------------
DefineEngineStaticMethod(WebEngine, mapDeviceEvent, bool, (const char* deviceInst, const char* deviceAction, const char* keyboardAction), ,
   "@brief Maps a joystick/controller/Device input to a keyboard or browser event.\n\n"
   "By mapping device events to keyboard navigation keys, you can navigate "
   "keyboard accessible webpages with controllers and other devices. The device "
   "and action strings use the same format that is used by ActionMap.\n"
   "@param deviceInst The device for this action 'gamepad', 'joystick'...\n"
   "@param deviceAction Any description from gVirtualMap in platform/input/event.cpp "
   "'btn_a', 'dpov', 'button0'...\n"
   "@param keyboardAction Any description, with optional modifier, from gKeyboardMap "
   "in platform/input/event.cpp 'tab', 'shift tab', 'up'...etc. In addition to the keyboard events, the "
   "4 browser events are mappable with the strings 'goback', 'goforward', 'reload' and 'stop'.\n\n"

   "@tsexample\n"
   "WebEngine::mapDeviceEvent(\"gamepad0\", \"btn_r\", \"tab\");\n"
   "WebEngine::mapDeviceEvent(\"gamepad0\", \"btn_l\", \"shift tab\");\n"
   "WebEngine::mapDeviceEvent(\"gamepad0\", \"btn_a\", \"space\");\n"
   "WebEngine::mapDeviceEvent(\"gamepad0\", \"btn_back\", \"goback\");\n"
   "WebEngine::mapDeviceEvent(\"gamepad0\", \"btn_guide\", \"f1\");\n"
   "@endtsexample\n\n")
{
   if (gWebEngine)
      return gWebEngine->mapDeviceEvent(deviceInst, deviceAction, keyboardAction);
   return false;
}

