#pragma once

#include <chrono>

#include "common/common.h"

void netLoop(TcpClient* client);
void ioLoop();
void on_line(char* line);
int splitLines(const std::string& str, std::vector<std::string>& lines);
void processLine(const std::string& line);
void parseOpt(const std::string& msg,
              std::chrono::steady_clock::time_point& last_recv);