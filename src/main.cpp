#include <iostream>
#include "math/math.h"
#include "asset.h"


int main() 
{
	try {
		sparki::convert_fbx_to_geo("../../data/plane.fbx", "../../data/plane.geo");
		auto m = sparki::read_geo("../../data/plane.geo");
	}
	catch (const std::exception& e) {
		const std::string msg = sparki::exception_message(e);
		std::cout << "----- Exception -----" << std::endl << msg << std::endl;
	}
	
	std::cin.get();
	return 0;
}
