#include "CameraRules.h"
#include <iostream>
#define CHECK(c) do{if(!(c)){std::cerr<<#c<<'\n';return 1;}}while(false)
int main(){CHECK(cs6::eyeHeight(96,96)==160);CHECK(cs6::eyeHeight(60,96)==105);CHECK(cs6::eyeHeight(20,96)==105);CHECK(cs6::eyeHeight(200,96)==160);CHECK(cs6::eyeHeight(78,96)>105 && cs6::eyeHeight(78,96)<160);std::cout<<"PASS standing/crouch eye heights are independent of animation\n";}
