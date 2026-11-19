#pragma once
#include <algorithm>
namespace cs6 {
inline float eyeHeight(float halfHeight,float standingHalfHeight) {
    const float standing=std::max(1.f,standingHalfHeight);
    const float standFraction=std::clamp((halfHeight/standing-0.625f)/0.375f,0.f,1.f);
    return 105.f+55.f*standFraction;
}
}
