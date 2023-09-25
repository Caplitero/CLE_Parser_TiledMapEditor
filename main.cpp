

#include "cle_parser.h"
#include <iostream>
#include <chrono>

int main()
{
	CLE::TME_MapFormat map;


	
	
	

	


    auto start = std::chrono::high_resolution_clock::now();

	map.load("test.cle");

	auto stop = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout << "Time taken by code section: " << duration.count() << " microseconds" << std::endl;




	std::cout << map.Project_root.Children.size();

 	return 0;
}