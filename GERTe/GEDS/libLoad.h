#pragma once
#include "Status.h"
#include "Version.h"
using namespace std;

Status loadLibs();
Version* getVersion(unsigned char);
unsigned char highestVersion();
