#include <windows.h>
#include <iostream>
#include <vector>

// TODO: REPLACE THESE WITH YOUR BUTTON'S IDs
// You can find these in Device Manager -> Details -> Hardware Ids
const USHORT MY_DEVICE_VID = 0x1234; 
const USHORT MY_DEVICE_PID = 0x5678;

// Function to simulate the Escape key press
void SendEscapeKey() {
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Press the "Esc" key
    ip.ki.wVk = VK_ESCAPE;
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "Esc" key
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    std::cout << "[!] Panic Button Detected: ESCAPE sent." << std::endl;
}

// Window Procedure to handle Raw Input messages
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INPUT: 
    {
        UINT dwSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        
        if (dwSize == 0) return 0;

        std::vector<BYTE> lpb(dwSize);
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
            std::cout << "GetRawInputData does not return correct size !" << std::endl;

        RAWINPUT* raw = (RAWINPUT*)lpb.data();

        if (raw->header.dwType == RIM_TYPEKEYBOARD) 
        {
            // Get the device handle to check VID/PID
            RID_DEVICE_INFO deviceInfo;
            UINT deviceSize = sizeof(RID_DEVICE_INFO);
            deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);

            UINT dataSize = GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &deviceInfo, &deviceSize);

            if (dataSize > 0) {
                // Check if the input came from YOUR specific USB button
                if (deviceInfo.hid.dwVendorId == MY_DEVICE_VID && deviceInfo.hid.dwProductId == MY_DEVICE_PID) {
                    
                    // Check if it's a Key Down event to avoid double triggering
                    if (raw->data.keyboard.Message == WM_KEYDOWN || raw->data.keyboard.Message == WM_SYSKEYDOWN) {
                        SendEscapeKey();
                    }
                }
            }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

int main() {
    // 1. Create a hidden message-only window to receive input
    const char* CLASS_NAME = "PanicButtonListener";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "HiddenWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    if (hwnd == NULL) {
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }

    // 2. Register for Raw Input
    // We want to listen to generic keyboard devices (Usage Page 1, Usage 6)
    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = 0x01; // Generic Desktop Controls
    Rid[0].usUsage = 0x06;     // Keyboard
    Rid[0].dwFlags = RIDEV_INPUTSINK; // Receive input even when not in foreground
    Rid[0].hwndTarget = hwnd;

    if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {
        std::cerr << "Failed to register raw input." << std::endl;
        return 1;
    }

    std::cout << "Listening for Panic Button (VID: " << std::hex << MY_DEVICE_VID 
              << ", PID: " << MY_DEVICE_PID << ")..." << std::endl;
    std::cout << "Press Ctrl+C to exit." << std::endl;

    // 3. Message Loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}