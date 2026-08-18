#pragma once

#include <chrono>

#include "common/common.h"

void netLoop(TcpClient* client);
void ioLoop();
void on_line(char* line);
void parseOpt(const std::string& msg,
              std::chrono::steady_clock::time_point& last_recv);