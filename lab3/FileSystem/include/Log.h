#pragma once

#include <iostream>

#define INFO(msg)  printf("\033[0m\033[1;32m%s\033[0m", msg)
#define WARN(msg)  printf("\033[0m\033[1;33m%s\033[0m", msg)
#define ERROR(msg) printf("\033[0m\033[1;31m%s\033[0m", msg)
