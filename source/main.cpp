#include <SFML/Graphics.hpp>
#include <cstring>
#include <iostream>
#include <cmath>

struct Tile
{
    unsigned owner;
    unsigned previousOwner;
    
    unsigned isAnimating;
    double animationTime;
};

struct Map
{
    Tile* tiles;
    unsigned int* pixels;
    
    unsigned numberOfTiles;
    unsigned sizeOfTile;
};

struct MouseData
{
    unsigned isLeftPressed;
    unsigned isRightPressed;
    unsigned isLeftReleased;
    unsigned isRightReleased;
    
    unsigned posX;
    unsigned posY;
};

//TODO(Stanisz13): global for now
//NOTE(Stanisz13): in seconds
const double ANIMATION_TIME = 4.0d;

unsigned isMouseInWindow(const MouseData* mouseData, unsigned windowWidth, unsigned windowHeight)
{    
    if (mouseData->posX >= 0 && mouseData->posX < windowWidth)
    {
        if (mouseData->posY >= 0 && mouseData->posY < windowHeight)
        {
            return 1;
        }
    }

    return 0;
}

void runDFS(const unsigned xStart, const unsigned yStart, unsigned int* seen,
            const Map* map, unsigned int* fieldsVisited, unsigned int* enemyInside,
            unsigned curPlayer, unsigned runningToMark, int* dx, int* dy)
{
    unsigned int nTiles = map->numberOfTiles;
    
    seen[yStart * nTiles + xStart] = 1;

    unsigned curOwner = map->tiles[yStart * nTiles + xStart].owner;

    ++(*fieldsVisited);

    if (curOwner != 0 && curOwner != curPlayer)
    {
        ++(*enemyInside);

        if (runningToMark == 1)
        {
            map->tiles[yStart * nTiles + xStart].previousOwner =
                map->tiles[yStart * nTiles + xStart].owner; 
            map->tiles[yStart * nTiles + xStart].owner = curPlayer;
            map->tiles[yStart * nTiles + xStart].isAnimating = 1;
        }
    }

    for (unsigned int dir = 0; dir < 4; ++dir)
    {
        unsigned int xNext = xStart + dx[dir];
        unsigned int yNext = yStart + dy[dir];

        if (xNext >= 0 && xNext < nTiles)
        {
            if (yNext >= 0 && yNext < nTiles)
            {
                if (seen[yNext * nTiles + xNext] == 0)
                {
                    if (map->tiles[yNext * nTiles + xNext].owner !=
                        curPlayer)
                    {
                        runDFS(xNext, yNext, seen, map, fieldsVisited,
                               enemyInside, curPlayer, runningToMark, dx, dy);
                    }
                }
            }
        }
    }
}


void checkTakenTiles(const Map* map, const unsigned currentPlayer,
                     unsigned lastX, unsigned lastY, unsigned* seen)
{
    unsigned numTiles = map->numberOfTiles;

    for (unsigned i = 0; i < numTiles * numTiles; ++i)
    {
        seen[i] = 0;
    }
    
    int dx[4] = {1, -1, 0, 0};
    int dy[4] = {0, 0, 1, -1};

    unsigned minFieldsVisited = numTiles * numTiles + 2;
    unsigned minEnemiesVisited = minFieldsVisited;
    unsigned bestDir = 0;
    unsigned runningToMark = 0;
    unsigned timesDFSInvoked = 0;
    
    for (unsigned int dir = 0; dir < 4; ++dir)
    {
        unsigned int xNext = lastX + dx[dir];
        unsigned int yNext = lastY + dy[dir];
        
        if (xNext >= 0 && xNext < numTiles)
        {
            if (yNext >= 0 && yNext < numTiles)
            {
                unsigned curOwner = map->tiles[yNext * numTiles + xNext].owner; 
                unsigned curSeen = seen[yNext * numTiles + xNext];
                
                if (curOwner != currentPlayer && curSeen == 0)
                {
                    unsigned fieldsVisited = 0;
                    unsigned enemyInside = 0;

                    ++timesDFSInvoked;
                    
                    runDFS(xNext, yNext, seen, map, &fieldsVisited, &enemyInside,
                           currentPlayer, runningToMark, dx, dy);

                    if (fieldsVisited < minFieldsVisited)
                    {
                        minFieldsVisited = fieldsVisited;
                        minEnemiesVisited = enemyInside;
                        
                        bestDir = dir;
                    }
                    else if (fieldsVisited == minFieldsVisited)
                    {
                        if (enemyInside < minEnemiesVisited)
                        {
                            minEnemiesVisited = enemyInside;

                            bestDir = dir;
                        }
                    }
                }
            }
        }
    }

    runningToMark = 1;

    for (unsigned i = 0; i < numTiles * numTiles; ++i)
    {
        seen[i] = 0;
    }
    
    unsigned fields = 0;
    unsigned enemies = 0;

    if (timesDFSInvoked > 1 && minFieldsVisited > 1)
    {
        unsigned dir = bestDir;

        unsigned int xNext = lastX + dx[dir];
        unsigned int yNext = lastY + dy[dir];

        runDFS(xNext, yNext, seen, map, &fields, &enemies,
               currentPlayer, runningToMark, dx, dy);
    }

    return;
}

void updateAnimations(Map* map, const double dt)
{
    unsigned nTiles = map->numberOfTiles;
    
    for (unsigned i = 0; i < nTiles * nTiles; ++i)
    {
        Tile* currentTile = &map->tiles[i];

        if (currentTile->isAnimating == 1)
        {
            //TODO(Stanisz13): proper inequality checking
            if (currentTile->animationTime > ANIMATION_TIME)
            {
                currentTile->animationTime = 0.0d;
                currentTile->isAnimating = 0;
            }

            currentTile->animationTime += dt;
        }    
    }
}

template<typename T>
T lerp(const T a, const T b, const double t)
{
    return((1.0d - t) * a + t * b);
}

void checkGameEnded(const Map* map, unsigned* hasEnded, unsigned* winner, unsigned* isDraw)
{
    unsigned everybodyHasOwner = 1;
    unsigned bucket[8];
    memset(bucket, 0, 8 * sizeof(unsigned));
    
    for (unsigned i = 0; i < map->numberOfTiles * map->numberOfTiles; ++i)
    {
        if (map->tiles[i].owner == 0)
        {
            everybodyHasOwner = 0;
            break;
        }

        ++bucket[map->tiles[i].owner];
    }

    *hasEnded = everybodyHasOwner;
    *isDraw = 0;
    *winner = 0;
    
    if (*hasEnded == 1)
    {
        unsigned maxOwner = 0;
        unsigned maxPoints = 0;
        
        for (unsigned i = 1; i < 8; ++i)
        {
            if (bucket[i] > maxPoints)
            {
                maxPoints = bucket[i];
                maxOwner = i;
            }
        }

        for (unsigned i = 1; i < 8; ++i)
        {
            if (bucket[i] == maxPoints && maxOwner != i)
            {
                *isDraw = 1;
                break;
            }
        }

        if (*hasEnded == 1 && *isDraw == 0)
        {
            *winner = maxOwner;
        }
    }    
}

int main(int argc, char** argv)
{
    const unsigned int windowWidth = 1000, windowHeight = windowWidth;

    
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Strats");
    sf::Texture fromPixelBuffer;
    fromPixelBuffer.create(windowWidth, windowHeight);
    sf::Sprite fromTexturePixelBuffer(fromPixelBuffer);

    const unsigned int windowSize = sizeof(unsigned int) * windowHeight * windowWidth; 

    Map map;
    map.pixels = (unsigned int*)malloc(windowSize);
    
    map.numberOfTiles = 8;
    map.tiles = (Tile*)malloc(sizeof(Tile) * map.numberOfTiles * map.numberOfTiles);
    map.sizeOfTile = windowWidth / map.numberOfTiles;
    
    memset(map.tiles, 0, map.numberOfTiles * map.numberOfTiles * sizeof(Tile));

    MouseData mouseData = {};
    
    unsigned currentPlayer = 1;
    unsigned numberOfPlayers = 2;
    unsigned playerColors[8];
    //NOTE(Stanisz13): 0xAABBGGRR
    playerColors[0] = 0xff111111;
    playerColors[1] = 0xff0000ff;
    playerColors[2] = 0xff00ff00;
    playerColors[3] = 0xffff0000;
    playerColors[4] = 0xff00ffff;
    playerColors[5] = 0xff8823aa;
    playerColors[6] = 0xff550044;
    playerColors[7] = 0xff336633;

    
    unsigned int* dfsSeen = (unsigned int*)malloc(
        map.numberOfTiles * map.numberOfTiles * sizeof(unsigned));
    
    
    sf::Clock clock;
    
    while (window.isOpen())
    {
        double dt = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    mouseData.isRightPressed = 1;
                }

                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    mouseData.isLeftPressed = 1;
                }
            }

            if (event.type == sf::Event::MouseButtonReleased)
            {   
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    mouseData.isRightPressed = 0;
                    mouseData.isRightReleased = 1;
                }

                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    mouseData.isLeftPressed = 0;
                    mouseData.isLeftReleased = 1;
                }
            }

        }


        mouseData.posX = sf::Mouse::getPosition(window).x;
        mouseData.posY = sf::Mouse::getPosition(window).y;
        
        
        if (mouseData.isLeftReleased)
        {
            Tile changed = {};
            changed.owner = currentPlayer;
            
            unsigned X = mouseData.posX / map.sizeOfTile;
            unsigned Y = mouseData.posY / map.sizeOfTile;

            Tile* hit = map.tiles + Y * map.numberOfTiles + X; 

            if(hit->owner == 0)
            {
                *hit = changed;

                checkTakenTiles(&map, currentPlayer, X, Y, dfsSeen);
                
                if (currentPlayer < numberOfPlayers)
                {
                    ++currentPlayer;
                }
                else
                {
                    currentPlayer = 1;
                }
            }            

            mouseData.isLeftReleased = 0;
            mouseData.isRightReleased = 0;
        }

        unsigned hasGameEnded;
        unsigned winner;
        unsigned isDraw;
        checkGameEnded(&map, &hasGameEnded, &winner, &isDraw);
        
        if (hasGameEnded == 1)
        {
            for (unsigned i = 0; i < map.numberOfTiles * map.numberOfTiles; ++i)
            {
                Tile* currentTile = &map.tiles[i];

                if (currentTile->owner != winner)
                {
                    currentTile->isAnimating = 1;
                    currentTile->previousOwner = currentTile->owner;
                    currentTile->owner = winner;
                }
            }
        }

        updateAnimations(&map, dt);
        
        for (unsigned y = 0; y < windowHeight; ++y)
        {
            for (unsigned x = 0; x < windowWidth; ++x)
            {
                unsigned X = x / map.sizeOfTile;
                unsigned Y = y / map.sizeOfTile;

                Tile* currentTile = map.tiles + Y * map.numberOfTiles + X;
                unsigned int* currentColor = map.pixels + y * windowWidth + x;

                if (currentTile->isAnimating == 0)
                {
                    *currentColor = playerColors[currentTile->owner];
                }    
                else
                {
                    unsigned color1 = playerColors[currentTile->previousOwner] & 0x00ffffff;
                    unsigned color2 = playerColors[currentTile->owner] & 0x00ffffff;

                    unsigned redMask = 0xff0000ff;
                    unsigned greenMask = 0xff00ff00;
                    unsigned blueMask = 0xffff0000;

                    unsigned sizeOfOneChannel = 8;
                    
                    const double t = currentTile->animationTime / ANIMATION_TIME;
                
                    unsigned char red = lerp<unsigned char>((unsigned char)(color1 & redMask),
                                                            (unsigned char)(color2 & redMask), t);

                    unsigned rawGreenChannel1 = (color1 & greenMask) >> sizeOfOneChannel;
                    unsigned rawGreenChannel2 = (color2 & greenMask) >> sizeOfOneChannel;
                        
                    unsigned char green = lerp<unsigned char>((unsigned char)(rawGreenChannel1),
                                                              (unsigned char)(rawGreenChannel2), t);

                    unsigned rawBlueChannel1 = (color1 & blueMask) >> (sizeOfOneChannel * 2);
                    unsigned rawBlueChannel2 = (color2 & blueMask) >> (sizeOfOneChannel * 2);

                    unsigned char blue = lerp<unsigned char>((unsigned char)(rawBlueChannel1),
                                                             (unsigned char)(rawBlueChannel2), t);
                
                    *currentColor = 0xff000000 | (blue << sizeOfOneChannel * 2) |
                        (green << sizeOfOneChannel) | red;
                }
            }
        }
        
        fromPixelBuffer.update((unsigned char*)map.pixels);
        window.draw(fromTexturePixelBuffer);
        
        window.display();
    }

    free(map.pixels);
    free(map.tiles);
    free(dfsSeen);
    
    return 0;
}
