#pragma once
#include <algorithm>
#include <cmath>

namespace cs6 {
enum class Phase { Freeze, Live, Planted, Resolved };
enum class Team { Attackers, Defenders };
enum class Reason { None, Eliminated, Timeout, Defused, Exploded };
struct Rules {
    Phase phase = Phase::Freeze;
    Team winner = Team::Attackers;
    Reason reason = Reason::None;
    double remaining = 15, interaction = 0;
    int actor = -1, score[2] = {0,0}, completed = 0;
    bool swapped = false, matchOver = false;
    int teamIndex(Team t) const { return static_cast<int>(t) ^ (swapped ? 1 : 0); }
    void resolve(Team t, Reason why) {
        if (phase == Phase::Resolved || matchOver) return;
        phase = Phase::Resolved; winner = t; reason = why;
        actor = -1; interaction = 0;
        ++score[teamIndex(t)]; ++completed;
        // Offline Competitive: MR12, first to 13; regulation ties finish 12:12.
        matchOver = score[teamIndex(t)] >= 13 || completed >= 24;
    }
    void resetRound() {
        if (phase != Phase::Resolved || matchOver) return;
        swapped = completed >= 12;
        phase = Phase::Freeze; remaining = 15;
        actor = -1; interaction = 0; reason = Reason::None;
    }
    bool beginPlant(int id, bool hasBomb) {
        if (phase != Phase::Live || !hasBomb || id < 0 || actor != -1) return false;
        actor = id; interaction = 3.2; return true;
    }
    bool beginDefuse(int id, bool kit) {
        if (phase != Phase::Planted || id < 0 || actor != -1) return false;
        actor = id; interaction = kit ? 5 : 10; return true;
    }
    void cancel(int id) { if (actor == id) { actor = -1; interaction = 0; } }
    void setAlive(int attackers, int defenders) {
        if (phase != Phase::Live && phase != Phase::Planted) return;
        if (defenders == 0) resolve(Team::Attackers, Reason::Eliminated);
        else if (attackers == 0 && phase == Phase::Live) resolve(Team::Defenders, Reason::Eliminated);
    }
    void advance(double dt) {
        if (!std::isfinite(dt) || dt <= 0) return;
        while (dt > 0 && phase != Phase::Resolved) {
            double step = std::min(dt, remaining);
            if (actor != -1) step = std::min(step, interaction);
            remaining -= step; dt -= step;
            if (actor != -1) interaction -= step;
            // Deadline wins ties with interaction completion.
            if (remaining <= 1e-8) {
                if (phase == Phase::Freeze) { phase = Phase::Live; remaining = 115; }
                else if (phase == Phase::Live) resolve(Team::Defenders, Reason::Timeout);
                else resolve(Team::Attackers, Reason::Exploded);
            } else if (actor != -1 && interaction <= 1e-8) {
                actor = -1;
                if (phase == Phase::Live) { phase = Phase::Planted; remaining = 40; }
                else resolve(Team::Defenders, Reason::Defused);
            }
        }
    }
};
}
