#include <iostream>
#include <vector>
#include <stdexcept>
#include <limits>
#include <time.h>
#include <random>
#include <fstream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using vector = std::vector<short>;

// Global shapes array
const char shapes[3]{ 'R', 'P', 'S' };
const std::string shape_names[3]{ "Rock", "Paper", "Scissors" };

// empty vector to pass to Player::get_move() if other history or self history is not important
const vector empty_vec{};


// Euclidian modulo for rotation (% operator uses non Euclidian remainder)
template<typename T, typename F>
requires std::is_integral<T>::value&& std::is_integral<F>::value
T mod_euc(T a, F b) {
	return ((a % b) + b) % b; // This returns modulus with sign of b, so will always be positive if called with positive b
}


//// Move definition

// Move: R (Rock), P (Paper) or S (Scissors)
struct Move {

	// These attributes are public so they can be accessed and modified outside of Move
	char shape{}; // represents shape of Move (Rock, Paper, Scissors as single char (R, P, S))
	short index{}; // represents index of shape in global shapes array

	// Can be initialized with int (shapes_index) or char (shape)
	Move(short shapes_index) : shape{ shapes[shapes_index] }, index{ shapes_index } {};

	// Also set index to corresponding index in shapes array if initialized with char
	Move(char shape_char) : shape{ shape_char } {
		for (short i{}; i < 3; i += 1) {
			if (shapes[i] == shape_char) index = i;
		}
	};

	Move() = default; // enables initialization without braces

	// copy constructor (invoked when passed as function argument)
	Move(const Move& other) : shape{ other.shape }, index{ other.index } {}

	~Move() = default;

	// Rotate method: Rotates Move shape/index by given value (positive -> clock wise; negative -> counter clock wise); returns lvalue copy
	Move rotate_by(short by) {
		// We use C-style casts throughout the code, usually to set an int argument to short
		return Move{ (short)(mod_euc((index + by), 3)) };
	}
};



//// Player strategies

// Virtual base class for all player strategies
struct Player {

	// Every player needs a get_move Method
	virtual Move get_move(const vector& other_history, const vector& self_history) = 0;

	// get player name
	virtual std::string get_name() = 0;

	// Default destructor
	virtual ~Player() = default;


};



// Frequency: Play winning move against most frequently played opponent move
struct Frequency : Player {

	// Every Player derived class at least takes a tag argument which is useful for player identification
	// (e.g. when 2 players of same type play against each other, we can distinguish them by tag)
	Frequency(std::string tag = "") {
		if(not (tag == "")) name = name + " " + tag;
	}

	Move get_move(const vector& other_history, const vector& unused) override {
		// Analyse opponent move history and play move that wins against most frequently played opponent move

		if (other_history.empty()) return Move{ (short)0 }; // return default move if history is empty

		// Set up score array to count opponent move frequency
		int score[3]{ 0, 0, 0 };
		for (short element : other_history) {
			score[element] += 1; // add to score each occurence of corresponding element in history
		}

		// Find index of max element in score (which will be most frequently played opponent move)
		int max{};
		short index{};
		for (int curr_index{}; int element : score) {
			if (element > max) {
				max = element;
				index = curr_index; // index in global shapes array pointing to most frequently played opponent move shape
			}
			curr_index += 1;
		}

		// return most frequently played opponent move rotated by 1 (winning move)
		return Move{ index }.rotate_by(1);
	}

	// get_name is used for printing to console
	std::string get_name() override {
		return name;
	}

	// Every Player derived class has a name attribute
	std::string name = "Frequency Player";
};



// Fixed: Always play the same move
struct Fixed : Player {

	Fixed(char move, std::string tag = "") : fixed_move{ move } {
		if (not (tag == "")) name = name + "(" + move + ")" + tag;
		else name = name + " (" + move + ")";
	};

	Move get_move(const vector& empty1, const vector& empty2) override {
		// Always return the same move
		return fixed_move;
	}

	std::string get_name() override {
		return name;
	}

	std::string name = "Fixed Player";

private:
	Move fixed_move;
};



std::mt19937 init_engine; // initialization engine for Random Player; this is necessary because if we initialize two random players
						  // without a seed argument in quick succession, we get the same seed from time(0) (which only changes every second)

// Random: Always play random move
struct Random : Player {

	// Can supply seed for rng; otherwise pseudo random seed
	Random(int s, std::string tag = "") {
		engine.seed(s);
		if (not (tag == "")) name = name + " " + tag;
	};
	Random(std::string tag = "") {
		engine.seed(distribution(init_engine)+time(0)); // pseudo random seed from current calendar time in seconds and the contribution from our init engine
		if (not (tag == "")) name = name + " " + tag;

	};

	// move history not important to Random Player; can pass empty vectors
	Move get_move(const vector& empty1, const vector& empty2) override {
		short randn{};
		randn = mod_euc(distribution(engine), 3); // distribution returns uniformly distributed shorts; Euclidian modulo with 3 returns our final pseudo-random init value between 0 and 2
		return Move{ randn };
	}

	std::string get_name() override {
		return name;
	}

	std::string name = "Random Player";

private:
	// use mersenne twister random engine (Matsumoto and Nishimura); distribution returns short based on engine input
	std::mt19937 engine;
	std::uniform_int_distribution<int> distribution;

};



// Rotation: Play opponent move rotated by x
struct Rotation : Player {

	Rotation(short by = 0, std::string tag = "") : rotation_by{ by } {
		if (not (tag == "")) name = name + " " + tag;
	};

	Move get_move(const vector& other_history, const vector& self_history) override {
		if (other_history.empty()) return Move{ (short)0 };

		// get last element of opponent move history vector (last opponent move)
		short last = other_history.back();

		// rotate last opponent move by rotation_by and return
		return Move{ last }.rotate_by(rotation_by);
	}

	std::string get_name() override {
		return name;
	}

	std::string name = "Rotation Player";

private:
	short rotation_by{};
};



// Anti_Rotation: Figure out opponent rotation and play winning move against it
struct Anti_Rotation : Player {

	// Verbose argument is used whenever a Player derived class has an internal state than may be interesting; in this case, the internal state
	// is the opponent rotation Anti_Rotation has figured out; if true, will print internal state to console 
	Anti_Rotation(bool verbose = false, std::string tag = "") : verbose{verbose} {
		if (not (tag == "")) name = name + " " + tag;
	}

	Move get_move(const vector& other_history, const vector& self_history) {
		// Assume opponent uses a rotation strategy; look at history and figure out by how much;
		// play winning strategy (which is expected opponent move rotated by 1)

		if (self_history.size() < 2) { // Anti_Rotation needs exactly two entries in opponent and self history in order to figure out opponent Rotation; 
			return Move{ (short)0 };   // so return default Move if self_history size (which is always equal to other history size) is less than 2
		}

		short last_self = self_history[self_history.size() - 2]; // second to last self move
		short other_response = other_history.back(); // last opponent move (opponent response to second to last self move)
		short index_diff = other_response - last_self; // index difference between second to last self move and opponent response
		short rotate = mod_euc(index_diff, 3); // Euclidian modulo of index difference and 3 is opponent rotation

		// print internal state to console if verbose is true
		if (verbose) {
			std::cout << "\nrotation used by other: " << rotate;
			std::cout << "\npredicted other move: " << shapes[mod_euc(self_history.back() + rotate, 3)] << "\n";
		}

		return Move{ self_history.back() }.rotate_by(rotate + 1); // If opponent rotation is known, all we need to do in order to win is rotate last self move by that rotation + 1
	}


	std::string get_name() override {
		return name;
	}

	std::string name = "Anti Rotation Player";
	bool verbose{};
};



// Human: Player that prompts for human input for move decision
struct Human : Player {

	Human(std::string tag = "") {
		if (not (tag == "")) name = name + " " + tag;
	}

	// Move history is not important to human player; if played on console, move history will be accessible by Game::evaluate_round()
	Move get_move(const vector& empty1, const vector& empty2) override {

		// shape variable stores user input
		short shape{};

		while (true) {
			try {
				std::cout << name << ", Input Move shape (0 (Rock), 1 (Paper), 2 (Scissors)): ";
				std::cin >> shape;

				// throw errors if unser input is out of range or invalid type (cin will be in bad state and cin=0)
				if (shape > 2 or shape < 0) throw std::range_error{ "Move shape must be 0 (Rock), 1 (Paper) or 2 (Scissors)." };
				if (!std::cin) throw std::runtime_error{ "Invalid input." };
				break;
			}
			catch (std::range_error& e) {
				// if invalid input: clear last bit in stream buffer and ignore rest of stream buffer until '\n'
				std::cout << "Error: " << e.what() << std::endl;
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				continue;
			}
			catch (std::runtime_error& e) {
				std::cout << "Error: " << e.what() << std::endl;
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				continue;
			}

		}

		return Move{ shape };

	}

	std::string get_name() override {
		return name;
	}

	std::string name = "Pathetic Human Player";
};


//// Meta Players: Can access and play with all previously defined Player strategies 

// evaluate_round() will return index distance between given Moves, which is =0 if Moves are equal, =1 if m1 wins against m2 and =2 if m2 wins against m1
// evaluate round is not a member of any class, because it needs to be called by Meta Players (in order to evaluate strategy performance) and by Game (in order to evaluate current round)
short evaluate_round(const Move m1, const Move m2) {
	// this uses the same principle Anti_Rotation uses in order to figure out opponent rotation, but is applied to current Moves
	short index_diff{};
	index_diff = m1.index - m2.index;
	short index_distance = mod_euc(index_diff, 3);
	return index_distance;
}


// Scoring functions: Used by Meta Player in order to modify internal scores representing different strategies based on previous strategy performance
// Every scoring function applies different operations on given score (e.g. ..._add() adds/subtracts a constant value from score depending on win/loss of respective strategy)

// naive_score_mul() multiplies score by given constants in scoring_vector
// score vector : {draw multiplier, win multiplier, loss multiplier, floor, ceiling, decay constant}
void naive_score_mul(double& score, short index_distance, double score_vector[6]) {

	// scoring_vector is a caller supplied array which contains the constants used by the respective scoring function to modify scores
	// index distance determines whether strategy loses, wins or results in a draw; is supplied by evaluate_round() called by Meta_Player::get_move()
	switch (index_distance) {
	case 0: // draw
		score *= score_vector[0];
		break;
	case 1: // win
		score *= score_vector[1];
		break;
	case 2: // loss
		score *= score_vector[2];
		break;
	}
	if (score < score_vector[3]) score = score_vector[3]; // score_vector[3] is floor: if score is less than floor after scoring function operation, set score equal to floor
														// --> scores will never be smaller than floor; prevents scores from diminishing without bound
	if (score > score_vector[4]) score = score_vector[4]; // score_vector[4] is ceiling: if score is larger than ceiling after scoring function operation, set score equal to ceiling
														// --> scores will never be larger than ceiling; prevents boundless growth
	if (!(score_vector[5] == 1)) score *= score_vector[5]; // score_vector[5] is decay constant: multiply score by decay constant
														// --> every score will continually diminish (decay < 1) or grow (decay > 1) regardless of outcome
}

// naive_score_add() adds to score given constants in scoring_vector
// score vector : {draw constant, win constant, loss constant, floor, ceiling, decay constant}
void naive_score_add(double& score, short index_distance, double score_vector[6]) {
	switch (index_distance) {
	case 0: // draw
		score += score_vector[0];
		break;
	case 1:// win
		score += score_vector[1];
		break;
	case 2: // loss
		score += score_vector[2];
		break;
	}
	if (score < score_vector[3]) score = score_vector[3];
	if (score > score_vector[4]) score = score_vector[4];
	if (!(score_vector[5] == 1)) score *= score_vector[5];
}

// drop_switch_mul() multilies score by constants in scoring_vector in case of win or draw, but sets score back to scoring_vector[2] in case of loss
// score vector : {draw multiplier, win multiplier, loss value, floor, ceiling, decay constant}
void drop_switch_mul(double& score, short index_distance, double score_vector[6]) {
	switch (index_distance) {
	case 0: // draw
		score *= score_vector[0];
		break;
	case 1: // win
		score *= score_vector[1];
		break;
	case 2: // loss
		score = score_vector[2]; // set score back to value (-> drop)
		break;
	}
	if (score < score_vector[3]) score = score_vector[3];
	if (score > score_vector[4]) score = score_vector[4];
	if (!(score_vector[5] == 1)) score *= score_vector[5];
}

// drop_switch_add() adds to score constants in scoring_vector in case of win or draw, buts sets score back to scoring_vector[3] in case of loss
// score vector : {draw constant, win constant, loss value, floor, ceiling, decay constant}
void drop_switch_add(double& score, short index_distance, double score_vector[6]) {
	switch (index_distance) {
	case 0: // draw
		score += score_vector[0];
		break;
	case 1: // win
		score += score_vector[1];
		break;
	case 2: // loss
		score = score_vector[2]; // set score back to value (-> drop)
		break;
	}
	if (score < score_vector[3]) score = score_vector[3];
	if (score > score_vector[4]) score = score_vector[4];
	if (!(score_vector[5] == 1)) score *= score_vector[5];
}


// Type alias for scoring function pointer used to initialize Meta Player
using scoring_func_ptr = void (*)(double& score, short index_distance, double score_vector[6]);


// Meta_Player_Naive has access to all strategies defined above (except Human Player of course) and plays strategy with highest score; keeps internal array of scores
// and modifies these according to previous strategy performance
struct Meta_Player_Naive : Player {

	// Meta Player is initialized with a scoring function pointer and a scoring vector used by the pointed to scoring function
	Meta_Player_Naive(bool verbose = false, std::string tag = "", scoring_func_ptr scoring_func = naive_score_mul, double scoring_vector[] = {}) : \
		verbose{ verbose }, scoring_func{ scoring_func }, scoring_vector{ scoring_vector } {
		if (not (tag == "")) name = name + " " + tag;
	}


	Move get_move(const vector& other_history, const vector& self_history) override {
		if (other_history.size() < 1) return Move{ (short)0 };

		// i is rotation applied to every strategy
		for (int i{}; i < 3; i += 1) {
			// j is index of respective strategy in strategies vector; i and j are used to access the respective score in scores array
			for (int j{}; Player * strat_ptr : strategies) { // loop through every player pointer in strategies array

				// next Move within each iteration is player strategy (Player.get_move(history)) rotated by i (0, 1 or 2)
				// we pass to Player::get_move() the entire move history (notice self_history is first argument -> puts itself in shoes of opponent)
				// except for the last Move pair (vector slice from beginning to end-1); will return Move that would be played against self last Move in current round
				Move next = strat_ptr->get_move(vector(self_history.begin(), self_history.end() - 1), vector(other_history.begin(), other_history.end() - 1)).rotate_by(i);

				// evaluate whether this Move rotated by 1 would win against current opponent last Move
				short index_dist = evaluate_round(next, Move{ other_history[other_history.size() - 1] });

				// call scoring function based on index distance (0=draw, 1=win, 2=loss)
				scoring_func(scores[i][j], index_dist, scoring_vector);

				j += 1; // increment j after scores access; would lead to out of bounds access if incremented any sooner
			}
		}

		// After every possible strategy has been evaluated, find the best perfroming strategy (corresponds to largest value in scores array)
		max_index_i = 0; max_index_j = 0;
		double max{};
		for (int curr_index_i{}; std::vector<double> element : scores) {
			for (int curr_index_j{};  double score : element) {
				if (score > max) {
					max = score;
					max_index_i = curr_index_i;
					max_index_j = curr_index_j;
				}
				curr_index_j += 1;
			}
			curr_index_i += 1;
		}

		if (verbose) get_current_state();
		
		if (max < 1) return teller_rand.get_move(other_history, self_history); // return random Move if no strategy perfroms reasonably well
																			   // --> failsafe: win rate will never drop far below 50% on average if random moves are played

		// return move played by best performing strategy (strategy is always basic strategy (rotation, frequency, ...) and a rotation between 0 and 2)
		return strategies[max_index_j]->get_move(self_history, other_history).rotate_by(max_index_i);
	}

	// print internal state (scores, current best performing strategy) to cosnole
	void get_current_state() {
		std::cout << "\n\n\n----------------\nMeta Player " << name << " current scores:\n\n";
		std::cout << "[{freq, anti_rot, rot, fix}Rot0, {...}Rot1, {...}Rot2}]\n";
		for (std::vector<double> element : scores) {
			for (double score : element) {
				std::cout << score << " ";
			}
			std::cout << "  ";
		}
		std::cout << "\n\nCurrent Strategy: " << strategies[max_index_j]->get_name() << " Rotated by " << max_index_i << \
					"\n----------------\n\n\n";
	}

	// reset scores (in case of multiple Games)
	void reset_scores() {
		scores = { {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1} };
	}

	std::string get_name() override {
		return name;
	}

private:
	// Initalize "Oracle Players" Meta Player consults whenever a move decision has to be made; can be thought of as Meta Player's repertoire of strategies
	Frequency teller_freq{};
	Anti_Rotation teller_anti_rot{};
	Rotation teller_rot{ 0 };
	Random teller_rand{};
	Fixed teller_fix{ 'R' };

	// Putting the oracle Players (except Random which is called whenever scores fall below a certain threshold) into a Player pointer vector "strategies"
	std::vector<Player*> strategies{ &teller_freq, &teller_anti_rot, &teller_rot, &teller_fix};

	// 2d score array: corresponds to scores of all 4 main stratgies {Frequency, Anti_Rotation, Rotation, Fixed} rotated by 0 (first element in scores), by 1 (second element) or by 2 (third element)
	std::vector<std::vector<double>> scores{ {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1} };

	// scoring function pointer pointing to scoring function used for strategy performance evaluation
	scoring_func_ptr scoring_func;

	// scoring vector pointer passed to scoring function pointed to by scoring_func
	double* scoring_vector = new double[3];

	// these keep track of best performing strategy: used to index access highest score in scores 2d array
	int max_index_i{}, max_index_j{};

	bool verbose = false;
	std::string name = "Naive Meta Player";
};


// Meta_Player_Rand_Strat plays one of four basic strategies rotated by 0, 1 or 2 for a number of rounds; this will be used to test Meta_Player_Naive performance against switching strategies
struct Meta_Player_Rand_Strat : Player {
	Meta_Player_Rand_Strat(bool verbose = false, std::string tag = "", int init_rounds=20, int init_strat=0, int init_rot = 0) : \
		verbose{ verbose } {
		if (not (tag == "")) name = name + " " + tag;
	}

	Move get_move(const vector& other_history, const vector& self_history) {
		if (!curr_rounds) { // if curr_rounds reaches 0, generate a new strategy, rotation and rounds
			curr_rounds = mod_euc(distribution(engine), 21); // will play random strategy for number of rounds between 0 and 20
			curr_strat = mod_euc(distribution(engine), 4); // random basic strategy
			curr_rot = mod_euc(distribution(engine), 3); // random rotation between 0 and 2
		}
		else if (curr_rounds) curr_rounds -= 1; // after every get_move() call, decrement curr_rounds by 1

		return strategies[curr_strat]->get_move(other_history, self_history).rotate_by(curr_rot); // return current basic strategy rotated by current rotation
	}

	// print internal state (current basic strategy, current rotation) to console
	void get_current_state() {
		std::cout << "\n\n\n----------------\nMeta Player " << name << " current strategy:\n\n";
		std::cout << "\n\nCurrent Strategy: " << strategies[curr_strat]->get_name() << "Rotated by " << curr_rot << \
			"\n----------------\n\n\n";
	}

	std::string get_name() override {
		return name;
	}

private:
	// Random engine & distribution to randomly generate a strategy, rotation and number of rounds
	std::mt19937 engine;
	std::uniform_int_distribution<int> distribution;

	// same strategy repertoire as Meta_Player_Naive
	Frequency teller_freq{};
	Anti_Rotation teller_anti_rot{};
	Rotation teller_rot{ 0 };
	Random teller_rand{};
	Fixed teller_fix{ 'R' };
	std::vector<Player*> strategies{ &teller_freq, &teller_anti_rot, &teller_rot, &teller_fix };

	// curr_rounds is number of rounds the current basic strategy will be played for
	int curr_rounds{};

	// curr_strat is index of current basic strategy in strategies vector
	int curr_strat{};

	// curr_rot is rotation applied to current basic strategy
	int curr_rot{};

	bool verbose{};
	std::string name{ "Random Strategy Meta Player" };
};


//// Game definition

struct Game {

	// Initialize a Game with Players (always 2) and number of rounds to be played
	Game(Player& p1, Player& p2, int num_rounds, int sleep = 0) : p1{ p1 }, p2{ p2 }, num_rounds{ num_rounds }, sleep{ sleep } {};

	~Game() {
		delete[] game_history;
	}

	// Game::play() calls Players::get_move(), pushes back returned moves to respective move history array, and calls game::evaluate_game_round() for given number of rounds;
	// calls Game::evaluate_game() after all rounds have been played
	void play(bool verbose = true, bool is_1_v_1 = false) {
		win_history = {}; // set win_history to empty array (in case Game::play() is called multiple times; we only care for current win_history, not for previous Games)
		for (int i{}; i < num_rounds; i += 1) {

			Move next_move_p1 = p1.get_move(move_history_p2, move_history_p1);
			Move next_move_p2 = p2.get_move(move_history_p1, move_history_p2);

			move_history_p1.push_back(next_move_p1.index);
			move_history_p2.push_back(next_move_p2.index);

			std::cout << "\n----------------\n\nRound " << i + 1 << ": \n\n";
			if (verbose) print_last_move(); // if verbose = true; prints result of current round to console
			evaluate_game_round(next_move_p1, next_move_p2, verbose);
			std::cout << "\n----------------\n";
			if (sleep) {
				std::this_thread::sleep_for(sleep_ms);
			}

		}
		evaluate_game(verbose);
	}

	// print result of current round to console
	void print_last_move() {
		std::cout << "Player 1 (" << p1.get_name() << ") played " << shape_names[move_history_p1.back()] << "\n";
		std::cout << "Player 2 (" << p2.get_name() << ") played " << shape_names[move_history_p2.back()] << "\n\n";
	}

	// Game::evaluate_game_round() determines which Player wins current round and saves result in win_history array
	short evaluate_game_round(const Move m1, const Move m2, bool verbose = false) {

		short index_distance = evaluate_round(m1, m2); // get index distance with respect to m1

		switch (index_distance) {
		case 0: // draw
			win_history.push_back(0);
			if (verbose) {
				std::cout << "Draw!\n\n" << std::endl;
			}
			break;
		case 1: // p1 win
			win_history.push_back(1);
			if (verbose) {
				std::cout << p1.get_name() << " wins this round!\n\n" << std::endl;
			}
			break;
		case 2: // p2 win
			win_history.push_back(2);
			if (verbose) {
				std::cout << p2.get_name() << " wins this round!\n\n" << std::endl;
			}
			break;
		}
		return 0;
	}

	// After all rounds have been played, Game::evaluate_game() will determine the winner based on number of won rounds of each player in win_history array
	void evaluate_game(bool verbose = false) {
		// score{draws, p1_wins, p2_wins}

		int score[]{ 0, 0, 0 }; // score{draws, wins p1, wins p2}

		for (short element : win_history) { // add up wins of each player; draws are on win_history[0]
			score[element] += 1;
		}
		if (score[1] > score[2]) { // p1 wins game
			game_history[1] += 1;
			if (verbose) {
				std::cout << "\n\n" << p1.get_name() << " wins this game.\n";
			}
		}
		else if (score[1] < score[2]) { // p2 wins game
			game_history[2] += 1;
			if (verbose) {
				std::cout << "\n\n" << p2.get_name() << " wins this game.\n";
			}
		}
		else {
			game_history[0] += 1;
			if (verbose) { // draw
				std::cout << "\n\nThis game resulted in a draw!\n";
			}
		}

		// Calculate and put out game stats: wl is win-loss ratio; wr is win rate
		double wl1{}, wl2{}, wr1{}, wr2{};
		constexpr double inf = std::numeric_limits<double>::infinity();

		if (score[2] == 0) wl1 = inf;
		else wl1 = (double)score[1] / (double)score[2];

		if (score[1] == 0) wl2 = inf;
		else wl2 = (double)score[2] / (double)score[1];

		if (num_rounds == 0) wr1 = 0;
		else wr1 = (double)score[1] / (double)num_rounds;

		if (num_rounds == 0) wr2 = 0;
		else wr2 = (double)score[2] / (double)num_rounds;

		std::cout << "\n\n----------------\n\nGame Stats\n\n\n";
		std::cout << "Player 1 (" << p1.get_name() << ") wins " << score[1] << "/" << num_rounds << " games\n";
		std::cout << "Player 2 (" << p2.get_name() << ") wins " << score[2] << "/" << num_rounds << " games\n\n";
		std::cout << "Win-Loss Ratio " << p1.get_name() << " : " << wl1 << "\n";
		std::cout << "Win-Loss Ratio " << p2.get_name() << " : " << wl2 << "\n\n";
		std::cout << "Win Rate " << p1.get_name() << " : " << wr1*100 << "%" << "\n";
		std::cout << "Win Rate " << p2.get_name() << " : " << wr2*100 << "%" << "\n\n";
		std::cout << "With " << score[0] << " Draws" << "\n\n----------------" << std::endl;
	}

	// Save current game state to .csv-file for Analysis 
	bool save(std::string path, std::string id_tag = "") {
		try {
			path = path +  "\\" + id_tag + ".csv";
			std::ofstream ofs(path, std::ofstream::out);
			if (!ofs.is_open()) throw std::runtime_error{ "This file path is invalid, unable to open file." };

			ofs << "Move History P1,Move History P2,Win History,Game History" << "\n"; // Header
			for (int i{}; i < num_rounds+2; i += 1) {
				if (i < num_rounds) {
					ofs << move_history_p1[i] << "," << move_history_p2[i] << "," << win_history[i];
				} 

				// game_history has only 3 entries
				if (i < 3) {
					if (num_rounds <= i) ofs << ",,";
					ofs << "," << game_history[i];
				}
				ofs << "\n";
			}
			ofs.close();
		}

		catch (std::exception& e) {
			std::cout << "Error saving game data: " << e.what() << std::endl;
			return false;
		}
		return true;
	}

	// reset internal state
	void reset() {
		move_history_p1 = {};
		move_history_p2 = {};
		win_history = {};
		game_history[0] = 0;
		game_history[1] = 0;
		game_history[2] = 0;
	}

	// setter method to change number of game rounds with same Players
	void set_rounds(int rounds) {
		num_rounds = rounds;
	}

private:
	// game_history is only important when playing multiple games (calling Game::play() repeatedly): score{draws, player1 wins, player2 wins}
	int *game_history = new int[3]{ 0, 0, 0 };

	// Game Players (can be any Player derived class); Player base class serves as common interface
	Player& p1;
	Player& p2;

	// number of game rounds
	int num_rounds;

	// move histories for both players
	vector move_history_p1{};
	vector move_history_p2{};

	// win history: 0:draw  1:p1 win  2:p2 win
	vector win_history{};

	// sleep duration between rounds in ms
	int sleep{};
	std::chrono::milliseconds sleep_ms{ sleep };
};