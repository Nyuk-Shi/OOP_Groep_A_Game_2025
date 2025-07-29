#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <stack>
#include <random>
#include <chrono>
#include <sstream>
#include <iostream>


const int CELL_SIZE = 32;
const int COLS = 25, ROWS = 18;
const int WIDTH = COLS * CELL_SIZE;
const int HEIGHT = ROWS * CELL_SIZE;

enum directions { down, right, up, left };

struct Cell {
    bool walls[4] = { true, true, true, true };
    bool visited = false;
};

std::vector<std::vector<Cell>> maze;
std::mt19937 rng(std::random_device{}());

void carveMaze(int sx = 0, int sy = 0) {
    const int DX[4] = { 0, 1, 0, -1 }; // UP, RIGHT, DOWN, LEFT
    const int DY[4] = { -1, 0, 1, 0 };

    maze.assign(COLS, std::vector<Cell>(ROWS)); // Clear maze
    std::stack<sf::Vector2i> st;
    st.push({ sx, sy });
    maze[sx][sy].visited = true;

    while (!st.empty()) {
        auto [x, y] = st.top();
        std::array<int, 4> dirs = { 0, 1, 2, 3 };
        std::shuffle(dirs.begin(), dirs.end(), rng);

        bool moved = false;
        for (int d : dirs) {
            int nx = x + DX[d];
            int ny = y + DY[d];
            if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && !maze[nx][ny].visited) {
                // Remove wall between current and next cell
                maze[x][y].walls[d] = false;
                maze[nx][ny].walls[(d + 2) % 4] = false;

                maze[nx][ny].visited = true;
                st.push({ nx, ny });
                moved = true;
                break;
            }
        }

        if (!moved) st.pop(); // Backtrack
    }
}


int main() {
    sf::RenderWindow window(sf::VideoMode({ WIDTH, HEIGHT }), "PIXEL PATH");
    window.setFramerateLimit(60);
    sf::Vector2u windowSize = window.getSize();
    sf::Font font;
    if (!font.openFromFile("Fonts/rainyhearts.ttf")) {

        std::cerr << "Failed to load font!" << std::endl;

        return -1;

    }

    //Backgrounds
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("Images/MazeBG.jpg")) {
        std::cerr << "ERROR::COULD NOT LOAD FILE::Images/MazeBG.jpg" << std::endl;
        return -1;
    }
    sf::Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setPosition({ 0, 0 });

    sf::Texture startBGTexture;
    if (!startBGTexture.loadFromFile("Images/StartBG.jpg")) {
        std::cerr << "ERROR::COULD NOT LOAD FILE::Images/StartBG.jpg" << std::endl;
        return -1;
    }
    sf::Sprite startBGSprite(startBGTexture);
    startBGSprite.setPosition({ 0, 0 });

    //Resizing the Background
    sf::Vector2u imageSize = startBGTexture.getSize();
    float scaleX = static_cast<float>(windowSize.x) / imageSize.x;
    float scaleY = static_cast<float>(windowSize.y) / imageSize.y;
    startBGSprite.setScale({ scaleX, scaleY });


    //Sound
    sf::SoundBuffer startBuffer("Sounds/GameStart.mp3");
    sf::Sound gameStart(startBuffer);

    sf::SoundBuffer coinBuffer("Sounds/CoinPickUp.wav");
    sf::Sound coinPickUp(coinBuffer);

    sf::SoundBuffer boostBuffer("Sounds/BoostSound.mp3");
    sf::Sound boostSound(boostBuffer);


    carveMaze();

    const float playerSpeed = 150.0f; // pixels per second

    sf::Vector2i playerCell(0, 0);
    sf::Vector2f playerPos(CELL_SIZE / 4.0f, CELL_SIZE / 4.0f); // Position in pixels
    sf::Vector2f moveDir(0.f, 0.f);    // Movement direction
    sf::Vector2i targetCell = playerCell;
    sf::RectangleShape playerShape({ CELL_SIZE / 2.0f, CELL_SIZE / 2.0f });
    playerShape.setFillColor(sf::Color::Transparent);


    sf::Texture playerTexture;
    if (!playerTexture.loadFromFile("Sprites/ExampleSprite.png")) {
        std::cerr << "ERROR::COULD NOT LOAD FILE::Sprites/ExampleSprite.png" << std::endl;
        return -1;
    }
    sf::Sprite playerSprite(playerTexture);

    sf::IntRect dir[4];
    for (int i = 0; i < 4; i++) {
        dir[i] = sf::IntRect({ {32 * i, 0 }, {32,32} });
    }
    playerSprite.setTextureRect(dir[down]);
    playerSprite.setOrigin({ 16,16 });
    playerSprite.setPosition(playerShape.getPosition());

    std::uniform_int_distribution<int> dX(0, COLS - 1), dY(0, ROWS - 1);
    std::vector<sf::Vector2i> coins;
    auto spawnCoin = [&]() {
        coins.push_back({ dX(rng), dY(rng) });
        };
    for (int i = 0; i < 10; i++) spawnCoin();

    int score = 0;
    bool boost = false, started = false, paused = false, finished = false;
    sf::Clock boostClock, gameClock, clock;
    sf::Text info(font);
    info.setCharacterSize(24);
    info.setString("");
    info.setFillColor(sf::Color::White);
    info.setPosition({ 20, HEIGHT / 2 - 30 });

    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        float delta = deltaTime.asSeconds();
        sf::Time pausedTime = sf::Time::Zero;
        sf::Time elapsed = gameClock.getElapsedTime();



        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Escape))
                window.close();

        }
        // Menu logic
        if (!started) {
            sf::FloatRect infoRect = info.getLocalBounds();
            info.setOrigin(infoRect.getCenter());
            info.setPosition({ WIDTH / 2.0f, HEIGHT / 2.0f });
            info.setCharacterSize(60u);
            info.setString("PIXEL PATH\n\nEnter   Start\nEsc     Exit");
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Enter)) {
                gameStart.play();
                started = true; gameClock.restart();

            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Escape))
                window.close();
        }
        else if (finished) {
            std::ostringstream ss;
            ss << "CONGRATULATIONS!!\nYou Win!\nScore: " << score << "\nTime: " <<
                static_cast<int>(elapsed.asSeconds()) << "s\n\nEsc to exit";
            info.setString(ss.str());
            sf::FloatRect infoRect = info.getLocalBounds();
            info.setOrigin(infoRect.getCenter());
            info.setPosition({ WIDTH / 2.0f, HEIGHT / 2.0f });
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Escape))
                window.close();
        }
        else {
            // Toggle pause
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::P)) {
                paused = !paused;
                if (paused) {
                    elapsed = elapsed - pausedTime;
                }
                else {
                    // If not paused, store the current time for later pause
                    pausedTime = gameClock.getElapsedTime();
                }
                sf::sleep(sf::milliseconds(200)); // Debounce
            }

            if (!paused) {
                // Movement & collision
                auto tryMove = [&](int dx, int dy) {
                    if (moveDir != sf::Vector2f(0.f, 0.f)) return; // Already moving

                    int x = playerCell.x, y = playerCell.y;
                    int dir = (dx == 1 ? 1 : dx == -1 ? 3 : dy == 1 ? 2 : 0);
                    if (!maze[x][y].walls[dir]) {
                        targetCell = playerCell + sf::Vector2i(dx, dy);
                        moveDir = sf::Vector2f(float(dx), float(dy));
                    }
                    };
                if (moveDir == sf::Vector2f(0.f, 0.f)) {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W)) {
                        tryMove(0, -1);
                        playerSprite.setTextureRect(dir[up]);
                        playerSprite.setPosition(playerShape.getPosition());
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S)) {
                        tryMove(0, 1);
                        playerSprite.setTextureRect(dir[down]);
                        playerSprite.setPosition(playerShape.getPosition());
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) {
                        tryMove(-1, 0);
                        playerSprite.setTextureRect(dir[left]);
                        playerSprite.setPosition(playerShape.getPosition());
                    }
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) {
                        playerSprite.setTextureRect(dir[right]);
                        playerSprite.setPosition(playerShape.getPosition());
                        tryMove(1, 0);
                    }
                }
                if (moveDir != sf::Vector2f(0.f, 0.f)) {
                    sf::Vector2f targetPos = {
                        targetCell.x * CELL_SIZE + CELL_SIZE / 4.0f,
                        targetCell.y * CELL_SIZE + CELL_SIZE / 4.0f
                    };

                    sf::Vector2f direction = sf::Vector2f(
                        static_cast<float>(moveDir.x) * playerSpeed * delta,
                        static_cast<float>(moveDir.y) * playerSpeed * delta
                    );

                    playerPos += direction;

                    // Check if we've reached or passed the target
                    bool reachedX = (moveDir.x == 0) ||
                        (moveDir.x > 0 && playerPos.x >= targetPos.x) ||
                        (moveDir.x < 0 && playerPos.x <= targetPos.x);

                    bool reachedY = (moveDir.y == 0) ||
                        (moveDir.y > 0 && playerPos.y >= targetPos.y) ||
                        (moveDir.y < 0 && playerPos.y <= targetPos.y);

                    if (reachedX && reachedY) {
                        playerPos = targetPos;
                        playerCell = targetCell;
                        moveDir = { 0.0f, 0.0f };
                    }
                }
                // Boost
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space) && !boost) {
                    boostSound.play();
                    boost = true; boostClock.restart();
                }
                if (boost && boostClock.getElapsedTime().asSeconds() > 5)
                    boost = false;

                int maxScore = 10;
                // Coin collection
                for (size_t i = 0; i < coins.size(); ) {
                    if (coins[i] == playerCell) {
                        coinPickUp.play();
                        score += (boost ? 2 : 1);
                        coins[i] = coins.back();
                        coins.pop_back();
                        spawnCoin();
                    }
                    else {
                        ++i;
                    }
                }

                // Finish check
                if (score >= maxScore) {
                    finished = true;
                }
            }
        }

        // Draw
        window.clear();
        if (!started || finished) {
            window.draw(startBGSprite);
            window.draw(info);

        }
        else {
            window.draw(backgroundSprite);
            // Maze
            for (int x = 0; x < COLS; x++) {
                for (int y = 0; y < ROWS; y++) {
                    sf::Vector2f base(float(x * CELL_SIZE), float(y * CELL_SIZE));
                    sf::Vertex line[2];
                    line[0].color = line[1].color = sf::Color::White;

                    if (maze[x][y].walls[0]) { line[0].position = base; line[1].position = base + sf::Vector2f(CELL_SIZE, 0); window.draw(line, 2, sf::PrimitiveType::Lines); }
                    if (maze[x][y].walls[1]) { line[0].position = base + sf::Vector2f(CELL_SIZE, 0); line[1].position = base + sf::Vector2f(CELL_SIZE, CELL_SIZE); window.draw(line, 2, sf::PrimitiveType::Lines); }
                    if (maze[x][y].walls[2]) { line[0].position = base + sf::Vector2f(CELL_SIZE, CELL_SIZE); line[1].position = base + sf::Vector2f(0, CELL_SIZE); window.draw(line, 2, sf::PrimitiveType::Lines); }
                    if (maze[x][y].walls[3]) { line[0].position = base + sf::Vector2f(0, CELL_SIZE); line[1].position = base; window.draw(line, 2, sf::PrimitiveType::Lines); }
                }
            }
            // Coins
            for (auto& c : coins) {
                sf::CircleShape coin(CELL_SIZE / 4.0f);
                coin.setFillColor(sf::Color::Yellow);
                coin.setPosition({ c.x * CELL_SIZE + CELL_SIZE / 4.f, c.y * CELL_SIZE + CELL_SIZE / 4.f });
                coin.setOutlineThickness(2.0f);
                window.draw(coin);
            }

            // Player
            playerShape.setPosition({ playerCell.x * CELL_SIZE + CELL_SIZE / 4.f, playerCell.y * CELL_SIZE + CELL_SIZE / 4.f });
            window.draw(playerShape);
            window.draw(playerSprite);

            // HUD
            sf::Text hud(font);
            hud.setCharacterSize(32);
            hud.setFillColor(sf::Color::White);
            std::ostringstream ss;
            ss << "Score: " << score << "   Time: "
                << static_cast<int>(elapsed.asSeconds()) << "s"
                << (paused ? "   [PAUSED]" : "")
                << (boost ? "   BOOST!" : "");
            hud.setString(ss.str());
            hud.setPosition({ 5.f, 5.f });
            window.draw(hud);
        }
        window.display();
    }
    return 0;
}