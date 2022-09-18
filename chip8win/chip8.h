#ifndef CHIP_8_WIN_
#define CHIP_8_WIN_

#define _CRT_SECURE_NO_WARNINGS // let me use sprintf/vsprintf!

#include <Windows.h>
#include <stdbool.h>
#include <stdint.h>

#define CHIP8_MEM_SIZE 4096
#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32
#define CHIP8_PROGRAM_START_OFFSET 0x200
#define CHIP8_HEX_SPRITE_START_OFFSET 0x100
#define CHIP8_HEX_SPRITE_SIZE_PER 5
#define CHIP8_CLOCK_SPEED_HZ 500 // Online sources say 500Hz is a good CHIP-8 emulator clock speed  TODO: Configurable?
#define CHIP8_STR_SIZE 2048

// Various registers and other strucures defined in the CHIP-8 spec
uint8_t _chip8_Mem[CHIP8_MEM_SIZE];
uint8_t _chip8_GenRegs[16];
uint16_t _chip8_I;
uint8_t _chip8_DelayTimerReg;
uint8_t _chip8_SoundTimerReg;
uint16_t _chip8_ProgramCounter;
uint8_t _chip8_StackPointer;
uint16_t _chip8_Stack[16];
uint32_t _chip8_ClockSpeed;

bool _chip8_Screen[CHIP8_SCREEN_WIDTH][CHIP8_SCREEN_HEIGHT];

bool _chip8_Keyboard[16];        // Tracks status of the keys
bool _chip8_Running;             // True while the emulator is running
char _chip8_Msg[CHIP8_STR_SIZE]; // String description of the various structures
char _chip8_SoundFile[260];      // Location of the sound file (note: wchar because windows)
bool _chip8_SoundPlaying;        // Flag that tracks whether or not a sound is playing
uint64_t _chip8_DTStartTick;     // The system timer at the moment the delay timer was set
uint64_t _chip8_STStartTick;     // The system timer at the moment the sound timer was set
uint8_t _chip8_DTLastSetValue;   // The value the delay timer was last set to
uint8_t _chip8_STLastSetValue;   // The value the delay timer was last set to
bool _chip8_ShiftQuirkMode;      // Different CHIP-8 docs disagree on exact details of SHL/SHR
bool _chip8_Reset;               // If true, re-initializes all registers
HANDLE _chip8_Mutex;             // Mutex used for exclusive access to the debug msg
HANDLE _chip8_Mutex_Screen;      // Mutex used for exclusive access to the screen buffer

// Initializes the chip 8 emulator.  Must be called before *any* other function.
void chip8Init();

// Interpreter function.  Runs the ROM.  Does not return until chip8Shutdown() is called
void chip8Run();

// Updates the timer registers if necessary (also starts/stops sound)
void chip8TimerUpdate();

// Shuts down the emulator
void chip8Shutdown();

// Sets the reset flag.  Emulator will reset before processing the next instruction.
void chip8Reset();

// Loads a Chip-8 ROM at PROGRAM_START_OFFSET
int32_t chip8LoadRom(WCHAR* filename);

// Surprise: processes a single instruction
void chip8ProcessInstruction(uint16_t instruction);

// Reads a single instruction at the program counter
uint16_t chip8ReadInstruction();

// Helper for the timer thread.  Used to calculate how many 60Hz ticks have elapsed while it slept.
uint64_t getElapsed60HzTicksSinceHighPerfTick(uint64_t startTick);

// Given a tick (QueryPerformanceCounter), get the elapsed time in seconds
double getElapsedTimeSinceHighPerfTick(uint64_t startTick);

// Creates a summary of the various registers as well as a description of the current instruction
void chip8BuildDebugString(uint16_t instruction);

// Gets a copy of the screen buffer.  Thread-safe.
void chip8GetScreen(bool* pScreen);

#endif