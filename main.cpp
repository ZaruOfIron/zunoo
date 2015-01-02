#include "main.hpp"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <cstdint>
#include <iostream>

Te::Te(COLOR color_, int num_)
    : color(color_), num(num_)
{
    assert(num_ != 0);
}

///

Board::CountsData::CountsData(int red, int green, int blue)
    : red_(red), green_(green), blue_(blue)
{}

int& Board::CountsData::operator[](COLOR color)
{
    switch(color)
    {
    case COLOR::RED:
        return red_;
    case COLOR::GREEN:
        return green_;
    case COLOR::BLUE:
        return blue_;
    }

    assert(false);
}

const int& Board::CountsData::operator[](COLOR color) const
{
    switch(color)
    {
    case COLOR::RED:
        return red_;
    case COLOR::GREEN:
        return green_;
    case COLOR::BLUE:
        return blue_;
    }

    assert(false);
}

template<class Prod> void Board::CountsData::foreach(Prod prod)
{
    prod(COLOR::RED, red_);
    prod(COLOR::GREEN, green_);
    prod(COLOR::BLUE, blue_);
}

template<class Prod> void Board::CountsData::foreach(Prod prod) const
{
    prod(COLOR::RED, red_);
    prod(COLOR::GREEN, green_);
    prod(COLOR::BLUE, blue_);
}

///

Board::Board(int red, int green, int blue)
    : turn_(Turn::WHITE), counts_(red, green, blue)
{}

void Board::forward(const Te& te)
{
    counts_[te.color] -= te.num;
    turn_.change();
}

void Board::back(const Te& te)
{
    counts_[te.color] += te.num;
    turn_.change();
}

int Board::getCount(COLOR color) const
{
    return counts_[color];
}

Hash Board::getHash() const
{
    Hash hash = 0;
    hash |= static_cast<int8_t>(counts_[COLOR::RED]);
    hash |= static_cast<int8_t>(counts_[COLOR::GREEN]) << 8;
    hash |= static_cast<int8_t>(counts_[COLOR::BLUE]) << 16;
    hash |= static_cast<int8_t>(turn_.isWhite()) << 24;

    return hash;
}

void Board::getAllTes(std::vector<Te>& tes) const
{
    counts_.foreach([&tes](COLOR color, int num) -> void {
        for(int i = 0;i < num;i++)  tes.push_back(Te(color, i + 1));
    });
}

bool Board::hasFinished() const
{
    return counts_[COLOR::RED] == 0 && counts_[COLOR::GREEN] == 0 && counts_[COLOR::BLUE] == 0;
}

///

std::map<Hash, Turn> AI::hashTable_;

AI::AI()
    : bestNextHash_(-1)
{}

boost::optional<Te> AI::update(const Board& nowBoard)
{
    Board board(nowBoard);

    if(board.getHash() != bestNextHash_){
        best_.clear();
        solve(board, best_, 0);
        if(best_.size() == 0)    return boost::optional<Te>();
    }

    if(best_.size() < 3)    return best_.front();

    Te ret = best_.front();

    for(int i = 0;i < 2;i++){
        board.forward(best_.front());
        best_.pop_front();
    }
    bestNextHash_ = board.getHash();

    return boost::optional<Te>(ret);
}

Turn AI::solve(Board& board, std::deque<Te>& best, int depth)
{
    if(board.hasFinished())	return board.getTurn();
	Hash hash = board.getHash();

    if(depth != 0){
        auto it = hashTable_.find(hash);
        if(it != hashTable_.end())	return it->second;
    }

	std::vector<Te> tes;	board.getAllTes(tes);
	assert(!tes.empty());
	if(best.size() <= depth)	best.push_back(tes.back());

	for(auto& te : tes){
		board.forward(te);
		Turn turn = solve(board, best, depth + 1);
		board.back(te);
		if(board.getTurn() == turn){
			best.at(depth) = te;
			best.resize(depth + 1, Te(COLOR::RED, 100));
			hashTable_.insert(std::make_pair(hash, turn));
			return turn;
		}
	}

    Turn turn(board.getTurn());  turn.change();
	hashTable_.insert(std::make_pair(hash, turn));
	return turn;
}

///

boost::optional<Te> CUIUser::update(const Board& board)
{
    std::vector<Te> tes;    board.getAllTes(tes);

    while(true){
        std::string input;
        
        std::cout << "RGB: ";
        std::getline(std::cin, input);
        COLOR color;    
        switch(input.at(0))
        {
        case 'r':   case 'R':
            color = COLOR::RED;
            break;
        case 'g':   case 'G':
            color = COLOR::GREEN;
            break;
        case 'b':   case 'B':
            color = COLOR::BLUE;
            break;
        default:
            assert(false);
        }

        std::cout << "num: ";
        std::getline(std::cin, input);
        int num = boost::lexical_cast<int>(input);

        Te te(color, num);
        if(std::find(tes.begin(), tes.end(), te) != tes.end())  return te;
    }
}

///

Game::Game(const Board& initialBoard, Player& white, Player& black)
    : board_(initialBoard), white_(white), black_(black)
{}

int Game::getCount(COLOR color) const
{
    return board_.getCount(color);
}

boost::optional<Te> Game::update()
{
    assert(!winnerTurn_);

    if(board_.hasFinished()){
        winnerTurn_ = board_.getTurn();
        return boost::optional<Te>();
    }

    auto& player = board_.getTurn().isWhite() ? white_ : black_;
    auto te = player.update(board_);
    if(!te) winnerTurn_ = board_.getTurn().getChangedTurn();
    else    board_.forward(*te);

    return te;
}

const boost::optional<Turn>& Game::getWinnerTurn() const
{
    return winnerTurn_;
}

///

struct GameConfig
{
    std::unique_ptr<Player> players[2];
    int red, green, blue;
};

GameConfig parseArgs(int ac, char **av)
{
    assert(ac == 5 + 1);

    GameConfig ret;

    for(int i = 0;i < 2;i++){
        switch(av[i + 1][0])
        {
            case 'a':   case 'A':   ret.players[i].reset(new AI);   break;
            case 'u':   case 'U':   ret.players[i].reset(new CUIUser);  break;
            default:    assert(false);
        }
    }

    ret.red = boost::lexical_cast<int>(av[3]);
    ret.green = boost::lexical_cast<int>(av[4]);
    ret.blue = boost::lexical_cast<int>(av[5]);

    return ret;
}

const char *turn2str(const Turn& turn)
{
    return turn.isWhite() ? "White" : "Black";
}

const char *color2str(COLOR color)
{
    switch(color)
    {
    case COLOR::RED:    return "red";
    case COLOR::GREEN:  return "green";
    case COLOR::BLUE:   return "blue";
    }

    return "unknown";
}

int main(int ac, char **av)
{
    GameConfig config = parseArgs(ac, av);
    Game game(Board(config.red, config.green, config.blue), *(config.players[0]), *(config.players[1]));

    while(true){
        std::cout
            << "   RED   " << "|" << "  GREEN  " << "|" << "  BLUE   " << std::endl
            << "=========" << "+" << "=========" << "+" << "=========" << std::endl
            << boost::format("   %02d    |") % game.getCount(COLOR::RED)
            << boost::format("   %02d    |") % game.getCount(COLOR::GREEN)
            << boost::format("   %02d    ") % game.getCount(COLOR::BLUE) << std::endl
            << "------------------------------" << std::endl;

        auto te = game.update();
        if(!te) break;
        std::cout << "==> " << turn2str(game.getTurn().getChangedTurn()) << " " << color2str(te->color) << " " << te->num << std::endl;
    }

    Turn winner = *(game.getWinnerTurn());
    std::cout << "WINNER: " << turn2str(winner) << std::endl;
}

