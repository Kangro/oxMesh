#pragma once

#ifdef _WIN32
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件:
#define NOMINMAX
#include <windows.h>
#endif

//#include "pplLib.h"


#include <string>
#include <vector>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <iostream>
#include <map>

#include "Include/oxTool.h"

#include "occlib.h"
#include "gmshlib.h"
