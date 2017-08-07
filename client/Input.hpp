#pragma once
#include <Windows.h>
#include <math.h>
#include <tchar.h>
extern "C"
{
#include <hidsdi.h>
}

#define     NUM_KEYS        256
#define     NUM_BUTTONS     10
#define     NUM_M_BUTTONS   2
#define     SAFE_FREE(p)    { if(p) { HeapFree(hHeap, 0, p); (p) = NULL; } }
#define     CHECK(exp)      { if(!(exp)) goto Error; }

namespace Input
{

class KeyBoard
{
public:
    bool    bKeys[NUM_KEYS];
    bool    bKeys_state[2][NUM_KEYS];
    int     input_state;

    KeyBoard();
    ~KeyBoard();

    void    update();

    bool    keyPressed(unsigned char key);
    bool    keyReleased(unsigned char key);
    bool    keyDown(unsigned char key);
    bool    keyUp(unsigned char key);
};

class Mouse
{
public:
    bool    bButtons[NUM_M_BUTTONS];
    bool    bButtons_state[2][NUM_M_BUTTONS];
    long    lX;
    long    lX_state[2];
    long    lY;
    long    lY_state[2];
    int     input_state;

    Mouse();
    ~Mouse();

    void    update();

    long    ldX();
    long    ldY();

    bool    buttonPressed();
    bool    buttonReleased();
    bool    buttonDown();
    bool    buttonUp();
};

typedef enum GP_HatDirection
{
    GPH_UP = 0,
    GPH_RIGHTUP = 1,
    GPH_RIGHT = 2,
    GPH_RIGHTDOWN = 3,
    GPH_DOWN = 4,
    GPH_LEFTDOWN = 5,
    GPH_LEFT = 6,
    GPH_LEFTUP = 7
} GP_HatDirection;

typedef enum GP_Button
{
    GPB_A = 0,
    GPB_B = 1,
    GPB_X = 2,
    GPB_Y = 3,
    GPB_LB = 4,
    GPB_RB = 5,
    GPB_SL = 6,
    GPB_ST = 7,
    GPB_LS = 8,
    GPB_RS = 9
} GP_Button;

typedef enum GP_AxisDirection
{
    GPA_LEFT = 1,
    GPA_RIGHT = 2
} GP_AxisDirection;

class GamePad
{
public:
    int     numButtons;

    bool    bButtons[NUM_BUTTONS];
    bool    bButtons_state[2][NUM_BUTTONS];
    long    lAxisX;
    long    lAxisX_state[2];
    long    lAxisY;
    long    lAxisY_state[2];
    long    lAxisZ;
    long    lAxisZ_state[2];
    long    lAxisRz;
    long    lAxisRz_state[2];
    bool    bHat_state[2][8];
    bool    bHat[8];

    int     input_state;

    GamePad();
    ~GamePad();

    void    update();

    bool    buttonPressed(int button);
    bool    buttonReleased(int button);
    bool    buttonDown(int button);
    bool    buttonUp(int button);

    bool    hatPressed(GP_HatDirection direction);
    bool    hatReleased(GP_HatDirection direction);
    bool    hatDown(GP_HatDirection direction);
    bool    hatUp(GP_HatDirection direction);

    bool    lAxisXPressed(GP_AxisDirection direction);
    bool    lAxisXReleased(GP_AxisDirection direction);
    bool    lAxisYPressed(GP_AxisDirection direction);
    bool    lAxisYReleased(GP_AxisDirection direction);

    float   fAxisX();
    float   fAxisY();
    float   fAxisZ();
    float   fAxisRz();
};

class InputGroup
{
public:
    KeyBoard    keyboard;
    Mouse       mouse;
    GamePad     gamepad;

    InputGroup();
    ~InputGroup();

    void        update();
};

void ReadGamePadInput(PRAWINPUT pRawInput, GamePad &pGamePad);

}

using namespace Input;

KeyBoard::KeyBoard()
{
    input_state = 0;
    ZeroMemory(bKeys, sizeof(bKeys));
}

KeyBoard::~KeyBoard()
{
}

void KeyBoard::update()
{
    input_state ^= 1;
    memcpy(bKeys_state[input_state], bKeys, NUM_KEYS);
}

bool KeyBoard::keyPressed(unsigned char key)
{
    return bKeys_state[input_state][key] &&
        !bKeys_state[input_state ^ 1][key];
}

bool KeyBoard::keyReleased(unsigned char key)
{
    return !bKeys_state[input_state][key] &&
        bKeys_state[input_state ^ 1][key];
}

bool KeyBoard::keyDown(unsigned char key)
{
    return bKeys_state[input_state][key];
}

bool KeyBoard::keyUp(unsigned char key)
{
    return !bKeys_state[input_state][key];
}

Mouse::Mouse()
{
    input_state = 0;
    lX = 0;
    lY = 0;
    ZeroMemory(bButtons, sizeof(bButtons));
}

Mouse::~Mouse()
{
}

void Mouse::update()
{
    input_state ^= 1;
    memcpy(bButtons_state[input_state], bButtons_state, NUM_M_BUTTONS);
    lX_state[input_state] = lX;
    lY_state[input_state] = lY;
}

long Mouse::ldX()
{
    return (lX_state[input_state] - lX_state[input_state ^ 1]);
}

long Mouse::ldY()
{
    return (lY_state[input_state] - lY_state[input_state ^ 1]);
}

GamePad::GamePad()
{
    ZeroMemory(bButtons, sizeof(bButtons));
    lAxisX = 0;
    lAxisY = 0;
    lAxisZ = 0;
    lAxisRz = 0;
    ZeroMemory(bHat, sizeof(bHat));
}

GamePad::~GamePad()
{
}

void GamePad::update()
{
    input_state ^= 1;
    memcpy(bButtons_state[input_state], bButtons, NUM_BUTTONS);
    memcpy(bHat_state[input_state], bHat, 8);
}

bool GamePad::buttonPressed(int button)
{
    return bButtons_state[input_state][button] &&
        !bButtons_state[input_state ^ 1][button];
}

bool GamePad::buttonReleased(int button)
{
    return !bButtons_state[input_state][button] &&
        bButtons_state[input_state ^ 1][button];
}

bool GamePad::buttonDown(int button)
{
    return bButtons_state[input_state][button];
}

bool GamePad::buttonUp(int button)
{
    return !bButtons_state[input_state][button];
}

bool GamePad::hatPressed(GP_HatDirection direction)
{
    return (bHat_state[input_state][direction]) &&
        !(bHat_state[input_state ^ 1][direction]);
}

bool GamePad::hatReleased(GP_HatDirection direction)
{
    return !(bHat_state[input_state][direction]) &&
        (bHat_state[input_state ^ 1][direction]);
}

bool GamePad::hatDown(GP_HatDirection direction)
{
    return bHat_state[input_state][direction];
}

bool GamePad::hatUp(GP_HatDirection direction)
{
    return !bHat_state[input_state][direction];
}

bool GamePad::lAxisXPressed(GP_AxisDirection direction)
{
    return false;
}

bool GamePad::lAxisXReleased(GP_AxisDirection direction)
{
    return false;
}

bool GamePad::lAxisYPressed(GP_AxisDirection direction)
{
    return false;
}

bool GamePad::lAxisYReleased(GP_AxisDirection direction)
{
    return false;
}

float GamePad::fAxisX()
{
    return ((float)lAxisX / 32767.0f);
}

float GamePad::fAxisY()
{
    return ((float)lAxisY / 32767.0f);
}

float GamePad::fAxisZ()
{
    return ((float)lAxisZ / 32767.0f);
}

float GamePad::fAxisRz()
{
    return ((float)lAxisRz / 32767.0f);
}

InputGroup::InputGroup()
{
}

InputGroup::~InputGroup()
{
}

void InputGroup::update()
{
    keyboard.update();
    mouse.update();
    gamepad.update();
}

void
Input::ReadGamePadInput(PRAWINPUT pRawInput, GamePad &pGamePad)
{
    PHIDP_PREPARSED_DATA pPreparsedData;
    HIDP_CAPS            Caps;
    PHIDP_BUTTON_CAPS    pButtonCaps;
    PHIDP_VALUE_CAPS     pValueCaps;
    USHORT               capsLength;
    UINT                 bufferSize;
    HANDLE               hHeap;
    USAGE                usage[NUM_BUTTONS];
    ULONG                i, usageLength, value;

    pPreparsedData = NULL;
    pButtonCaps = NULL;
    pValueCaps = NULL;
    hHeap = GetProcessHeap();

    CHECK(GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0);
    CHECK(pPreparsedData = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, bufferSize));
    CHECK((int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize) >= 0);
    CHECK(HidP_GetCaps(pPreparsedData, &Caps) == HIDP_STATUS_SUCCESS);
    CHECK(pButtonCaps = (PHIDP_BUTTON_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps));
    capsLength = Caps.NumberInputButtonCaps;
    CHECK(HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS);
    pGamePad.numButtons = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;
    CHECK(pValueCaps = (PHIDP_VALUE_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps));
    capsLength = Caps.NumberInputValueCaps;
    CHECK(HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS);

    usageLength = pGamePad.numButtons;
    CHECK(
        HidP_GetUsages(
            HidP_Input, pButtonCaps->UsagePage, 0, usage, &usageLength, pPreparsedData,
            (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
        ) == HIDP_STATUS_SUCCESS);

    ZeroMemory(pGamePad.bButtons, sizeof(pGamePad.bButtons));
    for (i = 0; i < usageLength; i++)
        pGamePad.bButtons[usage[i] - pButtonCaps->Range.UsageMin] = TRUE;

    for (i = 0; i < Caps.NumberInputValueCaps; i++)
    {
        CHECK(
            HidP_GetUsageValue(
                HidP_Input, pValueCaps[i].UsagePage, 0, pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
                (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
            ) == HIDP_STATUS_SUCCESS);

        switch (pValueCaps[i].Range.UsageMin)
        {
        case 0x30:
            pGamePad.lAxisX = (long)((value - 32768));
            break;
        case 0x31:
            pGamePad.lAxisY = -1 * ((long)((value - 32767)));
            break;
        case 0x32:
            pGamePad.lAxisZ = (long)((value - 32768));
            break;
        case 0x35:
            pGamePad.lAxisRz = (long)((value - 32768));
            break;
        case 0x39:
            ZeroMemory(pGamePad.bHat, sizeof(pGamePad.bHat));
            pGamePad.bHat[value - 1] = 1;
            int i = value - 1;
            if (i == GPH_LEFTDOWN)
            {
                pGamePad.bHat[GPH_LEFT] = 1;
                pGamePad.bHat[GPH_DOWN] = 1;
            }
            else if (i == GPH_RIGHTDOWN)
            {
                pGamePad.bHat[GPH_RIGHT] = 1;
                pGamePad.bHat[GPH_DOWN] = 1;
            }
            else if (i == GPH_LEFTUP)
            {
                pGamePad.bHat[GPH_LEFT] = 1;
                pGamePad.bHat[GPH_UP] = 1;
            }
            else if (i == GPH_RIGHTUP)
            {
                pGamePad.bHat[GPH_RIGHT] = 1;
                pGamePad.bHat[GPH_UP] = 1;
            }
            break;
        }
    }

    long joyDeadZone = 8192;
    if (pGamePad.lAxisX < joyDeadZone && pGamePad.lAxisX > -joyDeadZone)
        pGamePad.lAxisX = 0;
    if (pGamePad.lAxisY < joyDeadZone && pGamePad.lAxisY > -joyDeadZone)
        pGamePad.lAxisY = 0;
    if (pGamePad.lAxisZ < joyDeadZone && pGamePad.lAxisZ > -joyDeadZone)
        pGamePad.lAxisZ = 0;
    if (pGamePad.lAxisRz < joyDeadZone && pGamePad.lAxisRz > -joyDeadZone)
        pGamePad.lAxisRz = 0;

Error:
    SAFE_FREE(pPreparsedData);
    SAFE_FREE(pButtonCaps);
    SAFE_FREE(pValueCaps);
}