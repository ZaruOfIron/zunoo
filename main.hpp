#pragma once
#ifndef ___ZUNOO___MAIN_HPP___
#define ___ZUNOO___MAIN_HPP___

#include <boost/optional.hpp>
#include <deque>
#include <map>
#include <utility>
#include <vector>

using Hash = int32_t;

enum class COLOR { RED, GREEN, BLUE };

class Turn
{
public:
    enum TagTurn { WHITE, BLACK };

private:
    TagTurn turn_;

public:
    Turn(TagTurn turn)
        : turn_(turn)
    {}

    bool operator==(const Turn& rhs) const { return turn_ == rhs.turn_; }
    bool operator!=(const Turn& rhs) const { return turn_ != rhs.turn_; }

    bool isWhite() const { return turn_ == WHITE; }
    bool isBlack() const { return turn_ == BLACK; }

    Turn getChangedTurn() const { return Turn(*this).change(); }

    Turn& change() { turn_ = turn_ == WHITE ? BLACK : WHITE;    return *this; }
};

struct Te
{
    COLOR color;
    int num;

    Te(COLOR color_, int num_);

    bool operator==(const Te& rhs) const { return color == rhs.color && num == rhs.num; }
    bool operator!=(const Te& rhs) const { return color != rhs.color || num != rhs.num; }
};

class Board
{
private:
    Turn turn_;
    class CountsData
    {
    private:
        int red_, green_, blue_;

    public:
        CountsData(int red, int green, int blue);

        int& operator[](COLOR color);
        const int& operator[](COLOR color) const;
        
        template<class Prod> void foreach(Prod prod);
        template<class Prod> void foreach(Prod prod) const;
    } counts_;

public:
    Board(int red, int green, int blue);

    void forward(const Te& te);
    void back(const Te& te);

    const Turn& getTurn() const { return turn_; }
    int getCount(COLOR color) const;
    Hash getHash() const;
    void getAllTes(std::vector<Te>& tes) const;

    bool hasFinished() const;
};

class Player
{
private:
public:
    Player(){}
    virtual ~Player(){}

    virtual boost::optional<Te> update(const Board& board) = 0;
};

class AI : public Player
{
private:
    static std::map<Hash, Turn> hashTable_;
    std::deque<Te> best_;
    int bestNextHash_;

public:
    AI();
    ~AI(){}

    boost::optional<Te> update(const Board& board) override;

private:
    static Turn solve(Board& board, std::deque<Te>& best, int depth);
};

class CUIUser : public Player
{
public:
    CUIUser(){}
    ~CUIUser(){}

    boost::optional<Te> update(const Board& board) override;
};

class Game
{
private:
    Board board_;
    Player &white_, &black_;
    boost::optional<Turn> winnerTurn_;

public:
    Game(const Board& initialBoard, Player& white, Player& black);

    int getCount(COLOR color) const;
    Turn getTurn() const { return board_.getTurn(); }

    boost::optional<Te> update();
    const boost::optional<Turn>& getWinnerTurn() const;
};

#endif
