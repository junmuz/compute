/* This file presents unit test for the Threefry Random Number Generator */

#include <iostream>
#include <vector>
#include <boost/compute/random/threefry.h>
#include <cassert>
int main(int argc, char **argv){
    int i;
    std::vector <unsigned long long> test_result(20);
    test_result.push_back(0xe7f9b3eaf8551101); 
    test_result.push_back(0x4673e4eba167c2c0); 
    test_result.push_back(0xdf218d5b1b97cbd2); 
    test_result.push_back(0xa0bd12b358f78559);
    test_result.push_back(0xb977e699a63dd0b4); 
    test_result.push_back(0x139d5e64a9653714);
    test_result.push_back(0x71a716d3adbd860d);
    test_result.push_back(0x2b412fcc92ef8ccb);
    test_result.push_back(0x3b3f8d195fe87ffa);
    test_result.push_back(0xf35b68dc70b3ed3);
    test_result.push_back(0xa0ad4ea90a6ac666);
    test_result.push_back(0x1139c7b4bc117ca1);
    test_result.push_back(0xcc0f1d607e20f245);
    test_result.push_back(0x6ccd1ec7129e9eb5);
    test_result.push_back(0xa2a79496dfa47352);
    test_result.push_back(0x65ca10886e2566df);
    test_result.push_back(0xed553e57f10b3b42);
    test_result.push_back(0xbaf51c00fb3a5957);
    test_result.push_back(0x6f81ed42f350084d);
    test_result.push_back(0xc2b6e3a8c2c69865);
    threefry2x64_ctr_t  ctr = {{2,0}};
    threefry2x64_key_t key = {{0x0,0x0}};
    for(i=0; i<10; ++i){
        ctr.v[0] = i;
        std::cout << "Running Test " << i + 1 << std::endl;
        {
          threefry2x64_ctr_t rand = threefry2x64(ctr, key);
          assert(rand.v[0] == test_result.back());
          test_result.pop_back();
          assert(rand.v[1] == test_result.back());
          test_result.pop_back();
        }
    }
    std::cout << "All Tests Successfully Passed" << std::endl;
    return 0;
}

