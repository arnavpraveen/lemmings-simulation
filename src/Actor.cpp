#include "Actor.h"
#include "StudentWorld.h"

// Students:  Add code to this file, Actor.h, StudentWorld.h, and StudentWorld.cpp

// initialize with image, position, direction, and world pointer
Actor::Actor(int imageID, int startX, int startY, int dir, StudentWorld* world)
 : GraphObject(imageID, Coord(startX, startY), dir), m_world(world), m_alive(true)
{}

// by default actors are not solid
bool Actor::isSolid() const {
    return false;
}

StudentWorld* Actor::getWorld() const{
    return m_world;
}

FloorBrick::FloorBrick(int startX, int startY, StudentWorld* world) : Actor(IID_FLOOR, startX, startY, GraphObject::none, world)
{}

// bricks do nothing each tick
void FloorBrick::doSomething() {}

// starts at 9 so it moves on the very first tick
IceMonster::IceMonster(int startX, int startY, StudentWorld* world) : Actor(IID_ICE_MONSTER, startX, startY, GraphObject::right, world), m_tickCounter(9)
{}


void IceMonster::doSomething() {
   
    if (getWorld()->killLemmingsAt(getCoord().x, getCoord().y)) {
        return;
    }
    
    // only move every 10 ticks
    m_tickCounter++;
    if (m_tickCounter < 10) {
        return;
    }
    m_tickCounter = 0;
    
    int curX = getCoord().x;
    int curY = getCoord().y;
    int dir = getDirection();
    
    int nextX = (dir == GraphObject::left) ? curX - 1 : curX + 1;
    
    // reverse if next square is a wall or has no floor beneath it
    if (getWorld()->isSolid(nextX, curY) || !getWorld()->isSolid(nextX, curY - 1)) {
        setDirection(dir == GraphObject::left ? GraphObject::right : GraphObject::left);
    } else {
        moveTo(Coord(nextX, curY));
    }
}

// starts walking right, with tick counter at 2
Lemming::Lemming(int startX, int startY, StudentWorld* world) : Actor(IID_LEMMING, startX, startY, GraphObject::right, world), m_state(WALKING), m_tickCounter(2), m_fallDistance(0), m_bounceTarget(0), m_bounceCurrent(0){}

// play death sound and notify world
void Lemming::die() {
    if (!isAlive()) return;
    Actor::die();
    getWorld()->playSound(SOUND_LEMMING_DIE);
    getWorld()->increaseDeadLemmings();
}

// play saved sound and notify world
void Lemming::getSaved() {
    if (!isAlive()) return;
    Actor::die();
    getWorld()->playSound(SOUND_LEMMING_SAVED);
    getWorld()->increaseSavedLemmings();
}

// initiate a bounce, returns false if already bouncing
bool Lemming::startBouncing(int targetDistance) {
    if (m_state == BOUNCING) return false;
    m_state = BOUNCING;
    m_bounceTarget = targetDistance;
    m_bounceCurrent = 0;
    return true;
}

void Lemming::doSomething() {
    
    if (!isAlive()) return;
    // walking lemmings move every 4 ticks, all other states every 2 ticks
    int requiredTicks = (m_state == WALKING) ? 4 : 2;
    m_tickCounter++;
    
    if (m_tickCounter < requiredTicks) {
        return;
    }
    m_tickCounter = 0;
    // check for pheromone attraction before moving
    processPheromones();

    // execute current movement state
    switch (m_state) {
        case WALKING:  doWalking();  break;
        case FALLING:  doFalling();  break;
        case CLIMBING: doClimbing(); break;
        case BOUNCING: doBouncing(); break;
    }
}

// turn toward the closest pheromone within 5 squares on the same row
void Lemming::processPheromones() {
    int targetDir = GraphObject::none;
    if (getWorld()->getClosestPheromoneDirection(getCoord().x, getCoord().y, targetDir)) {
        setDirection(targetDir);
    }
}

void Lemming::doWalking() {
    int curX = getCoord().x;
    int curY = getCoord().y;
    int dir = getDirection();

    // if standing on a net, start climbing
    if (getWorld()->hasClimbable(curX, curY)) {
        m_state = CLIMBING;
        return;
    }

    int nextX = (dir == GraphObject::left) ? (curX - 1) : (curX + 1);
    if (getWorld()->isSolid(nextX, curY)) {
        setDirection(dir == GraphObject::left ? GraphObject::right : GraphObject::left);
        return;
    }

    if (getWorld()->isSolid(nextX, curY - 1)) {
        moveTo(Coord(nextX, curY));
    } else {
        m_state = FALLING;
        m_fallDistance = 0;
        moveTo(Coord(nextX, curY));
    }
}

void Lemming::doFalling() {
    int curX = getCoord().x;
    int curY = getCoord().y;

    // if falling onto a net, switch to climbing
    if (getWorld()->hasClimbable(curX, curY)) {
        m_state = CLIMBING;
        return;
    }

    int belowY = curY - 1;
    if (!getWorld()->isValidPos(curX, belowY) || getWorld()->isSolid(curX, belowY)) {
        // Landed
        if (m_fallDistance > 5) {
            die();
        } else {
            m_state = WALKING;
            m_fallDistance = 0;
        }
        return;
    }
    
    // continue falling
    m_fallDistance++;
    moveTo(Coord(curX, belowY));
}

void Lemming::doClimbing() {
    int curX = getCoord().x;
    int curY = getCoord().y;

    // no longer on a net
    if (!getWorld()->hasClimbable(curX, curY)) {
        m_state = WALKING;
        return;
    }

    // try to move up one square
    int aboveY = curY + 1;
    if (!getWorld()->isValidPos(curX, aboveY) || getWorld()->isSolid(curX, aboveY)) {
        return;
    }

    moveTo(Coord(curX, aboveY));
}

void Lemming::doBouncing() {
    int curX = getCoord().x;
    int curY = getCoord().y;

    // if bouncing onto a net, switch to climbing
    if (getWorld()->hasClimbable(curX, curY)) {
        m_state = CLIMBING;
        return;
    }

    // straight-up phase: go up one square per tick until (bounceTarget - 1) reached
    if (m_bounceTarget > 0 && m_bounceCurrent < m_bounceTarget - 1) {
        int aboveY = curY + 1;
        if (getWorld()->isValidPos(curX, aboveY) && !getWorld()->isSolid(curX, aboveY)) {
        moveTo(Coord(curX, aboveY));
            m_bounceCurrent++;
            return;
        }
        // blocked above — step forward horizontally, then fall
        int dir = getDirection();
        int nextX = curX + (dir == GraphObject::left ? -1 : 1);
        if (getWorld()->isValidPos(nextX, curY) && !getWorld()->isSolid(nextX, curY)) {
            moveTo(Coord(nextX, curY));
        }
        m_state = FALLING;
        m_fallDistance = 0;
        return;
    }

    // apex: move diagonally (up + forward) if height > 0, else just forward
    int dir = getDirection();
    int nextX = curX + (dir == GraphObject::left ? -1 : 1);

    if (m_bounceTarget > 0) {
        int nextY = curY + 1;
        if (getWorld()->isValidPos(nextX, nextY) && !getWorld()->isSolid(nextX, nextY)) {
            moveTo(Coord(nextX, nextY));
        } else if (getWorld()->isValidPos(curX, nextY) && !getWorld()->isSolid(curX, nextY)) {
            moveTo(Coord(curX, nextY));
            setDirection(dir == GraphObject::left ? GraphObject::right : GraphObject::left);
        } else {
            setDirection(dir == GraphObject::left ? GraphObject::right : GraphObject::left);
        }
    } else {
        // bounce height 0: just move forward horizontally
        if (!getWorld()->isValidPos(nextX, curY) || getWorld()->isSolid(nextX, curY)) {
            setDirection(dir == GraphObject::left ? GraphObject::right : GraphObject::left);
        } else {
            moveTo(Coord(nextX, curY));
        }
    }

    m_state = FALLING;
    m_fallDistance = 0;
}

Bonfire::Bonfire(int startX, int startY, StudentWorld* world) : Actor(IID_BONFIRE, startX, startY, GraphObject::right, world)
{}

// kill any lemmings sharing this square
void Bonfire::doSomething() {
    getWorld()->killLemmingsAt(getCoord().x, getCoord().y);
}

// starts at center of board facing right
Player::Player(StudentWorld* world)
 : Actor(IID_PLAYER, VIEW_WIDTH / 2, VIEW_HEIGHT / 2, GraphObject::right, world)
{}

void Player::doSomething() {
    int ch;
    if (getWorld()->getKey(ch)) {
        int x = getCoord().x;
        int y = getCoord().y;

        switch (ch) {
            case KEY_PRESS_LEFT:
                if (x > 1) moveTo(Coord(x - 1, y));
                break;
            case KEY_PRESS_RIGHT:
                if (x < VIEW_WIDTH - 2) moveTo(Coord(x + 1, y));
                break;
            case KEY_PRESS_DOWN:
                if (y > 1) moveTo(Coord(x, y - 1));
                break;
            case KEY_PRESS_UP:
                if (y < VIEW_HEIGHT - 2) moveTo(Coord(x, y + 1));
                break;
                
            // Tool placement keys
            case 't': case 'T':
            case 'n': case 'N':
            case 'p': case 'P':
            case 's': case 'S':
            case '<': case '>':
                getWorld()->tryPlaceTool(x, y, ch);
                break;
        }
    }
}

Trampoline::Trampoline(int startX, int startY, StudentWorld* world)
 : Actor(IID_TRAMPOLINE, startX, startY, GraphObject::right, world)
{
}
// bounce any non-bouncing lemming on this square
void Trampoline::doSomething() {
    getWorld()->bounceLemmingAt(getCoord().x, getCoord().y);
}

Net::Net(int startX, int startY, StudentWorld* world)
 : Actor(IID_NET, startX, startY, GraphObject::right, world)
{}

// climbing handled by lemmings
void Net::doSomething() {}

OneWayDoor::OneWayDoor(int startX, int startY, int dir, StudentWorld* world)
 : Actor(IID_ONE_WAY_DOOR, startX, startY, dir, world)
{
}

// force all actors on this square to face the door's direction
void OneWayDoor::doSomething() {
    getWorld()->setActorsDirectionAt(getCoord().x, getCoord().y, getDirection());
}

LemmingFactory::LemmingFactory(int startX, int startY, StudentWorld* world)
 : Actor(IID_LEMMING_FACTORY, startX, startY, GraphObject::right, world),
   m_tickCount(0), m_spawnCount(0)
{
}

// spawn one lemming every 100 ticks, up to 10 total
void LemmingFactory::doSomething() {
    if (m_spawnCount >= 10) {
        return;
    }
    
    m_tickCount++;
    
    if (m_tickCount >= 100) {
        getWorld()->spawnLemmingAt(getCoord().x, getCoord().y);
        
        m_tickCount = 0;
        m_spawnCount++;
    }
}

Pheromone::Pheromone(int startX, int startY, StudentWorld* world)
 : Actor(IID_PHEROMONE, startX, startY, GraphObject::right, world)
{
}

// attraction handled by lemmings
void Pheromone::doSomething() {}

Spring::Spring(int startX, int startY, StudentWorld* world)
 : Actor(IID_SPRING, startX, startY, GraphObject::right, world)
{
    setVisible(true);
}

// launch any non-bouncing lemming on this square with bounce height 15
void Spring::doSomething() {
    getWorld()->springLemmingAt(getCoord().x, getCoord().y);
}

Exit::Exit(int startX, int startY, StudentWorld* world)
 : Actor(IID_EXIT, startX, startY, GraphObject::right, world)
{
}

// save one lemming per tick if any are on this square
void Exit::doSomething() {
    getWorld()->saveOneLemmingAt(getCoord().x, getCoord().y);
}
