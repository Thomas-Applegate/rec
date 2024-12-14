#include "input_parse.h"

int main(int argc, const char** argv)
{
	std::unordered_map<std::string, Dfa> dfa_map = parse_input(argc, argv);
	return 0;
}
