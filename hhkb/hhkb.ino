/*
 * Yet Another Controller for Happy Hacking Keyboard Professional2
 * This controller is designed for Arduino UNO.
 *
 * !DISCLAIMER!
 * This software is absolutely NOT perfect and may damage to your keyboard.
 * Please use this software only if you have mature knowledge of electronical 
 * enginnering and understand the risk of this software.
 * I'm not accountable for the damage caused by using this software.
 */

#define KEY_ROLL_OVER 6
#define MAX_ROWS 8
#define MAX_COLS 8
#define KEY_PRESSED LOW
#define COL_ENABLE LOW
#define COL_DISABLE HIGH
#define UNUSED 0
#define HID_KEYCODE_MODIFIER_MIN 0xE0
#define HID_KEYCODE_MODIFIER_MAX 0xE7
#define STATE_ON 0x01
#define STATE_OFF 0x00
#define SCAN_RATE 15
#define BT_MODE // Bluetooth Mode
//#define DEBUG

/* Arduino Pins */
int muxRowControlPin[] = {2, 3, 4};
int muxColControlPin[] = {5, 6, 7};
int enableColPin = 8;
int keyReadPin = 9;

/* Multiplexer channel */
int muxChannel[8][3] = {
    {0, 0, 0}, // channel 0
    {1, 0, 0}, // channel 1
    {0, 1, 0}, // channel 2
    {1, 1, 0}, // channel 3
    {0, 0, 1}, // channel 4
    {1, 0, 1}, // channel 5
    {0, 1, 1}, // channel 6
    {1, 1, 1}, // channel 7
};

uint8_t zeroState[MAX_ROWS][MAX_COLS] = {0};
uint8_t prevState[MAX_ROWS][MAX_COLS];
uint8_t currentState[MAX_ROWS][MAX_COLS];

/* KEYMAP KEY TO HID KEYCODE*/
uint8_t KEYMAP[MAX_ROWS][MAX_COLS] = {
    {0x1F/* 2 */, 0x14/* q */,   0x1A/* w */,      0x16/* s */,       0x04/* a */,       0x1D/* z */,       0x1B/* x */,     0x06/* c */}, 
    {0x20/* 3 */, 0x21/* 4 */,   0x15/* r */,      0x08/* e */,       0x07/* d */,       0x09/* f */,       0x19/* v */,     0x05/* b */}, 
    {0x22/* 5 */, 0x23/* 6 */,   0x1C/* y */,      0x17/* t */,       0x0A/* g */,       0x0B/* h */,       0x11/* n */,     UNUSED}, 
    {0x1E/* 1 */, 0x29/* ESC */, 0x2B/* TAB */,    0xE0/* CONTROL */, 0xE1/* L-SHIFT */, 0xE2/* L-Alt */,   0xE3/* L-GUI */, 0x2C/* SPACE */}, 
    {0x24/* 7 */, 0x25/* 8 */,   0x18/* u */,      0x0C/* i */,       0x0E/* k */,       0x0D/* j */,       0x10/* m */,     UNUSED}, 
    {0x31/* \ */, 0x35 /* ` */,  0x2A/* DELETE */, 0x28/* RETURN */,  UNUSED/* Fn */,    0xE5/* R-SHIFT */, 0xE6/* R-Alt */, 0xE7/* R-GUI */}, 
    {0x26/* 9 */, 0x27/* 0 */,   0x12/* o */,      0x13/* p */,       0x33/* ; */,       0x0F/* l */,       0x36/* , */,     UNUSED}, 
    {0x2D/* - */, 0x2E/* = */,   0x30/* ] */,      0x2F/* [ */,       0x34/* ' */,       0x38/* / */,       0x37/* . */,     UNUSED} 
};

void setup()
{
    Serial.begin(115200);

    // set pin mode
    for (int i = 0; i < 3; i++) {
        pinMode(muxRowControlPin[i], OUTPUT);
        pinMode(muxColControlPin[i], OUTPUT);
    }
    pinMode(enableColPin, OUTPUT);
    pinMode(keyReadPin, INPUT);

    // initialize pin status
    for (int i = 0; i < 3; i++) {
        digitalWrite(muxRowControlPin[i], LOW);
        digitalWrite(muxColControlPin[i], LOW);
    }
    digitalWrite(enableColPin, COL_DISABLE);

    initializeState(prevState);
    initializeState(currentState);
}

/* Scan Keyboard Matrix */
void loop()
{
    unsigned long scanStart = millis();

    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            readKey(row, col, currentState);
        }
    }
    
    // Check if some keyes are pressed currently
    if (isAnyKeyPressed(currentState)) {
        sendKeyCodes(currentState);

    // Check if some keyes were pressed previously,
    // but no key is pressed currently
    } else if (isAnyKeyPressed(prevState)) {
        sendKeyCodes(zeroState);
    }

    copyKeyState(currentState, prevState);

    unsigned long scanEnd = millis();

    // adjust scan rate to SCAN_RATE
    if ((scanEnd - scanStart) < SCAN_RATE) {
        delay(SCAN_RATE - (scanEnd - scanStart));
    }
}

void initializeState(uint8_t state[MAX_ROWS][MAX_COLS])
{
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            state[row][col] = STATE_OFF;
        }
    }
}

void readKey(int row, int col, uint8_t state[MAX_ROWS][MAX_COLS])
{
    // select row
    selectMux(row, muxRowControlPin);

    // select and enable column
    selectMux(col, muxColControlPin);
    enableSelectedColumn();

    if (digitalRead(keyReadPin) == KEY_PRESSED) {
#ifdef DEBUG
        Serial.print("(");
        Serial.print(row);
        Serial.print(",");
        Serial.print(col);
        Serial.println(") ON");
#endif
        state[row][col] = STATE_ON;
    } else {
        state[row][col] = STATE_OFF;
    }

    disableSelectedColumn();
}

void selectMux(int channel, int* controlPin)
{
    for (int i = 0; i < 3; i++) {
        digitalWrite(controlPin[i], muxChannel[channel][i]);
    }
    delayMicroseconds(30);
}

void enableSelectedColumn()
{
    digitalWrite(enableColPin, COL_ENABLE);
    delayMicroseconds(3);
}

void disableSelectedColumn()
{
    digitalWrite(enableColPin, COL_DISABLE);
    delayMicroseconds(30);
}

int copyKeyState(uint8_t from[MAX_ROWS][MAX_COLS],
                 uint8_t to[MAX_ROWS][MAX_COLS])
{
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            to[row][col] = from[row][col];
        }
    }
}

int isAnyKeyPressed(uint8_t state[MAX_ROWS][MAX_COLS])
{
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            if (state[row][col] == STATE_ON) return 1;
        }
    }
    return 0;
}

uint8_t keymapKeyToHidKeycode(int row, int col)
{
    return KEYMAP[row][col];
}

uint8_t ordinaryKeyToFnKey(int row, int col)
{
    if (row == 7 && col == 3) { /* [ */
        return 0x52; /* UpArrow */
    } else if (row == 7 && col == 4) { /* ' */
        return 0x4F; /* RightArrow */
    } else if (row == 7 && col == 5) { /* / */
        return 0x51; /* DownArrow */
    } else if (row == 6 && col == 4) { /* ; */
        return 0x50; /* LeftArrow */
    }

    return keymapKeyToHidKeycode(row, col);
}

void sendKeyCodes(uint8_t state[MAX_ROWS][MAX_COLS])
{
    uint8_t modifiers = 0x00;
    uint8_t keyCodes[KEY_ROLL_OVER] = {0};
    int fnFlag = 0;
    int numOfKeyDown = 0;
    int hidKeycode;

    // check FnKey(5, 4) is pressed
    if (state[5][4] == STATE_ON) {
        fnFlag = 1;
    }

    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            if (state[row][col] == STATE_ON &&
                (hidKeycode = keymapKeyToHidKeycode(row, col)) != UNUSED) {
                // modifier key is pressed
                if (HID_KEYCODE_MODIFIER_MIN <= hidKeycode && 
                    hidKeycode <= HID_KEYCODE_MODIFIER_MAX) {
                    modifiers |= (1 << (hidKeycode - HID_KEYCODE_MODIFIER_MIN));

                // ordinary key is pressed
                } else if (numOfKeyDown < KEY_ROLL_OVER) {
                    if (fnFlag) {
                        keyCodes[numOfKeyDown++] = ordinaryKeyToFnKey(row, col);
                    } else {
                        keyCodes[numOfKeyDown++] = hidKeycode;
                    }
                }
            }
        }
    }
#ifdef DEBUG
    showSendingKeyCodes(modifiers, keyCodes);
#endif

    sendKeyCodesBySerial(modifiers,
                         keyCodes[0],
                         keyCodes[1],
                         keyCodes[2],
                         keyCodes[3],
                         keyCodes[4],
                         keyCodes[5]);
}

void sendKeyCodesBySerial(uint8_t modifiers,
                          uint8_t keycode0,
                          uint8_t keycode1,
                          uint8_t keycode2,
                          uint8_t keycode3,
                          uint8_t keycode4,
                          uint8_t keycode5)
{
#ifdef BT_MODE
    Serial.write(0xFD); // Raw Report Mode
    Serial.write(0x09); // Length
    Serial.write(0x01); // Descriptor 0x01=Keyboard
#endif

    /* send key codes(8 bytes all) */
    Serial.write(modifiers); // modifier keys
    Serial.write(0x00, 1);   // reserved
    Serial.write(keycode0);  // keycode0
    Serial.write(keycode1);  // keycode1
    Serial.write(keycode2);  // keycode2
    Serial.write(keycode3);  // keycode3
    Serial.write(keycode4);  // keycode4
    Serial.write(keycode5);  // keycode5
    delay(10);
}

void showSendingKeyCodes(uint8_t modifiers, uint8_t keyCodes[KEY_ROLL_OVER])
{
    Serial.print("(");
    Serial.print(modifiers);
    Serial.print(", ");
    Serial.print(keyCodes[0]);
    Serial.print(", ");
    Serial.print(keyCodes[1]);
    Serial.print(", ");
    Serial.print(keyCodes[2]);
    Serial.print(", ");
    Serial.print(keyCodes[3]);
    Serial.print(", ");
    Serial.print(keyCodes[4]);
    Serial.print(", ");
    Serial.print(keyCodes[5]);
    Serial.println(")");
}
