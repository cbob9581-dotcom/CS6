#include "CS6TacticalMatch.h"
#include "CS6TemplateAccess.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
using namespace CS6TemplateAccess;

int32 UCS6RoundOverlay::NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Draw,int32 L,const FWidgetStyle& Style,bool Enabled)const {
    L=Super::NativePaint(Args,G,Clip,Draw,L,Style,Enabled);if(!IsValid(Match))return L;
    ShopHits.Empty();const float W=G.GetLocalSize().X,H=G.GetLocalSize().Y;
    const FLinearColor Dark(0.018,0.025,0.03,0.94),Gold(0.94,0.75,0.35),Blue(0.45,0.75,0.98),Muted(0.65,0.7,0.73),White(0.94,0.96,0.98);
    auto Box=[&](float X,float Y,float A,float B,FLinearColor C){FSlateDrawElement::MakeBox(Draw,++L,G.ToPaintGeometry(FVector2D(A,B),FSlateLayoutTransform(FVector2D(X,Y))),FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,C);};
    auto Text=[&](float X,float Y,const FString& T,int32 Size,FLinearColor C){FSlateDrawElement::MakeText(Draw,++L,G.ToPaintGeometry(FVector2D(1,1),FSlateLayoutTransform(FVector2D(X,Y))),T,FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),Size),ESlateDrawEffect::None,C);};
    auto* PC=Match->GetWorld()->GetFirstPlayerController();auto* P=PC?PC->GetPawn():nullptr;
    const bool Alive=P && P->IsA(Match->TemplatePawn) && !Bool(P,TEXT("Dead"));
    const bool IsT=(PC && Number(PC->PlayerState,TEXT("Team"))==0)!=Match->bSidesSwapped;
    const auto* Gear=Match->LocalGear();
    int32 Ts=0,CTs=0;for(const auto& C:Match->Combatants)if(C.bAlive && IsValid(C.Pawn))C.bAttacker?++Ts:++CTs;
    const int32 CTScore=Match->bSidesSwapped?Match->TeamOneScore:Match->TeamTwoScore,TScore=Match->bSidesSwapped?Match->TeamTwoScore:Match->TeamOneScore;
    Box(W/2-270,12,540,80,Dark);Text(W/2-250,22,FString::Printf(TEXT("CT  %d"),CTScore),26,Blue);Text(W/2+155,22,FString::Printf(TEXT("%d  T"),TScore),26,Gold);
    const int32 Seconds=FMath::Max(0,FMath::CeilToInt(Match->SecondsRemaining));
    Text(W/2-38,18,FString::Printf(TEXT("%02d:%02d"),Seconds/60,Seconds%60),28,White);
    const FString Phase=Match->bMatchOver?TEXT("MATCH COMPLETE"):Match->Phase==ECS6Phase::Freeze?TEXT("BUY / FREEZE"):Match->Phase==ECS6Phase::Planted?TEXT("BOMB PLANTED"):Match->Phase==ECS6Phase::Resolved?TEXT("ROUND OVER"):TEXT("LIVE");
    Text(W/2-95,61,Phase,14,Match->Phase==ECS6Phase::Planted?FLinearColor(1,0.35,0.25):Muted);
    Text(W/2-245,61,FString::Printf(TEXT("%d ALIVE"),CTs),14,Blue);Text(W/2+167,61,FString::Printf(TEXT("%d ALIVE"),Ts),14,Gold);
    Text(26,25,FString::Printf(TEXT("MILITARY BASE  |  ROUND %d"),FMath::Min(24,Match->TeamOneScore+Match->TeamTwoScore+1)),15,White);
    if(Match->bEconomyEnabled)Text(26,55,FString::Printf(TEXT("$%d"),Match->LocalCash()),28,FLinearColor(0.64,0.88,0.5));
    if(Alive) {
        const int32 HP=FMath::Max(0,FMath::CeilToInt(Scalar(Object(P,TEXT("C_Health")),TEXT("Health"))));
        Box(20,H-105,350,85,Dark);Text(38,H-96,FString::Printf(TEXT("%d"),HP),34,White);Text(120,H-87,TEXT("HP"),15,Muted);
        Text(190,H-87,FString::Printf(TEXT("%d  ARMOR"),Gear?Gear->armor:0),16,Blue);
        FString Role=IsT?TEXT("TERRORIST"):TEXT("COUNTER-TERRORIST");if(Match->BombCarrier==P)Role+=TEXT("  |  C4");if(Gear && Gear->kit)Role+=TEXT("  |  DEFUSE KIT");
        Text(38,H-48,Role,13,IsT?Gold:Blue);
        auto* Gun=Object(P,TEXT("CurrentWeapon"));
        if(Match->bBombSelected){Box(W-350,H-105,330,85,Dark);Text(W-325,H-92,TEXT("C4 EXPLOSIVE"),25,Gold);Text(W-325,H-48,TEXT("HOLD LMB IN BOMB SITE"),15,Muted);}
        else if(Gun) {
            Box(W-350,H-105,330,85,Dark);Text(W-325,H-98,FString::Printf(TEXT("%lld  /  %d"),Number(Gun,TEXT("CurrentAmmo")),FMath::Max(0,ReserveAmmo(P,Gun))),34,White);
            const auto* M=Match->Member(P);const auto Item=M?(Gun==Object(P,TEXT("Primary Weapon"))?M->Wallet.gear.primary:M->Wallet.gear.pistol):cs6::Item::None;
            const auto* D=Match->Offer(Item);Text(W-325,H-48,D?D->DisplayName.ToString():TEXT("1 PRIMARY   2 SIDEARM"),15,Muted);
        }
        if(!Bool(P,TEXT("IsAiming")) && !Bool(P,TEXT("Aiming")) && !Match->bBuyMenuOpen){const FLinearColor C(0.72,1,0.7);Box(W/2-12,H/2-1,7,2,C);Box(W/2+5,H/2-1,7,2,C);Box(W/2-1,H/2-12,2,7,C);Box(W/2-1,H/2+5,2,7,C);}
        const FString Hint=Match->bEconomyEnabled?TEXT("B BUY   5 C4 / LMB PLANT   E DEFUSE / PICK UP   G DROP   TAB SCORE"):TEXT("F  HOLD TO PLANT / DEFUSE    TAB  SCOREBOARD");
        Text(26,H-140,Hint,13,Muted);
    } else {
        Box(W/2-350,H-150,700,120,Dark);Text(W/2-320,H-135,TEXT("ELIMINATED  |  SPECTATING"),22,White);
        if(auto* Target=Match->ObservedPawn.Get()) {
            const auto* Member=Match->Member(Target);auto* Gun=Object(Target,TEXT("CurrentWeapon"));
            Text(W/2-320,H-100,FString::Printf(TEXT("BOT %02d   |   %d HP   |   %lld / %d AMMO"),Member?int32(Member-Match->Roster.GetData())+1:0,FMath::Max(0,int32(Scalar(Object(Target,TEXT("C_Health")),TEXT("Health")))),Number(Gun,TEXT("CurrentAmmo")),ReserveAmmo(Target,Gun)),16,Blue);
        }
        Text(W/2-320,H-64,TEXT("MOUSE  SWITCH   |   SPACE  VIEW   |   E  TAKE OVER BOT"),14,Muted);
    }
    const float Progress=Match->InteractionProgress();if(Progress>0){Box(W/2-160,H*0.64,320,10,Dark);Box(W/2-160,H*0.64,320*Progress,10,Gold);}
    int32 FeedRow=0;for(int32 I=Match->Notices.Num()-1;I>=0;--I){const auto& N=Match->Notices[I];if(Match->Age-N.Time>7)continue;const float Y=110+FeedRow++*38;Box(W-510,Y,490,32,Dark);Text(W-495,Y+5,N.Killer+TEXT("   ")+N.Weapon+TEXT("   ")+N.Victim,14,N.bFriendly?FLinearColor(1,0.4,0.35):White);}
    if(Match->Phase==ECS6Phase::Resolved && !Match->bBuyMenuOpen) {
        Box(W/2-345,H*0.35,690,100,Dark);Text(W/2-315,H*0.35+16,Match->Status,21,White);
        Text(W/2-315,H*0.35+58,Match->bMatchOver?TEXT("R  START A NEW MATCH"):TEXT("NEXT ROUND STARTING"),14,Muted);
    }
    if(PC && PC->IsInputKeyDown(EKeys::Tab) && !Match->bBuyMenuOpen && !Match->bPauseMenuOpen) {
        const float X=W*0.16,Y=H*0.17,A=W*0.68;Box(X,Y,A,H*0.64,Dark);Text(X+25,Y+20,TEXT("COMPETITIVE  |  MILITARY BASE"),24,White);
        Text(X+A-340,Y+30,TEXT("MONEY     K     A     D    DAMAGE"),14,Muted);
        for(int32 I=0;I<Match->Roster.Num();++I){const auto& M=Match->Roster[I];const float RY=Y+84+I*42;const bool T=M.bTeamA!=Match->bSidesSwapped;Box(X+15,RY-2,A-30,37,FLinearColor(0.08,0.1,0.12,0.85));Text(X+28,RY,T?TEXT("T"):TEXT("CT"),15,T?Gold:Blue);Text(X+85,RY,M.Controller.IsValid() && M.Controller->IsPlayerController()?TEXT("YOU"):FString::Printf(TEXT("BOT %02d"),I),16,White);Text(X+A-340,RY,FString::Printf(TEXT("$%d    %d    %d    %d    %d"),M.Wallet.cash,M.Kills,M.Assists,M.Deaths,M.Damage),15,White);}
    }
    if(Match->bBuyMenuOpen) {
        Box(0,0,W,H,FLinearColor(0.005,0.01,0.015,0.88));const float X=W*0.055,Y=H*0.16,A=W*0.89,Col=A/5,CardH=(H*0.62)/6;
        Text(X,Y-65,TEXT("BUY MENU"),34,White);Text(X+260,Y-57,FString::Printf(TEXT("$%d"),Match->LocalCash()),26,FLinearColor(0.64,0.88,0.5));Text(X+A-390,Y-52,TEXT("RIGHT CLICK  REFUND   |   B  CLOSE"),13,Muted);
        const TCHAR* Titles[]={TEXT("EQUIPMENT"),TEXT("PISTOLS"),TEXT("MID-TIER"),TEXT("RIFLES"),TEXT("GRENADES")};
        for(int32 C=0;C<5;++C){Box(X+C*Col,Y,Col-8,H*0.67,Dark);Text(X+C*Col+16,Y+14,Titles[C],16,Gold);}
        int32 Rows[5]={0,0,0,0,0};
        for(int32 I=1;I<int32(cs6::Item::Count);++I) {
            const auto Item=static_cast<cs6::Item>(I);const auto& Product=cs6::product(Item);const auto* D=Match->Offer(Item);
            if(!(Product.sides&(IsT?1:2)))continue;
            if((Product.slot==cs6::Slot::Primary || Product.slot==cs6::Slot::Pistol) && !D)continue;
            if(Product.slot==cs6::Slot::Grenade)continue;
            int32 C=Product.slot==cs6::Slot::Pistol?1:Product.slot==cs6::Slot::Primary?(I>=int32(cs6::Item::Galil) && I<=int32(cs6::Item::Scout)?3:2):0;
            if(Rows[C]>=6)continue;
            const float BX=X+C*Col+9,BY=Y+52+Rows[C]++*CardH;
            const auto* M=Match->Member(P);const int32 Price=M?M->Wallet.price(Item):Product.price;const bool Affordable=Match->LocalCash()>=Price;
            Box(BX,BY,Col-26,CardH-8,FLinearColor(0.085,0.105,0.12,0.94));Text(BX+12,BY+11,D?D->DisplayName.ToString():UTF8_TO_TCHAR(Product.name),15,Affordable?White:Muted);Text(BX+12,BY+40,FString::Printf(TEXT("$%d"),Price),17,Affordable?Gold:Muted);
            ShopHits.Add({FSlateRect(BX,BY,BX+Col-26,BY+CardH-8),I,false});
        }
        Text(X,Y+H*0.69,Match->BuyMessage.IsEmpty()?TEXT("PURCHASE IN YOUR BUY ZONE. UNUSED PURCHASES CAN BE REFUNDED THIS ROUND."):Match->BuyMessage,15,Gold);
    }
    if(Match->bPauseMenuOpen) {
        Box(0,0,W,H,FLinearColor(0.005,0.01,0.015,0.85));const float X=W/2-230,Y=H/2-205;Box(X,Y,460,410,Dark);Text(X+35,Y+25,TEXT("PAUSED"),30,White);
        const TCHAR* Labels[]={TEXT("RESUME"),TEXT("GRAPHICS: LOW"),TEXT("GRAPHICS: HIGH"),TEXT("QUIT GAME")};
        for(int32 I=0;I<4;++I){const float BY=Y+100+I*65;Box(X+30,BY,400,52,FLinearColor(0.08,0.1,0.12));Text(X+50,BY+12,Labels[I],18,I==3?Muted:White);ShopHits.Add({FSlateRect(X+30,BY,X+430,BY+52),-I-1,false});}
    }
    return L;
}
