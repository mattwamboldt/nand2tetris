@echo off

copy /y projects\12\Array.jack projects\12\ArrayTest\Array.jack
toolchain\bin\Release\jackcompiler.exe projects\12\ArrayTest

copy /y projects\12\Keyboard.jack projects\12\KeyboardTest\Keyboard.jack
toolchain\bin\Release\jackcompiler.exe projects\12\KeyboardTest

copy /y projects\12\Math.jack projects\12\MathTest\Math.jack
toolchain\bin\Release\jackcompiler.exe projects\12\MathTest

copy /y projects\12\Memory.jack projects\12\MemoryTest\Memory.jack
toolchain\bin\Release\jackcompiler.exe projects\12\MemoryTest

copy /y projects\12\Output.jack projects\12\OutputTest\Output.jack
toolchain\bin\Release\jackcompiler.exe projects\12\OutputTest

copy /y projects\12\Screen.jack projects\12\ScreenTest\Screen.jack
toolchain\bin\Release\jackcompiler.exe projects\12\ScreenTest

copy /y projects\12\String.jack projects\12\StringTest\String.jack
toolchain\bin\Release\jackcompiler.exe projects\12\StringTest

copy /y projects\12\Sys.jack projects\12\SysTest\Sys.jack
toolchain\bin\Release\jackcompiler.exe projects\12\SysTest
