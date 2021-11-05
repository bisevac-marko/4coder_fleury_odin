@echo off
call ..\bin\build_one_time .\4coder_fleury_jai_lexer_gen.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_jai.h .\generated\4coder_fleury_lexer_jai.h
copy ..\generated\lexer_jai.cpp .\generated\4coder_fleury_lexer_jai.cpp

call ..\bin\build_one_time .\4coder_marko_odin_lexer_gen.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_odin.h .\generated\4coder_marko_lexer_odin.h
copy ..\generated\lexer_odin.cpp .\generated\4coder_marko_lexer_odin.cpp