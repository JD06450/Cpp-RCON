#!/usr/bin/env /bin/bash

cmake -B build && cmake --build build --target Exe-Cpp-RCON

if test -f "build/open-rcon"; then
	mv build/open-rcon ./
fi