#include "chip8.h"

#include <stdio.h>

// ********************************************************************************************************************
// ********************************************************************************************************************
void chip8Run()
{
    uint64_t prevTick; // When the last instruction was processed
    QueryPerformanceCounter(&prevTick);

    while (_chip8_Running)
    {
        Sleep(10);

        if (_chip8_Reset)
        {
            chip8Init();
        }

        // Update the delay/sound timer as necessary
        chip8TimerUpdate();

        // Run enough instructions to simulate a clock speed of _chip8_ClockSpeed
        int32_t instructionsToExecute = getElapsedTimeSinceHighPerfTick(prevTick) * _chip8_ClockSpeed;
        while (instructionsToExecute-- > 0)
        {
            uint16_t ins = chip8ReadInstruction();
            if (ins == 0) break;
            chip8ProcessInstruction(ins);
            QueryPerformanceCounter(&prevTick);
        }
    }
}

// ********************************************************************************************************************
// ********************************************************************************************************************
uint16_t chip8ReadInstruction()
{
    // NOTE: Instructions are two bytes and stored as big endian
    return (_chip8_Mem[_chip8_ProgramCounter] << 8) | _chip8_Mem[_chip8_ProgramCounter + 1];
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void chip8ProcessInstruction(uint16_t instruction)
{
    uint16_t nnn = instruction & 0x0FFF;
    uint8_t x = (instruction & 0x0F00) >> 8;
    uint8_t kk = instruction & 0x00FF;
    uint8_t y = (instruction & 0x00F0) >> 4;
    uint8_t n = instruction & 0x000F;

    chip8BuildDebugString(instruction);

    if (instruction == 0x00E0)
    {
        // 00E0 - CLS
        // Clear the display.
        memset(_chip8_Screen, 0, CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT);
        _chip8_ProgramCounter += 2;
    }
    else if (instruction == 0x00EE)
    {
        // 00EE - RET
        // Return from a subroutine. The interpreter sets the program counter to the address at the top of the stack,
        // then subtracts 1 from the stack pointer.
        _chip8_ProgramCounter = _chip8_Stack[_chip8_StackPointer];
        if (_chip8_StackPointer > 0) _chip8_StackPointer--;
    }
    else if ((instruction & 0xF000) == 0x1000)
    {
        // 1nnn - JP addr
        // Jump to location nnn. The interpreter sets the program counter to nnn.
        _chip8_ProgramCounter = nnn;
    }
    else if ((instruction & 0xF000) == 0x2000)
    {
        // 2nnn - CALL addr
        // Call subroutine at nnn. The interpreter increments the stack pointer, then puts the current PC on the top of
        // the stack. The PC is then set to nnn.
        _chip8_Stack[++_chip8_StackPointer] = _chip8_ProgramCounter + 2;
        _chip8_ProgramCounter = nnn;
    }
    else if ((instruction & 0xF000) == 0x3000)
    {
        // 3xkk - SE Vx, byte
        // Skip next instruction if Vx = kk. The interpreter compares register Vx to kk, and if they are equal,
        // increments the program counter by 2.
        _chip8_ProgramCounter += 2;
        if (_chip8_GenRegs[x] == kk)
        {
            _chip8_ProgramCounter += 2;
        }
    }
    else if ((instruction & 0xF000) == 0x4000)
    {
        // 4xkk - SNE Vx, byte
        // Skip next instruction if Vx != kk. The interpreter compares register Vx to kk, and if they are not equal,
        // increments the program counter by 2.
        _chip8_ProgramCounter += 2;
        if (_chip8_GenRegs[x] != kk)
        {
            _chip8_ProgramCounter += 2;
        }
    }
    else if ((instruction & 0xF00F) == 0x5000)
    {
        // 5xy0 - SE Vx, Vy
        // Skip next instruction if Vx = Vy.The interpreter compares register Vx to register Vy, and if they are equal,
        // increments the program counter by 2.
        _chip8_ProgramCounter += 2;
        if (_chip8_GenRegs[x] == _chip8_GenRegs[y])
        {
            _chip8_ProgramCounter += 2;
        }
    }
    else if ((instruction & 0xF000) == 0x6000)
    {
        // 6xkk - LD Vx, byte
        // Set Vx = kk. The interpreter puts the value kk into register Vx.
        _chip8_GenRegs[x] = kk;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF000) == 0x7000)
    {
        // 7xkk - ADD Vx, byte
        // Set Vx = Vx + kk. Adds the value kk to the value of register Vx, then stores the result in Vx.
        _chip8_GenRegs[x] += kk;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8000)
    {
        // 8xy0 - LD Vx, Vy
        // Set Vx = Vy. Stores the value of register Vy in register Vx.
        _chip8_GenRegs[x] = _chip8_GenRegs[y];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8001)
    {
        // 8xy1 - OR Vx, Vy
        // Set Vx = Vx OR Vy. Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. A bitwise
        // OR compares the corrseponding bits from two values, and if either bit is 1, then the same bit in the result
        // is also 1. Otherwise, it is 0.
        _chip8_GenRegs[x] |= _chip8_GenRegs[y];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8002)
    {
        // 8xy2 - AND Vx, Vy
        // Set Vx = Vx AND Vy. Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. A
        // bitwise AND compares the corrseponding bits from two values, and if both bits are 1, then the same bit in the
        // result is also 1. Otherwise, it is 0.
        _chip8_GenRegs[x] &= _chip8_GenRegs[y];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8003)
    {
        // 8xy3 - XOR Vx, Vy
        // Set Vx = Vx XOR Vy. Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx.
        // An exclusive OR compares the corrseponding bits from two values, and if the bits are not both the same, then
        // the corresponding bit in the result is set to 1. Otherwise, it is 0.
        _chip8_GenRegs[x] ^= _chip8_GenRegs[y];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8004)
    {
        // 8xy4 - ADD Vx, Vy
        // Set Vx = Vx + Vy, set VF = carry. The values of Vx and Vy are added together. If the result is greater than 8
        // bits (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in
        // Vx.
        uint16_t sum = _chip8_GenRegs[x] + _chip8_GenRegs[y];
        _chip8_GenRegs[x] = sum & 0x00FF;
        if ((sum & 0xFF00) > 0)
            _chip8_GenRegs[0xF] = 1;
        else
            _chip8_GenRegs[0xF] = 0;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8005)
    {
        // 8xy5 - SUB Vx, Vy
        // Set Vx = Vx - Vy, set VF = NOT borrow. If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted
        // from Vx, and the results stored in Vx.
        if (_chip8_GenRegs[x] > _chip8_GenRegs[y])
            _chip8_GenRegs[0xF] = 1;
        else
            _chip8_GenRegs[0xF] = 0;

        _chip8_GenRegs[x] -= _chip8_GenRegs[y];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8006)
    {
        // 8xy6 - SHR Vx {, Vy}
        // Set Vx = Vx SHR 1. If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is
        // divided by 2.
        uint8_t valToShift = _chip8_GenRegs[y];
        if (_chip8_ShiftQuirkMode) valToShift = _chip8_GenRegs[x];

        if ((valToShift & 0x01) == 0x01)
            _chip8_GenRegs[0xF] = 1;
        else
            _chip8_GenRegs[0xF] = 0;

        _chip8_GenRegs[x] = valToShift >> 1;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x8007)
    {
        // 8xy7 - SUBN Vx, Vy
        // Set Vx = Vy - Vx, set VF = NOT borrow. If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted
        // from Vy, and the results stored in Vx.
        if (_chip8_GenRegs[y] > _chip8_GenRegs[x])
            _chip8_GenRegs[0xF] = 1;
        else
            _chip8_GenRegs[0xF] = 0;

        _chip8_GenRegs[x] = _chip8_GenRegs[y] - _chip8_GenRegs[x];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x800E)
    {
        // 8xyE - SHL Vx {, Vy}
        // Set Vx = Vx SHL 1. If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is
        // multiplied by 2.
        // NOTE: This does not agree with other chip 8 instruction references?!

        uint8_t valToShift = _chip8_GenRegs[y];
        if (_chip8_ShiftQuirkMode) valToShift = _chip8_GenRegs[x];

        if ((valToShift & 0x80) == 0x80)
            _chip8_GenRegs[0xF] = 1;
        else
            _chip8_GenRegs[0xF] = 0;

        _chip8_GenRegs[x] = valToShift << 1;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF00F) == 0x9000)
    {
        // 9xy0 - SNE Vx, Vy
        // Skip next instruction if Vx != Vy. The values of Vx and Vy are compared, and if they are not equal, the
        // program counter is increased by 2.
        _chip8_ProgramCounter += 2;
        if (_chip8_GenRegs[x] != _chip8_GenRegs[y])
        {
            _chip8_ProgramCounter += 2;
        }
    }
    else if ((instruction & 0xF000) == 0xA000)
    {
        // Annn - LD I, addr
        // Set I = nnn. The value of register I is set to nnn.
        _chip8_I = nnn;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF000) == 0xB000)
    {
        // Bnnn - JP V0, addr
        // Jump to location nnn + V0. The program counter is set to nnn plus the value of V0.
        _chip8_ProgramCounter = nnn + _chip8_GenRegs[0];
    }
    else if ((instruction & 0xF000) == 0xC000)
    {
        // Cxkk - RND Vx, byte
        // Set Vx = random byte AND kk. The interpreter generates a random number from 0 to 255, which is then ANDed
        // with the value kk. The results are stored in Vx. See instruction 8xy2 for more information on AND.
        _chip8_GenRegs[x] = (rand() % 256) & kk;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF000) == 0xD000)
    {
        // Dxyn - DRW Vx, Vy, nibble
        // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision. The interpreter reads n
        // bytes from memory, starting at the address stored in I. These bytes are then displayed as sprites on screen
        // at coordinates (Vx, Vy). Sprites are XORed onto the existing screen. If this causes any pixels to be erased,
        // VF is set to 1, otherwise it is set to 0. If the sprite is positioned so part of it is outside the
        // coordinates of the display, it wraps around to the opposite side of the screen. See instruction 8xy3 for more
        // information on XOR, and section 2.4, Display, for more information on the Chip-8 screen and sprites.

        uint8_t x = _chip8_GenRegs[(instruction & 0x0F00) >> 8];
        uint8_t y = _chip8_GenRegs[(instruction & 0x00F0) >> 4];
        bool pixelCleared = false;
        for (int rowNum = 0; rowNum < n; rowNum++)
        {
            uint8_t row = _chip8_Mem[_chip8_I + rowNum]; // Read a row of pixels

            // Display on the screen, start with MSB because it's "left most"
            for (int bitNum = 7; bitNum >= 0; bitNum--)
            {
                bool bit = (row >> bitNum) & 0x01;

                uint8_t bitOffset = 7 - bitNum;

                // Get the x/y positions considering the bit # and row # of the sprite
                uint8_t xPos = x + bitOffset;
                uint8_t yPos = y + rowNum;
                xPos %= CHIP8_SCREEN_WIDTH;
                yPos %= CHIP8_SCREEN_HEIGHT;

                bool oldValue = _chip8_Screen[xPos][yPos];

                // XOR the bit into the screen
                _chip8_Screen[xPos][yPos] ^= bit;

                if (oldValue && !_chip8_Screen[xPos][yPos])
                {
                    pixelCleared = true;
                }
            }
        }

        _chip8_GenRegs[0xF] = pixelCleared;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xE09E)
    {
        // Ex9E - SKP Vx
        // Skip next instruction if key with the value of Vx is pressed. Checks the keyboard, and if the key
        // corresponding to the value of Vx is currently in the down position, PC is increased by 2.
        uint8_t key = _chip8_GenRegs[x];
        _chip8_ProgramCounter += 2;
        if (_chip8_Keyboard[key])
        {
            _chip8_ProgramCounter += 2;
        }
    }
    else if ((instruction & 0xF0FF) == 0xE0A1)
    {
        // ExA1 - SKNP Vx
        // Skip next instruction if key with the value of Vx is not pressed. Checks the keyboard, and if the key
        // corresponding to the value of Vx is currently in the up position, PC is increased by 2.
        uint8_t key = _chip8_GenRegs[x];
        _chip8_ProgramCounter += 2;
        if (!_chip8_Keyboard[key])
        {
            _chip8_ProgramCounter += 2;
        }
    }
    else if ((instruction & 0xF0FF) == 0xF007)
    {
        // Fx07 - LD Vx, DT
        // Set Vx = delay timer value. The value of DT is placed into Vx.
        _chip8_GenRegs[x] = _chip8_DelayTimerReg;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xF00A)
    {
        // Fx0A - LD Vx, K
        // Wait for a key press, store the value of the key in Vx. All execution stops until a key is pressed, then the
        // value of that key is stored in Vx.
        uint32_t key = 0;
        do
        {
            if (_chip8_Keyboard[key])
            {
                _chip8_GenRegs[x] = key;
                _chip8_ProgramCounter += 2; // Only increment PC on key press
                break;
            }
        } while (++key <= 0xF);
    }
    else if ((instruction & 0xF0FF) == 0xF015)
    {
        // Fx15 - LD DT, Vx
        // Set delay timer = Vx. DT is set equal to the value of Vx.
        _chip8_DTLastSetValue = _chip8_DelayTimerReg = _chip8_GenRegs[x];
        QueryPerformanceCounter(&_chip8_DTStartTick);
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xF018)
    {
        // Fx18 - LD ST, Vx
        // Set sound timer = Vx. ST is set equal to the value of Vx.
        _chip8_STLastSetValue = _chip8_SoundTimerReg = _chip8_GenRegs[x];
        QueryPerformanceCounter(&_chip8_STStartTick);
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xF01E)
    {
        // Fx1E - ADD I, Vx
        // Set I = I + Vx. The values of I and Vx are added, and the results are stored in I.
        _chip8_I += _chip8_GenRegs[x];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xF029)
    {
        // Fx29 - LD F, Vx
        // Set I = location of sprite for digit Vx. The value of I is set to the location for the hexadecimal
        // sprite corresponding to the value of Vx. See section 2.4, Display, for more information on the
        // Chip-8 hexadecimal font.
        _chip8_I = CHIP8_HEX_SPRITE_START_OFFSET + CHIP8_HEX_SPRITE_SIZE_PER * _chip8_GenRegs[x];
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xF033)
    {
        // Fx33 - LD B, Vx
        // Store BCD representation of Vx in memory locations I, I+1, and I+2. The interpreter takes the decimal
        // value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1,
        // and the ones digit at location I+2.
        uint16_t memOffset = _chip8_I;
        uint8_t value = _chip8_GenRegs[x];
        uint8_t onesDigit = _chip8_GenRegs[x] % 10;
        uint8_t tensDigit = ((_chip8_GenRegs[x] % 100) - onesDigit) / 10;
        uint8_t hundredsDigit = ((_chip8_GenRegs[x] % 1000) - tensDigit - onesDigit) / 100;

        _chip8_Mem[memOffset] = hundredsDigit;
        _chip8_Mem[memOffset + 1] = tensDigit;
        _chip8_Mem[memOffset + 2] = onesDigit;
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xF055)
    {
        // Fx55 - LD [I], Vx
        // Store registers V0 through Vx in memory starting at location I. The interpreter copies the values of
        // registers V0 through Vx into memory, starting at the address in I.
        for (uint8_t i = 0; i <= x; i++)
        {
            _chip8_Mem[_chip8_I + i] = _chip8_GenRegs[i];
        }
        _chip8_ProgramCounter += 2;
    }
    else if ((instruction & 0xF0FF) == 0xF065)
    {
        // Fx65 - LD Vx, [I]
        // Read registers V0 through Vx from memory starting at location I. The interpreter reads values from
        // memory starting at location I into registers V0 through Vx.
        for (uint8_t i = 0; i <= x; i++)
        {
            _chip8_GenRegs[i] = _chip8_Mem[_chip8_I + i];
        }
        _chip8_ProgramCounter += 2;
    }
    else
    {
        // Unknown instruction?!
    }
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void chip8Init()
{
    _chip8_SoundPlaying = false;
    _chip8_Reset = false;

    _chip8_ClockSpeed = CHIP8_CLOCK_SPEED_HZ;

    _chip8_Mutex = CreateMutex(NULL, FALSE, NULL);
    _chip8_Mutex_Screen = CreateMutex(NULL, FALSE, NULL);

    // Clear registers/stack/memory space
    memset(_chip8_Msg, 0, CHIP8_STR_SIZE);
    memset(_chip8_Mem, 0, CHIP8_PROGRAM_START_OFFSET);
    memset(_chip8_GenRegs, 0, 16);
    memset(_chip8_Stack, 0, 32);
    memset(_chip8_Screen, 0, CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT);
    memset(_chip8_Keyboard, 0, 16);
    _chip8_I = _chip8_DelayTimerReg = _chip8_SoundTimerReg = _chip8_ProgramCounter = _chip8_StackPointer = 0;
    _chip8_ProgramCounter = CHIP8_PROGRAM_START_OFFSET;

    // Load hex digit sprites:
    // clang-format off
    unsigned char letters[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    // clang-format on
    memcpy(_chip8_Mem + CHIP8_HEX_SPRITE_START_OFFSET, letters, 80);

    // Initialize rng
    time_t t;
    srand((unsigned)time(&t));

    // Many ROMs assume quirky shifting
    // TODO: Make this configurable?
    _chip8_ShiftQuirkMode = true;

    _chip8_Running = true;
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void chip8Shutdown() { _chip8_Running = false; }

void chip8Reset() { _chip8_Reset = true; }

// ********************************************************************************************************************
// ********************************************************************************************************************
int32_t chip8LoadRom(char* filename)
{
    FILE* fp;

    // Attempt to open the file
    errno_t err = _wfopen_s(&fp, filename, TEXT("rb"));
    if (err == 0)
    {
        printf("File was opened\n");
    }
    else
    {
        printf("File *not* opened\n");
        return -1;
    }

    // Clear the ROM space
    memset(_chip8_Mem + CHIP8_PROGRAM_START_OFFSET, 0, CHIP8_MEM_SIZE - CHIP8_PROGRAM_START_OFFSET);

    // Read the ROM
    const uint32_t MAX_SIZE = 3584;
    int32_t size = fread(_chip8_Mem + CHIP8_PROGRAM_START_OFFSET, 1, MAX_SIZE, fp);
    printf("%i bytes read\n", size);

    // Verify no errors
    if (ferror(fp) != 0)
    {
        printf("File error!\n");
        size = -1;
    }
    else if (feof(fp) != 1)
    {
        printf("ROM is too large?!\n");
        size = -1;
    }

    fclose(fp);
    return size;
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void appendFormatted(char* dest, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[100];
    vsprintf(buffer, format, args);
    strcat(dest, buffer);
    va_end(args);
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void chip8BuildDebugString(uint16_t instruction)
{
    WaitForSingleObject(_chip8_Mutex, INFINITE);

    memset(_chip8_Msg, 0, sizeof(_chip8_Msg));
    char* str = _chip8_Msg;

    // Read 2 bytes at a time, identifying instructions
    uint16_t nnn = instruction & 0x0FFF;
    uint16_t kk = instruction & 0x00FF;
    uint8_t x = (instruction & 0x0F00) >> 8;
    uint8_t y = (instruction & 0x00F0) >> 4;
    uint8_t n = instruction & 0x000F;

    appendFormatted(str, "                ", instruction);
    for (int i = 0; i < 16; i++) appendFormatted(str, "     %X", i);
    appendFormatted(str, "\n");
    appendFormatted(str, "         GENERAL");
    for (int i = 0; i < 16; i++) appendFormatted(str, "    %02X", _chip8_GenRegs[i]);
    appendFormatted(str, "\n");
    appendFormatted(str, "           STACK");
    for (int i = 0; i < 16; i++) appendFormatted(str, "  %04X", _chip8_Stack[i]);
    appendFormatted(str, "\n");
    appendFormatted(str, "        KEYBOARD");
    for (int i = 0; i < 16; i++) appendFormatted(str, "     %01X", _chip8_Keyboard[i]);
    appendFormatted(str, "\n");
    appendFormatted(str, "               I  %04X\n", _chip8_I);
    appendFormatted(str, "     DELAY TIMER  %02X\n", _chip8_DelayTimerReg);
    appendFormatted(str, "     SOUND TIMER  %02X\n", _chip8_SoundTimerReg);
    appendFormatted(str, " PROGRAM COUNTER  %04X\n", _chip8_ProgramCounter);
    appendFormatted(str, "   STACK POINTER  %02X\n", _chip8_StackPointer);
    appendFormatted(str, "     INSTRUCTION  %04X ", instruction);

    if (instruction == 0x00E0)
        appendFormatted(str, "CLEAR DISPLAY");
    else if (instruction == 0x00EE)
        appendFormatted(str, "RETURN");
    else if ((instruction & 0xF000) == 0x1000)
        appendFormatted(str, "JUMP TO %3X", nnn);
    else if ((instruction & 0xF000) == 0x2000)
        appendFormatted(str, "CALL %3X", nnn);
    else if ((instruction & 0xF000) == 0x3000)
        appendFormatted(str, "SKIP IF V%X == %02X", x, kk);
    else if ((instruction & 0xF000) == 0x4000)
        appendFormatted(str, "SKIP IF V%X != %02X", x, kk);
    else if ((instruction & 0xF00F) == 0x5000)
        appendFormatted(str, "SKIP IF V%X == V%X", x, y);
    else if ((instruction & 0xF000) == 0x6000)
        appendFormatted(str, "LOAD %02X INTO V%X", kk, x);
    else if ((instruction & 0xF000) == 0x7000)
        appendFormatted(str, "SET V%X += %02X", x, kk);
    else if ((instruction & 0xF00F) == 0x8000)
        appendFormatted(str, "SET V%X = V%X", x, y);
    else if ((instruction & 0xF00F) == 0x8001)
        appendFormatted(str, "SET V%X |= V%X", x, y);
    else if ((instruction & 0xF00F) == 0x8002)
        appendFormatted(str, "SET V%X &= V%X", x, y);
    else if ((instruction & 0xF00F) == 0x8003)
        appendFormatted(str, "SET V%X ^= V%X", x, y);
    else if ((instruction & 0xF00F) == 0x8004)
        appendFormatted(str, "SET V%X += V%X, SET VF = CARRY", x, y);
    else if ((instruction & 0xF00F) == 0x8005)
        appendFormatted(str, "SET V%X -= V%X, SET VF = NOT BORROW", x, y);
    else if ((instruction & 0xF00F) == 0x8006)
        appendFormatted(str, "SET V%X = V%X >> 1, SET VF = DROPPED BIT", x, x);
    else if ((instruction & 0xF00F) == 0x8007)
        appendFormatted(str, "SET V%X = V%X - V%X, SET VF = NOT BORROW", x, y, x);
    else if ((instruction & 0xF00F) == 0x800E)
        appendFormatted(str, "SET V%X = V%X << 1, SET VF = DROPPED BIT", x, x);
    else if ((instruction & 0xF00F) == 0x9000)
        appendFormatted(str, "SKIP IF %X != %X", _chip8_GenRegs[x], _chip8_GenRegs[y]);
    else if ((instruction & 0xF000) == 0xA000)
        appendFormatted(str, "SET I = %03X", nnn);
    else if ((instruction & 0xF000) == 0xB000)
        appendFormatted(str, "JUMP TO %03X + V0", nnn);
    else if ((instruction & 0xF000) == 0xC000)
        appendFormatted(str, "SET V%X = RANDOM & %02X", x, kk);
    else if ((instruction & 0xF000) == 0xD000)
        appendFormatted(str, "DRAW %i-BYTE SPRITE AT %X,%X", n, _chip8_GenRegs[x], _chip8_GenRegs[y]);
    else if ((instruction & 0xF0FF) == 0xE09E)
        appendFormatted(str, "SKIP IF KEY AT V%X IS PRESSED", x);
    else if ((instruction & 0xF0FF) == 0xE0A1)
        appendFormatted(str, "SKIP IF KEY AT V%X IS NOT PRESSED", x);
    else if ((instruction & 0xF0FF) == 0xF007)
        appendFormatted(str, "SET V%X = DT", x);
    else if ((instruction & 0xF0FF) == 0xF00A)
        appendFormatted(str, "WAIT FOR KEY, STORE IN V%X", x);
    else if ((instruction & 0xF0FF) == 0xF015)
        appendFormatted(str, "SET DT = V%X", x);
    else if ((instruction & 0xF0FF) == 0xF018)
        appendFormatted(str, "SET ST = V%X", x);
    else if ((instruction & 0xF0FF) == 0xF01E)
        appendFormatted(str, "SET I += V%X", x);
    else if ((instruction & 0xF0FF) == 0xF029)
        appendFormatted(str, "SET I = SPRITE FOR DIGIT IN V%X", x);
    else if ((instruction & 0xF0FF) == 0xF033)
        appendFormatted(str, "STORE BCD OF V%X IN I, I+1, I+2", x);
    else if ((instruction & 0xF0FF) == 0xF055)
        appendFormatted(str, "STORE V0 THROUGH V%X AT LOCATION I", x);
    else if ((instruction & 0xF0FF) == 0xF065)
        appendFormatted(str, "LOAD V0 THROUGH V%X FROM LOCATION I", x);
    else
        appendFormatted(str, "UNKNOWN\n", instruction);

    appendFormatted(str, "\n");

    ReleaseMutex(_chip8_Mutex);
}

// ********************************************************************************************************************
// ********************************************************************************************************************
uint64_t getElapsed60HzTicksSinceHighPerfTick(uint64_t startTick)
{
    double elapsedTime = getElapsedTimeSinceHighPerfTick(startTick);
    uint64_t elapsed60HzTicks = 60 * elapsedTime;
    return elapsed60HzTicks;
}

// ********************************************************************************************************************
// ********************************************************************************************************************
double getElapsedTimeSinceHighPerfTick(uint64_t startTick)
{
    static uint64_t systemTickFreq = 0;
    if (systemTickFreq == 0) QueryPerformanceFrequency(&systemTickFreq);

    uint64_t tick;
    QueryPerformanceCounter(&tick);
    uint64_t elapsedHighPerfTicks = tick - startTick;
    return (elapsedHighPerfTicks * 1.0) / systemTickFreq;
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void chip8TimerUpdate()
{
    // If the delay timer is non-zero, determine how many 60Hz ticks have elapsed and decrement accordingly
    if (_chip8_DelayTimerReg > 0)
    {
        uint64_t elapsedTicks = getElapsed60HzTicksSinceHighPerfTick(_chip8_DTStartTick);
        if (elapsedTicks < _chip8_DTLastSetValue)
            _chip8_DelayTimerReg = _chip8_DTLastSetValue - elapsedTicks;
        else
            _chip8_DelayTimerReg = 0;
    }

    // If the sound timer is non-zero, determine how many 60Hz ticks have elapsed and decrement accordingly
    if (_chip8_SoundTimerReg > 0)
    {
        uint64_t elapsedTicks = getElapsed60HzTicksSinceHighPerfTick(_chip8_STStartTick);
        if (elapsedTicks < _chip8_STLastSetValue)
            _chip8_SoundTimerReg = _chip8_STLastSetValue - elapsedTicks;
        else
            _chip8_SoundTimerReg = 0;
    }

    // If no sound is playing but it should be, start playing it
    if (_chip8_SoundTimerReg > 0 && !_chip8_SoundPlaying)
    {
        // TODO: This won't work if the working directory is changed, should probably be updated to a better way
        PlaySound(_chip8_SoundFile, NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
        _chip8_SoundPlaying = true;
    }

    // If sound is playing but it shouldn't be, stop playing it
    if (_chip8_SoundTimerReg == 0 && _chip8_SoundPlaying)
    {
        PlaySound(NULL, NULL, NULL);
        _chip8_SoundPlaying = false;
    }
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void chip8GetScreen(bool* pScreen)
{
    WaitForSingleObject(_chip8_Mutex_Screen, INFINITE);
    memcpy(pScreen, _chip8_Screen, sizeof(_chip8_Screen));
    ReleaseMutex(_chip8_Mutex_Screen);
}