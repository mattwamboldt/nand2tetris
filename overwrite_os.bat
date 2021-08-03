@echo off

toolchain\bin\Release\jackcompiler.exe projects\12
move /y projects\12\Array.vm tools\OS\Array.vm
move /y projects\12\Keyboard.vm tools\OS\Keyboard.vm
move /y projects\12\Math.vm tools\OS\Math.vm
move /y projects\12\Memory.vm tools\OS\Memory.vm
move /y projects\12\Output.vm tools\OS\Output.vm
move /y projects\12\Screen.vm tools\OS\Screen.vm
move /y projects\12\String.vm tools\OS\String.vm
move /y projects\12\Sys.vm tools\OS\Sys.vm
