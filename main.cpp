#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <optional>
#include <map>
#include <iostream>
#include <fstream>
#include <cmath>
#include <random>
#include <filesystem>

const float SCALE = 30.f;

struct PhysicsObject {
    b2BodyId bodyId;
    sf::Sprite sprite;
    int level;
    bool markedForDeletion = false;
};

int highScore = 0;
std::map<int, sf::Texture> celestialTextures;

void loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file >> highScore) file.close();
}

void saveHighScore(int s) {
    if (s > highScore) {
        highScore = s;
        std::ofstream file("highscore.txt");
        file << highScore;
    }
}

void loadAssets() {
    for (int i = 1; i <= 8; ++i) {
        sf::Texture tex;
        std::string path = "assets/" + std::to_string(i) + ".png";
        if (tex.loadFromFile(path)) {
            tex.setSmooth(true);
            celestialTextures[i] = std::move(tex);
        }
    }
}

b2BodyId createCelestialBody(b2WorldId worldId, float x, float y, float radius) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = { x, y };
    bodyDef.fixedRotation = false; // Enable spinning
    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
    
    b2Circle circle = { {0.0f, 0.0f}, radius / SCALE };
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    
    // Physical properties for rolling and bouncing
    shapeDef.material.restitution = 0.2f; 
    shapeDef.material.friction = 0.5f;
    shapeDef.density = 1.0f + (radius / 10.f); 
    shapeDef.enableContactEvents = true; 
    
    b2CreateCircleShape(bodyId, &shapeDef, &circle);
    return bodyId;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "FusionBox: Stellar Evolution");
    window.setFramerateLimit(60);
    loadAssets();
    loadHighScore();

    std::vector<sf::CircleShape> stars;
    for (int i = 0; i < 120; ++i) {
        sf::CircleShape star(static_cast<float>(rand() % 2 + 1));
        star.setPosition({(float)(rand() % 800), (float)(rand() % 600)});
        star.setFillColor(sf::Color(255, 255, 255, rand() % 100 + 50));
        stars.push_back(star);
    }

    sf::Font font;
    font.openFromFile("arial.ttf");
    sf::Text uiText(font);
    uiText.setCharacterSize(18);
    uiText.setFillColor(sf::Color(200, 200, 255));

    std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<int> dist(1, 2);

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 7.0f}; 
    b2WorldId worldId = b2CreateWorld(&worldDef);

    b2ShapeDef wallShapeDef = b2DefaultShapeDef();
    auto createWall = [&](float x, float y, float w, float h) {
        b2BodyDef bd = b2DefaultBodyDef(); bd.position = {x / SCALE, y / SCALE};
        b2BodyId bid = b2CreateBody(worldId, &bd);
        b2Polygon box = b2MakeBox(w / SCALE, h / SCALE);
        b2CreatePolygonShape(bid, &wallShapeDef, &box);
    };
    createWall(400, 595, 400, 5); createWall(5, 300, 5, 300); createWall(795, 300, 5, 300);

    std::vector<PhysicsObject> objects;
    int score = 0;
    bool isGameOver = false;
    float spawnTimer = 0.8f;
    int nextLevel = dist(rng);
    float screenShake = 0.f;
    sf::Clock dtClock;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            
            if (isGameOver && event->is<sf::Event::KeyPressed>()) {
                if (auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::R) {
                        for (auto& obj : objects) b2DestroyBody(obj.bodyId);
                        objects.clear(); score = 0; isGameOver = false;
                    }
                }
            }
            if (!isGameOver && event->is<sf::Event::MouseButtonPressed>() && spawnTimer >= 0.8f) {
                sf::Vector2i mPos = sf::Mouse::getPosition(window);
                float radius = 20.f + (nextLevel * 10.f);
                
                // --- CLAMPING LOGIC ---
                float minX = 10.f + radius; 
                float maxX = 790.f - radius;
                float spawnX = std::max(minX, std::min((float)mPos.x, maxX));
                
                b2BodyId bId = createCelestialBody(worldId, spawnX / SCALE, 40.f / SCALE, radius);
                
                objects.push_back({bId, sf::Sprite(celestialTextures[nextLevel]), nextLevel, false});
                float tSize = (float)celestialTextures[nextLevel].getSize().x;
                objects.back().sprite.setScale({(radius * 2.f) / tSize, (radius * 2.f) / tSize});
                objects.back().sprite.setOrigin({tSize / 2.f, tSize / 2.f});
                
                nextLevel = dist(rng);
                spawnTimer = 0.f;
            }
        }

        if (!isGameOver) {
            spawnTimer += 1.0f / 60.0f;
            if (screenShake > 0) screenShake -= 0.5f;

            b2World_Step(worldId, 1.0f / 60.0f, 4);

            // Fusion Detection
            b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
            for (int i = 0; i < contactEvents.beginCount; ++i) {
                b2BodyId bA = b2Shape_GetBody(contactEvents.beginEvents[i].shapeIdA);
                b2BodyId bB = b2Shape_GetBody(contactEvents.beginEvents[i].shapeIdB);
                int idxA = -1, idxB = -1;
                for(int j=0; j<(int)objects.size(); j++) {
                    if (objects[j].bodyId.index1 == bA.index1) idxA = j;
                    if (objects[j].bodyId.index1 == bB.index1) idxB = j;
                }
                if (idxA != -1 && idxB != -1 && idxA != idxB && objects[idxA].level == objects[idxB].level) {
                    objects[idxA].markedForDeletion = true; objects[idxB].markedForDeletion = true;
                }
            }

            // Merging
            for (int i = 0; i < (int)objects.size(); i++) {
                if (objects[i].markedForDeletion) {
                    for(int j = i + 1; j < (int)objects.size(); j++) {
                        if(objects[j].markedForDeletion && objects[j].level == objects[i].level) {
                            b2Vec2 pA = b2Body_GetPosition(objects[i].bodyId);
                            b2Vec2 pB = b2Body_GetPosition(objects[j].bodyId);
                            int nextL = objects[i].level + 1;
                            if (nextL > 8) nextL = 8; 
                            float nextR = 20.f + (nextL * 10.f);
                            
                            score += nextL * 100; saveHighScore(score);
                            if (nextL >= 7) screenShake = 5.0f;

                            b2BodyId nId = createCelestialBody(worldId, (pA.x+pB.x)/2.f, (pA.y+pB.y)/2.f, nextR);
                            b2DestroyBody(objects[i].bodyId); b2DestroyBody(objects[j].bodyId);
                            
                            PhysicsObject nObj = {nId, sf::Sprite(celestialTextures[nextL]), nextL, false};
                            float tSize = (float)celestialTextures[nextL].getSize().x;
                            nObj.sprite.setScale({(nextR * 2.f) / tSize, (nextR * 2.f) / tSize});
                            nObj.sprite.setOrigin({tSize / 2.f, tSize / 2.f});

                            objects.erase(objects.begin() + j); objects.erase(objects.begin() + i);
                            objects.push_back(std::move(nObj));
                            i--; break;
                        }
                    }
                }
            }
            

            // Sync Sprites and Check Game Over
            for (auto& obj : objects) {
                b2Vec2 pos = b2Body_GetPosition(obj.bodyId);
                float ang = b2Rot_GetAngle(b2Body_GetRotation(obj.bodyId));
                obj.sprite.setPosition({pos.x * SCALE, pos.y * SCALE});
                obj.sprite.setRotation(sf::degrees(ang * 180.f / 3.14159f));
                
                // Game Over Logic
                if (pos.y * SCALE < 140.f && spawnTimer > 1.0f) { 
                    b2Vec2 vel = b2Body_GetLinearVelocity(obj.bodyId);
                    if (std::abs(vel.y) < 0.1f && std::abs(vel.x) < 0.1f) {
                        isGameOver = true;
                    }
                }
            }
        }

        // --- DRAWING ---
        sf::View view = window.getDefaultView();
        if (screenShake > 0) {
            view.setCenter({400.f + (float)(rand()%10 - 5), 300.f + (float)(rand()%10 - 5)});
        }
        window.setView(view);

        window.clear(sf::Color(10, 10, 25));
        for (const auto& star : stars) window.draw(star);
        
        // Danger Line
        sf::RectangleShape danger({800.f, 2.f}); 
        danger.setFillColor(sf::Color(255, 0, 0, 100));
        danger.setPosition({0.f, 140.f}); 
        window.draw(danger);
        
        // CORRECTED PREVIEW LOGIC (Must be here to draw every frame)
        if (!isGameOver) {
            sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            float pRad = 20.f + (nextLevel * 10.f);
            float pX = std::max(10.f + pRad, std::min(mPos.x, 790.f - pRad));

            sf::Sprite previewSprite(celestialTextures[nextLevel]);
            float tSize = (float)celestialTextures[nextLevel].getSize().x;
            previewSprite.setScale({(pRad * 2.f) / tSize, (pRad * 2.f) / tSize});
            previewSprite.setOrigin({tSize / 2.f, tSize / 2.f});
            previewSprite.setPosition({pX, 40.f});
            previewSprite.setColor(sf::Color(255, 255, 255, 150)); // Semi-transparent
            
            window.draw(previewSprite);
        }

        for (auto& obj : objects) {
            if (obj.level >= 7) window.draw(obj.sprite, sf::BlendAdd);
            else window.draw(obj.sprite);
        }

        sf::RectangleShape wall({800.f, 10.f}); wall.setFillColor(sf::Color(40, 40, 60));
        wall.setPosition({0.f, 590.f}); window.draw(wall);

        uiText.setString("Score: " + std::to_string(score) + " | High: " + std::to_string(highScore));
        uiText.setPosition({20, 20}); window.draw(uiText);

        if (isGameOver) {
            sf::RectangleShape ov({800.f, 600.f}); ov.setFillColor(sf::Color(0,0,0,180));
            window.draw(ov);
            uiText.setString("SYSTEM COLLAPSE\nPress R to Restart");
            uiText.setCharacterSize(30); uiText.setPosition({280, 250}); window.draw(uiText);
        }

        window.display();
    }
    b2DestroyWorld(worldId);
    return 0;
}