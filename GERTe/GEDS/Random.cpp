#include "Random.h"
#include <random>

std::mt19937 engine;

uint16_t genInt() {
	return engine();
}
