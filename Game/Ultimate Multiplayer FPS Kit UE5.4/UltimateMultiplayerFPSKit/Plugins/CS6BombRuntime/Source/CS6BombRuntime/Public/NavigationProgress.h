#pragma once
namespace cs6 {
struct AreaProgress {
    double since=-1;
    bool update(double now,double distance,bool wantsMove) {
        if(!wantsMove || since<0 || now<since || distance>200){since=now;return false;}
        if(now-since>6){since=now;return true;}return false;
    }
};
enum class NavAction {Hold,KeepPath,RequestPath,Recover};
struct NavigationProgress {
    double lastProgress=0,nextRequest=0,lastSample=-1;
    int recoveries=0;
    NavAction update(double now,bool moved,bool wantsMove,bool hasPath,bool newGoal) {
        if(now<lastSample){lastProgress=now;nextRequest=now;newGoal=true;}
        lastSample=now;
        if(!wantsMove){lastProgress=now;return NavAction::Hold;}
        if(newGoal){lastProgress=now;nextRequest=now+0.8;return NavAction::RequestPath;}
        if(moved){lastProgress=now;recoveries=0;}
        if(now-lastProgress>2.0){lastProgress=now;nextRequest=now+0.8;++recoveries;return NavAction::Recover;}
        if(!hasPath && now>=nextRequest){nextRequest=now+0.8;return NavAction::RequestPath;}
        return NavAction::KeepPath;
    }
};
}
