#include "input_parse.h"
#include "insert_order_map.h"

#ifdef DEBUG
#include <iostream>
#endif

int main(int argc, const char** argv)
{
	insert_order_map<std::string, token_data> token_map = parse_input(argc, argv);
#ifdef DEBUG
	std::cout << "debug: program completed successfully\n";
#endif
	return 0;
}
