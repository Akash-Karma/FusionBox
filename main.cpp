#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <optional>
#include <map>
#include <iostream>
#include <fstream>
#include <cmath>
#include <random>

const float SCALE = 30.f;

struct Particle {
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    float lifetime;
};

struct PhysicsObject {
    b2BodyId bodyId;
    sf::CircleShape shape;
    int level;
    bool markedForDeletion = false;
};

// --- GLOBAL HIGH SCORE HELPERS ---
int highScore = 0;
void loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file >> highScore) {}
}
void saveHighScore(int s) {
    if (s > highScore) {
        highScore = s;
        std::ofstream file("highscore.txt");
        file << highScore;
    }
}

sf::Color getLevelColor(int level) {
    static std::map<int, sf::Color> colors = {
        {1, sf::Color::Cyan}, {2, sf::Color::Magenta}, {3, sf::Color::Yellow}, 
        {4, sf::Color::Green}, {5, sf::Color::Red}, {6, sf::Color::Blue},
        {7, sf::Color(255, 165, 0)}, {8, sf::Color(128, 0, 128)}
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
    sf::RenderWindow window(sf::VideoMode({800, 600}), "FusionBox: The Final Polish");
    window.setFramerateLimit(60);
    sf::Clock gameClock;
    loadHighScore();

    // --- FONT & TEXT SETUP ---
    sf::Font font;
    bool hasFont = font.openFromFile("arial.ttf"); // Ensure arial.ttf is in your folder!

    sf::Text scoreText(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition({20.f, 20.f});

    sf::Text highScoreText(font);
    highScoreText.setCharacterSize(18);
    highScoreText.setFillColor(sf::Color(180, 180, 180));
    highScoreText.setPosition({20.f, 50.f});

    sf::Text gameOverText(font, "GAME OVER\nPress R to Restart");
    gameOverText.setCharacterSize(50);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setOrigin(gameOverText.getLocalBounds().getCenter());
    gameOverText.setPosition({400.f, 300.f});

    std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<int> dist(1, 3);

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 9.8f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    // Walls
    b2ShapeDef wallDef = b2DefaultShapeDef();
    auto createWall = [&](float x, float y, float w, float h) {
        b2BodyDef bd = b2DefaultBodyDef(); bd.position = {x / SCALE, y / SCALE};
        b2BodyId bid = b2CreateBody(worldId, &bd);
        b2Polygon box = b2MakeBox(w / SCALE, h / SCALE);
        b2CreatePolygonShape(bid, &wallDef, &box);
    };
    createWall(400, 590, 400, 10); createWall(5, 300, 5, 300); createWall(795, 300, 5, 300);

    std::vector<Particle> particles;
    std::vector<PhysicsObject> objects;
    int score = 0;
    bool isGameOver = false;
    float spawnTimer = 0.6f;
    int nextLevel = dist(rng);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            
            if (isGameOver && event->is<sf::Event::KeyPressed>()) {
                if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyEvent->code == sf::Keyboard::Key::R) {
                        for (auto& obj : objects) b2DestroyBody(obj.bodyId);
                        objects.clear(); particles.clear();
                        score = 0; isGameOver = false;
                    }
                }
            }

            if (!isGameOver && event->is<sf::Event::MouseButtonPressed>() && spawnTimer >= 0.6f) {
                sf::Vector2i mPos = sf::Mouse::getPosition(window);
                float radius = 15.f + (nextLevel * 5.f);
                b2BodyId bId = createCircleBody(worldId, (float)mPos.x / SCALE, 80.f / SCALE, radius);
                objects.push_back({bId, sf::CircleShape(radius), nextLevel});
                objects.back().shape.setOrigin({radius, radius});
                objects.back().shape.setFillColor(getLevelColor(nextLevel));
                nextLevel = dist(rng);
                spawnTimer = 0.f;
            }
        }

        if (!isGameOver) {
            spawnTimer += 1.0f / 60.0f;
            b2World_Step(worldId, 1.0f / 60.0f, 4);

            // Fusion Detection
            b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
            for (int i = 0; i < contactEvents.beginCount; ++i) {
                b2BodyId bA = b2Shape_GetBody(contactEvents.beginEvents[i].shapeIdA);
                b2BodyId bB = b2Shape_GetBody(contactEvents.beginEvents[i].shapeIdB);
                int idxA = -1, idxB = -1;
                for(int j=0; j<(int)objects.size(); ++j) {
                    if (objects[j].bodyId.index1 == bA.index1) idxA = j;
                    if (objects[j].bodyId.index1 == bB.index1) idxB = j;
                }
                if (idxA != -1 && idxB != -1 && idxA != idxB && objects[idxA].level == objects[idxB].level) {
                    objects[idxA].markedForDeletion = true; objects[idxB].markedForDeletion = true;
                }
            }

            // Merging + Score
            for (int i = 0; i < (int)objects.size(); ++i) {
                if (objects[i].markedForDeletion) {
                    for(int j = i + 1; j < (int)objects.size(); ++j) {
                        if(objects[j].markedForDeletion && objects[j].level == objects[i].level) {
                            b2Vec2 pA = b2Body_GetPosition(objects[i].bodyId);
                            b2Vec2 pB = b2Body_GetPosition(objects[j].bodyId);
                            b2Vec2 mid = { (pA.x + pB.x)/2.f, (pA.y + pB.y)/2.f };
                            
                            for(int k=0; k<12; k++) {
                                Particle p; p.shape.setSize({3.f, 3.f});
                                p.shape.setFillColor(getLevelColor(objects[i].level));
                                p.shape.setPosition({mid.x * SCALE, mid.y * SCALE});
                                float ang = (float)(rand() % 360) * 0.0174f;
                                p.velocity = { cos(ang) * 4.f, sin(ang) * 4.f };
                                p.lifetime = 0.8f; particles.push_back(p);
                            }

                            int nextL = objects[i].level + 1;
                            float nextR = 15.f + (nextL * 5.f);
                            score += nextL * 25;
                            saveHighScore(score);

                            b2BodyId nId = createCircleBody(worldId, mid.x, mid.y, nextR);
                            b2DestroyBody(objects[i].bodyId); b2DestroyBody(objects[j].bodyId);
                            objects.erase(objects.begin() + j); objects.erase(objects.begin() + i);
                            objects.push_back({nId, sf::CircleShape(nextR), nextL});
                            objects.back().shape.setOrigin({nextR, nextR});
                            objects.back().shape.setFillColor(getLevelColor(nextL));
                            i--; break;
                        }
                    }
                }
            }

            // Particles Update
            for (int i=0; i<(int)particles.size(); i++) {
                particles[i].shape.move(particles[i].velocity);
                particles[i].lifetime -= 0.02f;
                if (particles[i].lifetime <= 0) { particles.erase(particles.begin() + i); i--; }
            }

            // Sync
            for (auto& obj : objects) {
                b2Vec2 pos = b2Body_GetPosition(obj.bodyId);
                obj.shape.setPosition({pos.x * SCALE, pos.y * SCALE});
                b2Vec2 vel = b2Body_GetLinearVelocity(obj.bodyId);
                if (pos.y * SCALE < 120.f && pos.y * SCALE > 85.f && (vel.x*vel.x + vel.y*vel.y) < 0.01f) isGameOver = true;
            }

            // Update UI Text
            scoreText.setString("Score: " + std::to_string(score));
            highScoreText.setString("High Score: " + std::to_string(highScore));
        }

        window.clear(sf::Color(15, 15, 15));
        
        sf::RectangleShape danger({800.f, 2.f}); danger.setFillColor(sf::Color(255,0,0,80));
        danger.setPosition({0.f, 120.f}); window.draw(danger);

        if (!isGameOver) {
            sf::Vector2i mPos = sf::Mouse::getPosition(window);
            float pRad = 15.f + (nextLevel * 5.f);
            sf::CircleShape nextPreview(pRad);
            nextPreview.setOrigin({pRad, pRad});
            nextPreview.setPosition({(float)mPos.x, 60.f});
            nextPreview.setFillColor(getLevelColor(nextLevel));
            window.draw(nextPreview);
        }

        for (auto& p : particles) window.draw(p.shape);
        for (auto& obj : objects) window.draw(obj.shape);
        
        // Static Walls
        sf::RectangleShape wall({800.f, 20.f}); wall.setFillColor(sf::Color(50,50,50));
        wall.setPosition({0.f, 580.f}); window.draw(wall);
        wall.setSize({10.f, 600.f}); wall.setPosition({0.f, 0.f}); window.draw(wall);
        wall.setPosition({790.f, 0.f}); window.draw(wall);

        // UI Text
        if (hasFont) {
            window.draw(scoreText);
            window.draw(highScoreText);
            if (isGameOver) window.draw(gameOverText);
        }

        window.display();
    }
    b2DestroyWorld(worldId);
    return 0;
}