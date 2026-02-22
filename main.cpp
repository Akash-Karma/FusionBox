#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <optional>
#include <map>
#include <iostream>
#include <cmath>

const float SCALE = 30.f; 

struct PhysicsObject {
    b2BodyId bodyId;
    sf::CircleShape shape;
    int level;
    bool markedForDeletion = false;
};

sf::Color getLevelColor(int level) {
    static std::map<int, sf::Color> colors = {
        {1, sf::Color::Cyan}, {2, sf::Color::Magenta}, {3, sf::Color::Yellow}, 
        {4, sf::Color::Green}, {5, sf::Color::Red}, {6, sf::Color::Blue},
        {7, sf::Color(255, 165, 0)} 
    };
    return colors.count(level) ? colors[level] : sf::Color::White;
}

b2BodyId createCircleBody(b2WorldId worldId, float x, float y, float radius) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = { x, y };
    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
    b2Circle circle = { {0.0f, 0.0f}, radius / SCALE };
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableContactEvents = true; 
    b2CreateCircleShape(bodyId, &shapeDef, &circle);
    return bodyId;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "FusionBox - Optimized Edition");
    window.setFramerateLimit(60);
    sf::Clock gameClock;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 9.8f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    // --- STATIC CONTAINER ---
    b2ShapeDef wallDef = b2DefaultShapeDef();
    auto createWall = [&](float x, float y, float w, float h) {
        b2BodyDef bd = b2DefaultBodyDef();
        bd.position = {x / SCALE, y / SCALE};
        b2BodyId bid = b2CreateBody(worldId, &bd);
        b2Polygon box = b2MakeBox(w / SCALE, h / SCALE);
        b2CreatePolygonShape(bid, &wallDef, &box);
    };
    createWall(400, 590, 400, 10); // Floor
    createWall(5, 300, 5, 300);    // Left
    createWall(795, 300, 5, 300);  // Right

    sf::RectangleShape floorVisual({800.f, 20.f});
    floorVisual.setFillColor(sf::Color(80, 80, 80));
    floorVisual.setPosition({0.f, 580.f});

    // --- DASHED DROP LINE ---
    const int numDots = 15;
    std::vector<sf::CircleShape> dropDots;
    for (int i = 0; i < numDots; ++i) {
        sf::CircleShape dot(2.f);
        dot.setOrigin({2.f, 2.f});
        dropDots.push_back(dot);
    }

    sf::CircleShape previewCircle(20.f);
    previewCircle.setOrigin({20.f, 20.f});

    sf::RectangleShape dangerLine({800.f, 2.f});
    dangerLine.setFillColor(sf::Color(255, 0, 0, 180));
    dangerLine.setPosition({0.f, 120.f});

    std::vector<PhysicsObject> objects;
    int score = 0;
    bool isGameOver = false;
    float spawnTimer = 0.5f; // Initialized to allow first drop
    const float spawnDelay = 0.6f;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (!isGameOver && event->is<sf::Event::MouseButtonPressed>()) {
                if (spawnTimer >= spawnDelay) {
                    sf::Vector2i mPos = sf::Mouse::getPosition(window);
                    float radius = 20.f;
                    // Spawn slightly below the top to avoid instant wall collision
                    b2BodyId bId = createCircleBody(worldId, (float)mPos.x / SCALE, 80.f / SCALE, radius);
                    objects.push_back({bId, sf::CircleShape(radius), 1});
                    objects.back().shape.setOrigin({radius, radius});
                    objects.back().shape.setFillColor(getLevelColor(1));
                    spawnTimer = 0.f; 
                }
            }
        }

        if (!isGameOver) {
            spawnTimer += 1.0f / 60.0f;
            sf::Vector2i mPos = sf::Mouse::getPosition(window);
            
            // Visual feedback for cooldown
            float alphaBase = (spawnTimer >= spawnDelay) ? 150.f : 40.f;
            float pulse = (std::sin(gameClock.getElapsedTime().asSeconds() * 8.0f) + 1.0f) * 30.0f;
            
            for (int i = 0; i < numDots; ++i) {
                dropDots[i].setPosition({ (float)mPos.x, 140.f + (i * 30.f) });
                dropDots[i].setFillColor(sf::Color(255, 255, 255, (int)(alphaBase + pulse)));
            }

            previewCircle.setPosition({ (float)mPos.x, 80.f });
            previewCircle.setFillColor(sf::Color(0, 255, 255, (int)alphaBase));

            b2World_Step(worldId, 1.0f / 60.0f, 4);

            // Fusion Detection
            b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
            for (int i = 0; i < contactEvents.beginCount; ++i) {
                b2BodyId bodyA = b2Shape_GetBody(contactEvents.beginEvents[i].shapeIdA);
                b2BodyId bodyB = b2Shape_GetBody(contactEvents.beginEvents[i].shapeIdB);
                int idxA = -1, idxB = -1;
                for(int j=0; j < (int)objects.size(); ++j) {
                    if (objects[j].bodyId.index1 == bodyA.index1) idxA = j;
                    if (objects[j].bodyId.index1 == bodyB.index1) idxB = j;
                }
                if (idxA != -1 && idxB != -1 && idxA != idxB && objects[idxA].level == objects[idxB].level) {
                    if (!objects[idxA].markedForDeletion && !objects[idxB].markedForDeletion) {
                        objects[idxA].markedForDeletion = true;
                        objects[idxB].markedForDeletion = true;
                    }
                }
            }

            // Execute Fusion
            for (int i = 0; i < (int)objects.size(); ++i) {
                if (objects[i].markedForDeletion) {
                    for(int j = i + 1; j < (int)objects.size(); ++j) {
                        if(objects[j].markedForDeletion && objects[j].level == objects[i].level) {
                            b2Vec2 pA = b2Body_GetPosition(objects[i].bodyId);
                            b2Vec2 pB = b2Body_GetPosition(objects[j].bodyId);
                            int nextLvl = objects[i].level + 1;
                            float nextRad = 20.f + (nextLvl * 6.f);
                            
                            score += nextLvl * 10;
                            std::cout << "Score: " << score << std::endl;

                            b2BodyId nId = createCircleBody(worldId, (pA.x+pB.x)/2.f, (pA.y+pB.y)/2.f, nextRad);
                            b2DestroyBody(objects[i].bodyId);
                            b2DestroyBody(objects[j].bodyId);
                            objects.erase(objects.begin() + j);
                            objects.erase(objects.begin() + i);
                            
                            objects.push_back({nId, sf::CircleShape(nextRad), nextLvl});
                            objects.back().shape.setOrigin({nextRad, nextRad});
                            objects.back().shape.setFillColor(getLevelColor(nextLvl));
                            i--; break;
                        }
                    }
                }
            }

            // Position Sync & Strict Game Over
            for (auto& obj : objects) {
                b2Vec2 pos = b2Body_GetPosition(obj.bodyId);
                obj.shape.setPosition({pos.x * SCALE, pos.y * SCALE});
                
                b2Vec2 vel = b2Body_GetLinearVelocity(obj.bodyId);
                float speedSq = vel.x * vel.x + vel.y * vel.y;

                // Game over if stationary AND above line, but below the spawn point
                if (pos.y * SCALE < 120.f && pos.y * SCALE > 90.f && speedSq < 0.01f) {
                    isGameOver = true;
                }
            }
        }

        window.clear(sf::Color(20, 20, 20));
        
        window.draw(dangerLine);
        if (!isGameOver) {
            for (auto& dot : dropDots) window.draw(dot);
            window.draw(previewCircle);
        }
        
        window.draw(floorVisual);
        // Walls
        sf::RectangleShape wall({10.f, 600.f});
        wall.setFillColor(sf::Color(80, 80, 80));
        wall.setPosition({0.f, 0.f}); window.draw(wall);
        wall.setPosition({790.f, 0.f}); window.draw(wall);

        for (auto& obj : objects) window.draw(obj.shape);
        
        if (isGameOver) {
            // Darken the screen on game over
            sf::RectangleShape overlay({800.f, 600.f});
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);
        }

        window.display();
    }
    b2DestroyWorld(worldId);
    return 0;
}