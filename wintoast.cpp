// wintoast.cpp - Windows 10 notification executable 
// Compile with: 
// cl.exe /EHsc /std:c++17 win10_notifier.cpp /link /SUBSYSTEM:WINDOWS user32.lib ole32.lib shell32.lib runtimeobject.lib

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h> // Added missing include
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.ui.notifications.h>
#include <string>

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "shlwapi.lib") // Add this line to link shlwapi.lib

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Windows::Foundation;

// Convert UTF-8 string to wide string
std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], size_needed);
    wstr.resize(size_needed - 1);  // Remove null terminator
    return wstr;
}

// Show Windows 10 notification
bool ShowNotification(const std::wstring& appId, const std::wstring& title, const std::wstring& message) {
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return false;
    
    // Set the app ID
    hr = SetCurrentProcessExplicitAppUserModelID(appId.c_str());
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }
    
    // Get the toast notification manager
    ComPtr<IToastNotificationManagerStatics> toastStatics;
    hr = RoGetActivationFactory(
        HStringReference(L"Windows.UI.Notifications.ToastNotificationManager").Get(),
        __uuidof(IToastNotificationManagerStatics),
        &toastStatics);
    
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }
    
    // Get the toast XML template
    ComPtr<IXmlDocument> toastXml;
    hr = toastStatics->GetTemplateContent(ToastTemplateType_ToastText02, &toastXml);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }
    
    // Get the text elements
    ComPtr<IXmlNodeList> nodeList;
    hr = toastXml->GetElementsByTagName(HStringReference(L"text").Get(), &nodeList);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }
    
    // Set the title text
    ComPtr<IXmlNode> titleNode;
    hr = nodeList->Item(0, &titleNode);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlText> textElement;
        hr = toastXml->CreateTextNode(HStringReference(title.c_str()).Get(), &textElement);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> textNode;
            hr = textElement.As(&textNode);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> appendedNode;
                hr = titleNode->AppendChild(textNode.Get(), &appendedNode);
            }
        }
    }
    
    // Set the message text
    ComPtr<IXmlNode> msgNode;
    hr = nodeList->Item(1, &msgNode);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlText> textElement;
        hr = toastXml->CreateTextNode(HStringReference(message.c_str()).Get(), &textElement);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> textNode;
            hr = textElement.As(&textNode);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> appendedNode;
                hr = msgNode->AppendChild(textNode.Get(), &appendedNode);
            }
        }
    }
    
    // Create and show the toast
    ComPtr<IToastNotifier> notifier;
    hr = toastStatics->CreateToastNotifierWithId(HStringReference(appId.c_str()).Get(), &notifier);
    if (SUCCEEDED(hr)) {
        ComPtr<IToastNotificationFactory> factory;
        hr = RoGetActivationFactory(
            HStringReference(L"Windows.UI.Notifications.ToastNotification").Get(),
            __uuidof(IToastNotificationFactory),
            &factory);
            
        if (SUCCEEDED(hr)) {
            ComPtr<IToastNotification> toast;
            hr = factory->CreateToastNotification(toastXml.Get(), &toast);
            if (SUCCEEDED(hr)) {
                hr = notifier->Show(toast.Get());
            }
        }
    }
    
    // Clean up
    CoUninitialize();
    return SUCCEEDED(hr);
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Parse command line arguments
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    // Require exactly 4 arguments (program name + appId + title + message)
    if (argc != 4) {
        MessageBoxW(NULL, L"Usage: wintoast.exe <appId> <title> <message>", L"Error", MB_OK | MB_ICONERROR);
        LocalFree(argv);
        return 1;
    }
    
    // Convert wide strings to UTF-8
    char appIdUtf8[1024] = {0};
    char titleUtf8[1024] = {0};
    char messageUtf8[1024] = {0};
    
    WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, appIdUtf8, sizeof(appIdUtf8), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, titleUtf8, sizeof(titleUtf8), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, argv[3], -1, messageUtf8, sizeof(messageUtf8), NULL, NULL);
    
    // Convert back to wide strings (ensures proper UTF-8 handling)
    std::wstring wAppId = utf8_to_wide(appIdUtf8);
    std::wstring wTitle = utf8_to_wide(titleUtf8);
    std::wstring wMessage = utf8_to_wide(messageUtf8);
    
    // Show notification
    bool success = ShowNotification(wAppId, wTitle, wMessage);
    
    // Clean up
    LocalFree(argv);
    
    return success ? 0 : 1;
}

