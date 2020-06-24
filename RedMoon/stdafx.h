#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <locale.h>
#include <sstream>
#include <regex>
#include <mutex>
#include <curl/curl.h>

#define RET return 0
#define POP_FRONT(v) v.erase(v.begin())

using namespace std;

struct CMD 
{
	string command;
	string value;
	CMD(string _command, string _value) { command = _command; value = _value; }
};