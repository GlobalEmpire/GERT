#include "Random.h"
#include <random>

std::mt19937 engine;

uint16_t random() {
	return engine();
}
