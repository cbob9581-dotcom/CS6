#include "NavigationProgress.h"
#include <iostream>
using namespace cs6;
#define CHECK(condition) do {if(!(condition)){std::cerr << "FAILED: " #condition << " line " << __LINE__ << '\n';return 1;}} while(false)
int main(){
    NavigationProgress p;
    CHECK(p.update(0,false,true,false,true)==NavAction::RequestPath);
    CHECK(p.update(0.5,false,true,true,false)==NavAction::KeepPath);
    CHECK(p.update(1.5,true,true,true,false)==NavAction::KeepPath);
    CHECK(p.update(2.5,false,true,true,false)==NavAction::KeepPath);
    CHECK(p.update(3.6,false,true,true,false)==NavAction::Recover);
    CHECK(p.recoveries==1);
    CHECK(p.update(3.7,false,true,false,false)==NavAction::KeepPath);
    CHECK(p.update(4.6,false,true,false,false)==NavAction::RequestPath);
    CHECK(p.update(5,false,false,false,false)==NavAction::Hold);
    CHECK(p.update(6,false,false,false,false)==NavAction::Hold);
    CHECK(p.update(6.2,false,true,false,false)==NavAction::RequestPath);
    CHECK(p.update(7,true,true,true,false)==NavAction::KeepPath && p.recoveries==0);
    CHECK(p.update(7.1,false,true,true,true)==NavAction::RequestPath);
    CHECK(p.update(1,false,true,false,false)==NavAction::RequestPath);
    AreaProgress area;
    CHECK(!area.update(0,0,true));CHECK(!area.update(3,30,true));
    CHECK(area.update(6.1,45,true));
    CHECK(!area.update(7,250,true));CHECK(!area.update(12,20,true));
    CHECK(area.update(13.1,15,true));CHECK(!area.update(14,0,false));
    std::cout << "Navigation: retained paths, progress, stall recovery, cooldown and idle passed\n";
}
