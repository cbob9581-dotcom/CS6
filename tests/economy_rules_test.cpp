#include "EconomyRules.h"
#include <iostream>
using namespace cs6;
#define CHECK(condition) do {if(!(condition)){std::cerr << "FAILED: " #condition << " line " << __LINE__ << '\n';return 1;}} while(false)
int main(){
    Economy a;BuyContext allowed{true,true,true};
    CHECK(a.cash==800 && a.gear.pistol==Item::Glock);
    CHECK(a.buy(Item::AK47,allowed)==BuyResult::InsufficientFunds && a.cash==800);
    a.credit(3000);CHECK(a.buy(Item::AK47,allowed)==BuyResult::Ok && a.cash==1100);
    CHECK(a.refund(Item::AK47,allowed)==BuyResult::Ok && a.cash==3800 && a.gear.primary==Item::None);
    CHECK(a.buy(Item::AK47,allowed)==BuyResult::Ok);a.markUsed(Item::AK47);
    CHECK(a.refund(Item::AK47,allowed)==BuyResult::Used);
    CHECK(a.buy(Item::M4A4,allowed)==BuyResult::WrongSide);
    CHECK(a.buy(Item::P250,{true,false,true})==BuyResult::OutsideZone);
    CHECK(a.buy(Item::P250,{false,true,true})==BuyResult::Dead);
    CHECK(a.buy(Item::P250,{true,true,false})==BuyResult::TimeExpired);
    a.credit(99999);CHECK(a.cash==16000);
    CHECK(a.buy(Item::Armor,allowed)==BuyResult::Ok && a.cash==15350 && a.gear.armor==100);
    CHECK(a.buy(Item::Helmet,allowed)==BuyResult::Ok && a.cash==15000 && a.gear.helmet);
    CHECK(a.refund(Item::Helmet,allowed)==BuyResult::Ok && a.cash==15350 && !a.gear.helmet && a.gear.armor==100);
    CHECK(a.buy(Item::Flash,allowed)==BuyResult::Ok);CHECK(a.buy(Item::Flash,allowed)==BuyResult::Ok);
    CHECK(a.buy(Item::Flash,allowed)==BuyResult::LimitReached);
    CHECK(a.buy(Item::Smoke,allowed)==BuyResult::Ok && a.buy(Item::HE,allowed)==BuyResult::Ok);
    CHECK(a.buy(Item::Molotov,allowed)==BuyResult::LimitReached);
    a.startRound(true);CHECK(a.gear.primary==Item::AK47 && a.gear.armor==100);
    CHECK(a.refund(Item::AK47,allowed)==BuyResult::NotPurchased);
    a.startRound(false);CHECK(a.gear.primary==Item::None && a.gear.pistol==Item::Glock && a.gear.armor==0 && a.grenadeCount()==0);
    a.resetHalf(false);CHECK(a.cash==800 && a.gear.pistol==Item::USP && a.lossLevel==1);
    CHECK(a.buy(Item::DefuseKit,allowed)==BuyResult::Ok && a.cash==400 && a.gear.kit);
    CHECK(a.buy(Item::Molotov,allowed)==BuyResult::WrongSide);
    Economy loss;loss.awardRound(false,Reason::Eliminated,false,false);CHECK(loss.cash==2700 && loss.lossLevel==2);
    loss.awardRound(false,Reason::Eliminated,false,false);CHECK(loss.cash==5100 && loss.lossLevel==3);
    loss.awardRound(true,Reason::Exploded,true,true);CHECK(loss.cash==8600 && loss.lossLevel==2);
    Economy saver;saver.awardRound(false,Reason::Timeout,true,false);CHECK(saver.cash==800 && saver.lossLevel==2);
    Economy plant;plant.awardRound(false,Reason::Defused,false,true);CHECK(plant.cash==3500);
    Economy refund;refund.credit(4000);CHECK(refund.buy(Item::AK47,allowed)==BuyResult::Ok);
    CHECK(refund.buy(Item::Galil,allowed)==BuyResult::Ok);CHECK(refund.refund(Item::AK47,allowed)==BuyResult::NotOwned);
    CHECK(refund.refund(Item::Galil,allowed)==BuyResult::Ok && refund.gear.primary==Item::None);
    Economy protection;protection.credit(5000);CHECK(protection.buy(Item::Armor,allowed)==BuyResult::Ok);
    CHECK(protection.absorb(40,0.5f,false)==20 && protection.gear.armor==90);
    CHECK(protection.absorb(100,0.5f,true)==100 && protection.gear.armor==90);
    CHECK(protection.refund(Item::Armor,allowed)==BuyResult::Used);
    CHECK(protection.buy(Item::Helmet,allowed)==BuyResult::Ok);
    CHECK(protection.absorb(100,0.5f,true)==50 && protection.gear.armor==75);
    protection.gear.armor=5;CHECK(protection.absorb(100,0.5f,false)==90 && protection.gear.armor==0);
    Economy trading;trading.credit(5000);CHECK(trading.buy(Item::AK47,allowed)==BuyResult::Ok);
    CHECK(trading.drop(Slot::Primary)==Item::AK47 && trading.gear.primary==Item::None);
    CHECK(trading.refund(Item::AK47,allowed)==BuyResult::NotOwned);
    CHECK(trading.pickUp(Item::M4A4)==Item::None && trading.gear.primary==Item::M4A4);
    CHECK(trading.refund(Item::M4A4,allowed)==BuyResult::NotPurchased);
    CHECK(trading.pickUp(Item::AK47)==Item::M4A4 && trading.gear.primary==Item::AK47);
    std::cout << "Economy: purchase/refund/limits/rewards/survival/halftime/armor passed\n";
}
