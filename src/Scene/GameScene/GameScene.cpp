#include "GameScene.hpp"

#include <algorithm>

#include "../../Define/Define.hpp"
#include "../../InputMonitor/InputMonitor.hpp"
#include "../../PuzzleSolver/IceFloorPuzzleSolver/IceFloorPuzzleSolver.hpp"
#include "../../PuzzleSolver/SynchronizedPuzzleSolver/SynchronizedPuzzleSolver.hpp"
#include "../SceneType.hpp"

const std::string GameScene::PARAM_KEY_LEVEL = "PARAM_KEY_LEVEL";

const std::map<Define::CELL_TYPE, char> GameScene::CELL_MARKS = {
    {Define::CELL_TYPE::FLOOR, '.'},
    {Define::CELL_TYPE::WALL, '#'},
    {Define::CELL_TYPE::START, 'P'},
    {Define::CELL_TYPE::GOAL, 'G'},
    {Define::CELL_TYPE::HOLE, 'o'},
};

const std::string GameScene::INNER_MARKS =
    std::string(4, CELL_MARKS.at(Define::CELL_TYPE::FLOOR)) +
    std::string(1, CELL_MARKS.at(Define::CELL_TYPE::WALL));
const std::string GameScene::OUTER_MARKS =
    std::string(1, CELL_MARKS.at(Define::CELL_TYPE::WALL));

const int GameScene::START_NUM = 1;

const int GameScene::THRESHOLD = 10;
const SDL_Point GameScene::PUZZLE_SIZE = {10, 10};

const int GameScene::PLAYER_SPEED = 5;

const std::map<Define::DIRECTION, int> GameScene::MARGIN = {
    {Define::DIRECTION::UP, 25},
    {Define::DIRECTION::DOWN, 25},
    {Define::DIRECTION::LEFT, 25},
    {Define::DIRECTION::RIGHT, 25},
};

/**
 * @brief コンストラクタ
 * @param listener リスナー
 * @param params パラメータ
 */
GameScene::GameScene(IOnChangedListener *listener, const Parameter &params)
    : AbstractScene(listener, params),
      floors(std::make_unique<FloorManager>()),
      holes(std::make_unique<HoleManager>()),
      players(std::make_unique<PlayerManager>(true)),
      walls(std::make_unique<WallManager>()),
      puzzle_generator(nullptr),
      puzzle_solver(nullptr) {

    level = params.get(PARAM_KEY_LEVEL);

    puzzle_generator = std::make_unique<PuzzleGenerator>(
        PUZZLE_SIZE,
        INNER_MARKS,
        OUTER_MARKS,
        GameScene::CELL_MARKS.at(Define::CELL_TYPE::START),
        START_NUM
    );
    puzzle_solver = std::make_unique<IceFloorPuzzleSolver>();
    puzzle_solver->init(GameScene::CELL_MARKS);

    while (puzzle_solver->get_rate() < THRESHOLD) {
        puzzle_solver->reset();
        auto puzzle = puzzle_generator->generate();
        puzzle_solver->solve(std::move(puzzle));
    }

    this->puzzle = *puzzle_solver->get_puzzle();
    this->ans = puzzle_solver->get_ans();

    for (const auto &line : this->puzzle) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", line.c_str());
    }

    static const int width = Define::WINDOW_SIZE.x - MARGIN.at(Define::DIRECTION::LEFT) - MARGIN.at(Define::DIRECTION::RIGHT);
    static const int height = Define::WINDOW_SIZE.y - MARGIN.at(Define::DIRECTION::UP) - MARGIN.at(Define::DIRECTION::DOWN);
    cell_size = std::min(width / PUZZLE_SIZE.x, height / PUZZLE_SIZE.y);
    upper_left = {MARGIN.at(Define::DIRECTION::LEFT), MARGIN.at(Define::DIRECTION::UP)};

    static const int padding = 1;

    std::vector object_positions(PUZZLE_SIZE.y, std::vector<Define::CELL_TYPE>(PUZZLE_SIZE.x, Define::CELL_TYPE::FLOOR));

    std::array<SDL_Color, 2> floor_colors = {
        SDL_Color(Define::WHITE.r - 50, Define::WHITE.g - 50, Define::WHITE.b - 50, Define::WHITE.a),
        SDL_Color(Define::WHITE.r - 70, Define::WHITE.g - 70, Define::WHITE.b - 70, Define::BLACK.a),
    };

    for (int i = 0; i < PUZZLE_SIZE.y; i++) {
        for (int j = 0; j < PUZZLE_SIZE.x; j++) {
            object_positions[i][j] = std::ranges::find_if(CELL_MARKS, [this, i, j](const auto &cell) { return cell.second == this->puzzle[i][j]; })->first;
            SDL_Point coord = {j * cell_size + upper_left.x, i * cell_size + upper_left.y};
            SDL_Point size = {cell_size, cell_size};
            floors->add(coord, size, floor_colors[(i + j) % 2]);
            if (this->puzzle[i][j] == CELL_MARKS.at(Define::CELL_TYPE::WALL)) {
                walls->add(SDL_Point(coord.x + padding, coord.y + padding), SDL_Point(size.x - padding * 2 - 1, size.y - padding * 2 - 1), Define::BLACK);
            }
            if (this->puzzle[i][j] == CELL_MARKS.at(Define::CELL_TYPE::HOLE)) {
                holes->add(SDL_Point(coord.x + padding, coord.y + padding), SDL_Point(size.x - padding * 2 - 1, size.y - padding * 2 - 1), Define::WHITE);
            }
            if (this->puzzle[i][j] == CELL_MARKS.at(Define::CELL_TYPE::START)) {
                players->add(coord, size, size, Define::RED, PLAYER_SPEED);
            }
        }
    }
    players->set_object_positions(object_positions);

    const std::map<Define::DIRECTION, char> DIRECTION_CHARS = {
        {Define::DIRECTION::UP, 'U'},
        {Define::DIRECTION::RIGHT, 'R'},
        {Define::DIRECTION::DOWN, 'D'},
        {Define::DIRECTION::LEFT, 'L'},
    };
    std::string ans_str;
    for (const auto &dir : ans) {
        ans_str += DIRECTION_CHARS.at(dir);
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Answer: %s", ans_str.c_str());
}

/**
 * @brief 更新
 */
void GameScene::update() {
    floors->update();
    holes->update();
    players->update();
    walls->update();

    if (InputMonitor::get_instance().get_pressing_frame_cnt(SDLK_n) == 1) {
        Parameter param;
        param.set(GameScene::PARAM_KEY_LEVEL, level);
        listener->on_changed(SceneType::Game, param, true);
    }
}

/**
 * @brief 描画
 * @param renderer レンダラー
 * @param surface サーフェス
 * @param texture テクスチャ
 * @param font フォント
 */
void GameScene::draw(SDL_Renderer *renderer, SDL_Surface *surface, SDL_Texture *texture, TTF_Font *font) const {
    static_cast<void>(surface), static_cast<void>(texture), static_cast<void>(font);
    floors->draw(renderer);
    holes->draw(renderer);
    players->draw(renderer);
    walls->draw(renderer);
}