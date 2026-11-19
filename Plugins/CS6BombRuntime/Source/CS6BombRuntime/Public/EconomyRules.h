#pragma once
#include "RoundRules.h"
#include <array>
#include <vector>

namespace cs6 {
enum class Item {None,Glock,USP,P250,Deagle,Mac10,MP9,MP5,UMP,P90,Galil,Famas,AK47,M4A4,M4A1S,AWP,Scout,Nova,XM1014,Mag7,Armor,Helmet,DefuseKit,Flash,Smoke,HE,Molotov,Incendiary,Count};
enum class Slot {None,Pistol,Primary,Armor,Kit,Grenade};
enum class BuyResult {Ok,Dead,OutsideZone,TimeExpired,WrongSide,InsufficientFunds,LimitReached,NotPurchased,NotOwned,Used,Unavailable};
struct Product {Item id;const char* name;Slot slot;int price;int sides=3;int killReward=300;};
inline const Product& product(Item id) {
    static const std::array<Product,size_t(Item::Count)> products={{
        {Item::None,"",Slot::None,0},{Item::Glock,"Glock-18",Slot::Pistol,200,1},{Item::USP,"USP-S",Slot::Pistol,200,2},
        {Item::P250,"P250",Slot::Pistol,300},{Item::Deagle,"Desert Eagle",Slot::Pistol,700},
        {Item::Mac10,"MAC-10",Slot::Primary,1050,1,600},{Item::MP9,"MP9",Slot::Primary,1250,2,600},
        {Item::MP5,"MP5-SD",Slot::Primary,1400,3,600},{Item::UMP,"UMP-45",Slot::Primary,1200,3,600},{Item::P90,"P90",Slot::Primary,2350},
        {Item::Galil,"Galil AR",Slot::Primary,1800,1},{Item::Famas,"FAMAS",Slot::Primary,1950,2},
        {Item::AK47,"AK-47",Slot::Primary,2700,1},{Item::M4A4,"M4A4",Slot::Primary,2900,2},{Item::M4A1S,"M4A1-S",Slot::Primary,2900,2},
        {Item::AWP,"AWP",Slot::Primary,4750,3,100},{Item::Scout,"SSG 08",Slot::Primary,1700},
        {Item::Nova,"Nova",Slot::Primary,1050,3,900},{Item::XM1014,"XM1014",Slot::Primary,2000,3,900},{Item::Mag7,"MAG-7",Slot::Primary,1300,2,900},
        {Item::Armor,"Kevlar",Slot::Armor,650},{Item::Helmet,"Kevlar + Helmet",Slot::Armor,1000},
        {Item::DefuseKit,"Defuse Kit",Slot::Kit,400,2},{Item::Flash,"Flashbang",Slot::Grenade,200},{Item::Smoke,"Smoke",Slot::Grenade,300},
        {Item::HE,"HE Grenade",Slot::Grenade,300},{Item::Molotov,"Molotov",Slot::Grenade,400,1},{Item::Incendiary,"Incendiary",Slot::Grenade,500,2}
    }};
    const int index=int(id);return products[index>=0 && index<int(Item::Count)?index:0];
}
struct BuyContext {bool alive,inBuyZone,inBuyTime;};
struct Gear {
    Item primary=Item::None,pistol=Item::Glock;
    int armor=0;bool helmet=false,kit=false;
    std::array<int,size_t(Item::Count)> grenades{};
};
struct Receipt {Item item;int paid;bool used=false,owned=true,refunded=false;int priorArmor=0;bool priorHelmet=false,priorKit=false;};
struct Economy {
    int cash=800,lossLevel=1;bool attacker=true;Gear gear;std::vector<Receipt> receipts;
    void credit(int amount){cash=std::clamp(cash+amount,0,16000);}
    int grenadeCount()const {int n=0;for(int v:gear.grenades)n+=v;return n;}
    int price(Item id)const {
        if(id==Item::Helmet)return gear.helmet?650:gear.armor==100?350:1000;
        return product(id).price;
    }
    BuyResult context(const BuyContext& c)const {
        if(!c.alive)return BuyResult::Dead;if(!c.inBuyZone)return BuyResult::OutsideZone;
        return c.inBuyTime?BuyResult::Ok:BuyResult::TimeExpired;
    }
    BuyResult buy(Item id,const BuyContext& c) {
        auto check=context(c);if(check!=BuyResult::Ok)return check;
        const auto& p=product(id);if(p.slot==Slot::None)return BuyResult::Unavailable;
        if(!(p.sides & (attacker?1:2)))return BuyResult::WrongSide;
        if((id==Item::Armor && gear.armor==100) || (id==Item::Helmet && gear.armor==100 && gear.helmet) || (id==Item::DefuseKit && gear.kit))return BuyResult::LimitReached;
        if(p.slot==Slot::Grenade && (grenadeCount()>=4 || gear.grenades[size_t(id)]>=(id==Item::Flash?2:1)))return BuyResult::LimitReached;
        const int cost=price(id);if(cash<cost)return BuyResult::InsufficientFunds;
        Receipt r{id,cost};r.priorArmor=gear.armor;r.priorHelmet=gear.helmet;r.priorKit=gear.kit;
        if(p.slot==Slot::Primary || p.slot==Slot::Pistol)for(auto& old:receipts)if(product(old.item).slot==p.slot)old.owned=false;
        switch(p.slot){
            case Slot::Primary:gear.primary=id;break;
            case Slot::Pistol:gear.pistol=id;break;
            case Slot::Armor:gear.armor=100;if(id==Item::Helmet)gear.helmet=true;break;
            case Slot::Kit:gear.kit=true;break;
            case Slot::Grenade:++gear.grenades[size_t(id)];break;
            default:break;
        }
        cash-=cost;receipts.push_back(r);return BuyResult::Ok;
    }
    BuyResult refund(Item id,const BuyContext& c) {
        auto check=context(c);if(check!=BuyResult::Ok)return check;
        for(auto i=receipts.rbegin();i!=receipts.rend();++i)if(i->item==id && !i->refunded) {
            if(!i->owned)return BuyResult::NotOwned;if(i->used)return BuyResult::Used;
            switch(product(id).slot){
                case Slot::Primary:if(gear.primary!=id)return BuyResult::NotOwned;gear.primary=Item::None;break;
                case Slot::Pistol:if(gear.pistol!=id)return BuyResult::NotOwned;gear.pistol=Item::None;break;
                case Slot::Armor:
                    if(id==Item::Armor && gear.helmet!=i->priorHelmet)return BuyResult::NotOwned;
                    gear.armor=i->priorArmor;gear.helmet=i->priorHelmet;break;
                case Slot::Kit:gear.kit=i->priorKit;break;
                case Slot::Grenade:if(gear.grenades[size_t(id)]<=0)return BuyResult::NotOwned;--gear.grenades[size_t(id)];break;
                default:return BuyResult::NotOwned;
            }
            i->refunded=true;credit(i->paid);return BuyResult::Ok;
        }
        return BuyResult::NotPurchased;
    }
    Item drop(Slot slot) {
        if(slot!=Slot::Primary && slot!=Slot::Pistol)return Item::None;
        Item& held=slot==Slot::Primary?gear.primary:gear.pistol;const Item old=held;held=Item::None;
        for(auto& receipt:receipts)if(product(receipt.item).slot==slot)receipt.owned=false;
        return old;
    }
    Item pickUp(Item item) {
        const Slot slot=product(item).slot;if(slot!=Slot::Primary && slot!=Slot::Pistol)return Item::None;
        const Item old=drop(slot);(slot==Slot::Primary?gear.primary:gear.pistol)=item;return old;
    }
    void markUsed(Item id){for(auto& r:receipts)if(r.item==id && r.owned && !r.refunded)r.used=true;}
    void startRound(bool survived){receipts.clear();if(!survived){gear=Gear{};gear.pistol=attacker?Item::Glock:Item::USP;}}
    void resetHalf(bool isAttacker){attacker=isAttacker;cash=800;lossLevel=1;startRound(false);}
    void awardRound(bool won,Reason reason,bool survived,bool planted) {
        if(won){credit(reason==Reason::Defused || reason==Reason::Exploded?3500:3250);lossLevel=std::max(0,lossLevel-1);}
        else {
            if(!(attacker && survived && reason==Reason::Timeout))credit(1400+500*lossLevel);
            if(attacker && planted)credit(800);
            lossLevel=std::min(4,lossLevel+1);
        }
    }
    float absorb(float raw,float ratio,bool head) {
        if(!std::isfinite(raw) || raw<=0)return 0;
        if(gear.armor<=0 || (head && !gear.helmet))return raw;
        float reduced=raw*std::clamp(ratio,0.f,1.f),cost=(raw-reduced)*0.5f;
        if(cost>gear.armor){reduced=raw-gear.armor*2.f;cost=float(gear.armor);}
        gear.armor=std::max(0,gear.armor-int(std::ceil(cost)));markUsed(Item::Armor);markUsed(Item::Helmet);
        return reduced;
    }
};
}
