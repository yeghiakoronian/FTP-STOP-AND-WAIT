// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS // this line was suggested qby the compiler (to avoid using strcpu_s )

#include "targetver.h"


#include <Windows.h>
#include <lmcons.h>
#include <stdio.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <vector>
using namespace std;

#pragma comment(lib, "Ws2_32.lib") //liks to winsosk library
#include "userDefinedTypes.h"
#include "ConcoleThread.h"
#include "Client.h"
#include <stdio.h>

#include <time.h>


// TODO: reference additional headers your program requires here
