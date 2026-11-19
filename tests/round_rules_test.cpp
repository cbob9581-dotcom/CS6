#include "RoundRules.h"
#include <cassert>
#include <iostream>
using namespace cs6;
#undef assert
#define assert(condition) do {if(!(condition)){std::cerr << "FAILED: " #condition << " line " << __LINE__ << '\n';return 1;}} while(false)
int main() {
    Rules r;
    r.advance(15); assert(r.phase == Phase::Live);
    assert(r.beginPlant(1, true)); r.advance(3.2);
    assert(r.phase == Phase::Planted);
    r.setAlive(0, 3); assert(r.phase == Phase::Planted);
    assert(r.beginDefuse(2, false)); r.advance(9); r.cancel(2);
    assert(r.beginDefuse(3, false)); r.advance(10);
    assert(r.phase == Phase::Resolved && r.winner == Team::Defenders);
    assert(r.score[1] == 1); r.setAlive(0,0); assert(r.score[1] == 1);
    r.resetRound(); r.advance(15); r.advance(114);
    assert(r.beginPlant(1,true)); r.advance(1);
    assert(r.phase == Phase::Resolved && r.winner == Team::Defenders);
    Rules tie; tie.advance(15); tie.beginPlant(1,true); tie.advance(3.2);
    tie.advance(30); tie.beginDefuse(2,false); tie.advance(10);
    assert(tie.reason == Reason::Exploded);
    Rules carrier; carrier.advance(15); assert(!carrier.beginPlant(2,false));
    carrier.beginPlant(1,true); carrier.cancel(1); carrier.advance(4);
    assert(carrier.phase == Phase::Live);
    Rules halves;
    for(int i=0;i<12;++i){ halves.advance(15); halves.setAlive(0,1); if(i<11) halves.resetRound(); }
    assert(!halves.matchOver && !halves.swapped && halves.score[1]==12);
    halves.resetRound(); assert(halves.swapped && halves.score[1]==12);
    halves.advance(15); halves.setAlive(1,0);
    assert(halves.matchOver && halves.score[1]==13);
    Rules draw;
    for(int i=0;i<24;++i){draw.advance(15);draw.setAlive(0,1);if(i<23)draw.resetRound();}
    assert(draw.matchOver && draw.score[0]==12 && draw.score[1]==12);
    Rules large; large.advance(1000); assert(large.phase==Phase::Resolved);
    Rules invalid;
    invalid.advance(-1); invalid.advance(NAN); invalid.advance(INFINITY);
    assert(invalid.phase==Phase::Freeze && invalid.remaining==15);
    assert(!invalid.beginPlant(1,true));
    invalid.advance(15); assert(invalid.beginPlant(1,true));
    assert(!invalid.beginPlant(2,true)); invalid.cancel(2);
    assert(invalid.actor==1); invalid.cancel(1); assert(invalid.actor==-1);
    Rules kit; kit.advance(15); kit.beginPlant(1,true); kit.advance(3.2);
    kit.beginDefuse(2,true); kit.advance(5);
    assert(kit.reason==Reason::Defused);
    std::cout << "Round rules: all scenarios passed\n";
}
