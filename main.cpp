#include "input_parse.h"

#ifdef DEBUG
#include <iostream>
#endif

int main(int argc, const char** argv)
{
	token_data_map token_map = parse_input(argc, argv);
#ifdef DEBUG
	std::cout << "debug: program completed successfully\n";
#endif
	return 0;
}
