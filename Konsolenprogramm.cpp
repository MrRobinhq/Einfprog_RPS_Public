#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <limits>
#include <fstream>
#include <Pfad_zu/RPS_Header.h>


//// Game & Player config variables
int p1{}, p2{}, rounds{};
char fix_init{ 'R' };
short rot_init_1{}, rot_init_2{};
int rand_seed_1{}, rand_seed_2{};
int meta_scoring_func_1{}, meta_scoring_func_2{};
double scoring_array_1[6]{}, scoring_array_2[6]{};
double default_mul_scoring_array[6]{ 0.95, 1.1, 0.9, 1, 10, 1 };
int round_delay{};

// Bunch of control flags for program flow
bool user_flag{};
bool set_flag_rand_seed_1{}, set_flag_rand_seed_2{}, set_flag_rot_init_1{}, set_flag_rot_init_2{}, \
set_flag_meta_scoring_func_1{}, set_flag_meta_scoring_func_2{}, set_flag_scoring_array_1{}, \
set_flag_scoring_array_2{}, set_flag_user_name_1{}, set_flag_user_name_2{}, \
set_flag_no_seed_1{}, set_flag_no_seed_2{};
bool set_flag_print_rot_init{}, set_flag_print_rand_seed{}, set_flag_print_meta_scoring_func{}, set_flag_print_score_array{};

std::string user_name_1{ "" }, user_name_2{ "" };
std::string save_path{}, f_name{};
const std::vector<std::string> scoring_funcs{ "multiplicative", "additive", "multiplicative drop switch", "additive drop switch", "default multiplicative" };




// flush_cin() clears iostream stream buffer in case of bad input
void flush_cin() {
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// input_exception_handler() wraps exception handling and stream buffer clearing around std::cin >> target_variable 
template<typename T>
T input_exception_handler(std::string err_msg = "Invalid input!") {
	T target{};
	while (true) {
		try {
			std::cin >> target;
			if (!std::cin) throw std::runtime_error{ err_msg };
		}
		catch (std::exception& e) {
			std::cout << "\nError: " << e.what() << "\n\n";
			flush_cin();
			continue;
		}
		flush_cin();
		break;
	}
	return target;
}


// Sets up player variables based on user input
void configure_player(int p) {
	auto scoring_array_ptr = (set_flag_scoring_array_1 ? scoring_array_2 : scoring_array_1);// points to first or second scoring array based on truth value of set_flag_scoring_array_1
																						    // --> will choose second scoring array if first has already been set
	switch (p) {
	case 0: // Fixed Player
		std::cout << "\n\nChoose Fixed Player move shape (R for Rock, P for Paper, S for Scissors): ";
		while (true) {
			try {
				fix_init = input_exception_handler<char>();
				if (!(fix_init == 'R' or fix_init == 'P' or fix_init == 'S')) throw std::domain_error{ "Fixed shape has to be R, P or S!" };
			}
			catch (std::exception& e) {
				std::cout << "\nError: " << e.what() << "\n\n";
				continue;
			}
			break;
		}
		break;
	case 1: // Rotation Player
		std::cout << "\n\nChoose Rotation Player rotation (0-2; will use Euclidian modulo for negative or larger than 2 numbers): ";
		if (!set_flag_rot_init_1) { // if first rotation player
			rot_init_1 = input_exception_handler<short>();
			set_flag_rot_init_1 = true;
		}
		else { // if second rotation player
			rot_init_2 = input_exception_handler<short>();
			set_flag_rot_init_2 = true;
		}
		break;
	case 2: // Frequency Player
		break;
	case 3: // Anti Rotation Player
		break;
	case 4: // Human Player
		std::cout << "\nYour name: ";
		if (!set_flag_user_name_1) { // if first human player
			user_name_1 = input_exception_handler<std::string>();
			set_flag_user_name_1 = true;
		}
		else { // if second human player
			user_name_2 = input_exception_handler<std::string>();
			set_flag_user_name_2 = true;
		}
		break;
	case 5: // Random Player
		std::cout << "\nDo you want to use a seed for Random Player's RNG (1=yes, 0=no)? ";
		user_flag = input_exception_handler<bool>();

		if (user_flag) {// if seed is put in
			std::cout << "\nSeed: ";
			if (!set_flag_rand_seed_1) { // if first random player
				rand_seed_1 = input_exception_handler<int>();
				set_flag_rand_seed_1 = true;
			}
			else { // if second random player
				rand_seed_2 = input_exception_handler<int>();
				set_flag_rand_seed_2 = true;
			}
		}
		else {// if no seed is put in
			if (!set_flag_rand_seed_1) { // if first random player
				set_flag_no_seed_1 = true; // random player will be initialised without seed -> pseudo random seed
				set_flag_rand_seed_1 = true;
			}
			else { // if second random player
				set_flag_no_seed_2 = true; // random player will be initialised without seed -> pseudo random seed
				set_flag_rand_seed_2 = true;
			}
		}
		break;
	case 6: // Meta Player
		std::cout << "\n\nMeta Player needs a scoring funtion, and scoring function needs a scoring vector. Choose from the following scoring functions:\n\n";
		std::cout << "(0) Multiplicative scoring function: Multiplies Meta Player scores by values in scoring vector\n";
		std::cout << "(1) Additive scoring function: Adds values in scoring vector to Meta Player scores\n";
		std::cout << "(2) Multiplicative drop switch: Multipies Meta Player scores by values in scoring vector in case of win or draw; in case of loss sets score to 3rd value in scoring vector\n";
		std::cout << "(3) Additive drop switch: Adds values in scoring vector to Meta Player scores in case of win or draw; in case of loss sets score to 3rd value in scoring vector\n";
		std::cout << "(4) Default scoring function: multiplicative with scoring vector {0.95 (draw), 1.1 (win), 0.9 (loss), 1 (floor), 10 (ceil), 1 (decay)}\n\n";
		std::cout << "Choose one of the scoring functions above or put in 4 for default scoring function: ";
		while (true) {
			try {
				if (!set_flag_meta_scoring_func_1) { // if first meta player
					meta_scoring_func_1 = input_exception_handler<int>();
					if (meta_scoring_func_1 < 0 or meta_scoring_func_1 > 4) throw std::range_error{ "You have to choose between 0 and 4!" };
					set_flag_meta_scoring_func_1 = true;
				}
				else { // if second meta player
					meta_scoring_func_2 = input_exception_handler<int>();
					set_flag_meta_scoring_func_2 = true;
				}
				if (meta_scoring_func_2 < 0 or meta_scoring_func_2 > 4) throw std::range_error{ "You have to choose between 0 and 4!" };
			}
			catch (std::exception& e) {
				std::cout << "\nError: " << e.what() << "\n\n";
				continue;
			}
			break;
		}
		switch ((set_flag_meta_scoring_func_2 ? meta_scoring_func_2 : meta_scoring_func_1)) { // choosing scoring function
		case 0: // Multiplicative Scoring Function
			std::cout << "\n\nMultiplicative scoring function needs a scoring vector.\n";
			std::cout << "You will successively put in the constants that build up the scoring vector:\n\n";
			std::cout << "Set Draw Multiplier (score of any meta strategy will be multiplied by this value in case of draw): ";
			*(scoring_array_ptr + 0) = input_exception_handler<double>(); // write either to scoring array_1 or scoring_array_2
			std::cout << "Set Win Multiplier (score of any meta strategy will be multiplied by this value in case of win): ";
			*(scoring_array_ptr + 1) = input_exception_handler<double>();
			std::cout << "Set Loss Multiplier (score of any meta strategy will be multiplied by this value in case of loss): ";
			*(scoring_array_ptr + 2) = input_exception_handler<double>();
			std::cout << "Set score floor (score of any meta strategy can never be smaller than this value): ";
			*(scoring_array_ptr + 3) = input_exception_handler<double>();
			std::cout << "Set score ceiling (score of any meta strategy can never be larger than this value): ";
			*(scoring_array_ptr + 4) = input_exception_handler<double>();
			std::cout << "Set score decay (every score will get multiplied by this constant regardless of outcome): ";
			*(scoring_array_ptr + 5) = input_exception_handler<double>();
			if (!set_flag_scoring_array_1) set_flag_scoring_array_1 = true;
			else set_flag_scoring_array_2 = true;
			break;
		case 1: // Additive scoring function
			std::cout << "\n\nAdditive scoring function needs a scoring vector.\n";
			std::cout << "You will successively put in the constants that build up the scoring vector:\n\n";
			std::cout << "Set Draw Constant (this value will be added to score of any meta strategy in case of draw): ";
			*(scoring_array_ptr + 0) = input_exception_handler<double>();
			std::cout << "Set Win Constant (this value will be added to score of any meta strategy in case of win): ";
			*(scoring_array_ptr + 1) = input_exception_handler<double>();
			std::cout << "Set Loss Constant (this value will be added to score of any meta strategy in case of loss): ";
			*(scoring_array_ptr + 2) = input_exception_handler<double>();
			std::cout << "Set score floor (score of any meta strategy can never be smaller than this value): ";
			*(scoring_array_ptr + 3) = input_exception_handler<double>();
			std::cout << "Set score ceiling (score of any meta strategy can never be larger than this value): ";
			*(scoring_array_ptr + 4) = input_exception_handler<double>();
			std::cout << "Set score decay (every score will get multiplied by this constant regardless of outcome): ";
			*(scoring_array_ptr + 5) = input_exception_handler<double>();
			if (!set_flag_scoring_array_1) set_flag_scoring_array_1 = true;
			else set_flag_scoring_array_2 = true;
			break;
		case 2: // Multiplicative drop switch
			std::cout << "\n\nMultiplicative drop switch needs a scoring vector.\n";
			std::cout << "You will successively put in the constants that build up the scoring vector:\n\n";
			std::cout << "Set Draw Multiplier (score of any meta strategy will be multiplied by this value in case of draw): ";
			*(scoring_array_ptr + 0) = input_exception_handler<double>();
			std::cout << "Set Win Multiplier (score of any meta strategy will be multiplied by this value in case of win): ";
			*(scoring_array_ptr + 1) = input_exception_handler<double>();
			std::cout << "Set Loss Value (score of any meta strategy will be set to this value in case of loss): ";
			*(scoring_array_ptr + 2) = input_exception_handler<double>();
			std::cout << "Set score floor (score of any meta strategy can never be smaller than this value): ";
			*(scoring_array_ptr + 3) = input_exception_handler<double>();
			std::cout << "Set score ceiling (score of any meta strategy can never be larger than this value): ";
			*(scoring_array_ptr + 4) = input_exception_handler<double>();
			std::cout << "Set score decay (every score will get multiplied by this constant regardless of outcome): ";
			*(scoring_array_ptr + 5) = input_exception_handler<double>();
			if (!set_flag_scoring_array_1) set_flag_scoring_array_1 = true;
			else set_flag_scoring_array_2 = true;
			break;
		case 3: // Additive drop switch
			std::cout << "\n\nAdditive drop switch needs a scoring vector.\n";
			std::cout << "You will successively put in the constants that build up the scoring vector:\n\n";
			std::cout << "Set Draw Constant (this value will be added to score of any meta strategy in case of draw): ";
			*(scoring_array_ptr + 0) = input_exception_handler<double>();
			std::cout << "Set Win Constant (this value will be added to score of any meta strategy in case of win): ";
			*(scoring_array_ptr + 1) = input_exception_handler<double>();
			std::cout << "Set Loss Constant (score of any meta strategy will be set to this value in case of loss): ";
			*(scoring_array_ptr + 2) = input_exception_handler<double>();
			std::cout << "Set score floor (score of any meta strategy can never be smaller than this value): ";
			*(scoring_array_ptr + 3) = input_exception_handler<double>();
			std::cout << "Set score ceiling (score of any meta strategy can never be larger than this value): ";
			*(scoring_array_ptr + 4) = input_exception_handler<double>();
			std::cout << "Set score decay (every score will get multiplied by this constant regardless of outcome): ";
			*(scoring_array_ptr + 5) = input_exception_handler<double>();
			if (!set_flag_scoring_array_1) set_flag_scoring_array_1 = true;
			else set_flag_scoring_array_2 = true;
			break;
		case 4: // Default Multiplicative Scoring Function (uses default scoring array)
			for (int i{}; i < 6; i += 1) {
				*(scoring_array_ptr + i) = default_mul_scoring_array[i]; // copy default scoring array to current array (if default was chosen); just for printing
																		 // because if default, random player will be initialised with default scoring array, 
																		 // not with scoring array_1 or scoring_array_2
			}
			if (!set_flag_scoring_array_1) set_flag_scoring_array_1 = true;
			else set_flag_scoring_array_2 = true;
			break;
		}
		break;
	case 7: // Random Strategy Player
		break;
	}
}

// Prints current player setup after configuration is complete but before playing the game; main() gives option to reconfigure
void print_setup(int p, int p_num, Player* player) {
	std::cout << "Player " << p_num << " : ";
	auto scoring_array_ptr = (set_flag_print_score_array ? scoring_array_2 : scoring_array_1);

	switch (p) {
	case 0: // Fixed Player
		std::cout << player->get_name();
		break;
	case 1: // Rotation Player
		std::cout << player->get_name();
		std::cout << " (Rotation by " << (set_flag_print_rot_init ? rot_init_2 : rot_init_1) << ")\n";
		set_flag_print_rot_init = true;
		break;
	case 2: // Frequency Player
		std::cout << player->get_name();
		break;
	case 3: // Anti Rotation Player
		std::cout << player->get_name();
		break;
	case 4: // Human Player
		std::cout << player->get_name();
		break;
	case 5: // Random Player
		std::cout << player->get_name();
		if (!set_flag_print_rand_seed and set_flag_no_seed_1) {
			std::cout << " (no seed)";
		}
		else if (set_flag_print_rand_seed and set_flag_no_seed_2) {
			std::cout << " (no seed)";
		}
		else {
			std::cout << " (Seed: " << (set_flag_print_rand_seed ? rand_seed_2 : rand_seed_1) << ")";
		}
		set_flag_print_rand_seed = true;
		break;
	case 6: // Meta Player
		std::cout << player->get_name();
		std::cout << " (scoring array {";
		for (int i{}; i < 6; i += 1) { // print scoring array associated with meta player
			std::cout << *(scoring_array_ptr + i) << " ";
		}
		std::cout << "})\n";
		set_flag_print_meta_scoring_func = true;
		set_flag_print_score_array = true;
		break;
	case 7: // Random Strategy Player
		std::cout << player->get_name();
		break;
	}

}

// Initializes Player objects based on configuration
Player* setup_player(int p) {
	Player* player{};
	auto scoring_array_ptr = (set_flag_scoring_array_2 ? &scoring_array_2 : &scoring_array_1);
	switch (p) {
	case 0: // Fixed Player
		player = new Fixed{ fix_init };
		break;
	case 1: // Rotation Player
		if (!set_flag_rot_init_2) player = new Rotation{ rot_init_1 };
		else player = new Rotation{ rot_init_2 };
		break;
	case 2: // Frequency Player
		player = new Frequency{};
		break;
	case 3: // Anti Rotation Player
		player = new Anti_Rotation{};
		break;
	case 4: // Human Player
		if (!set_flag_user_name_2) player = new Human{ user_name_1 };
		else player = new Human{ user_name_2 };
		break;
	case 5: // Random Player
		if (!set_flag_rand_seed_2 and set_flag_no_seed_1) {
			player = new Random{};
		}
		else if (set_flag_rand_seed_2 and set_flag_no_seed_2) {
			player = new Random{};
		}
		else if (!set_flag_rand_seed_2 and !set_flag_no_seed_1) {
			player = new Random{ rand_seed_1 };
		}
		else if (set_flag_rand_seed_2 and !set_flag_no_seed_2) {
			player = new Random{ rand_seed_2 };
		}
		break;
	case 6: // Meta Player
		switch ((set_flag_meta_scoring_func_2 ? meta_scoring_func_2 : meta_scoring_func_1)) {
		case 0: // Multiplicative Scoring Function
			player = new Meta_Player_Naive{ false, scoring_funcs[0], naive_score_mul, *scoring_array_ptr };
			break;
		case 1: // Additive scoring function
			player = new Meta_Player_Naive{ false, scoring_funcs[1], naive_score_add, *scoring_array_ptr };
			break;
		case 2: // Multiplicative drop switch
			player = new Meta_Player_Naive{ false, scoring_funcs[2], drop_switch_mul, *scoring_array_ptr };
			break;
		case 3: // Additive drop switch
			player = new Meta_Player_Naive{ false, scoring_funcs[3], drop_switch_add, *scoring_array_ptr };
			break;
		case 4: // Default Multiplicative Scoring Function (uses default scoring array)
			player = new Meta_Player_Naive{ false, scoring_funcs[4], naive_score_mul, default_mul_scoring_array };
			break;
		}
		break;
	case 7: // Random Strategy Player
		player = new Meta_Player_Rand_Strat{ false };
		break;
	}
	return player;
}

// Resets all control flags before a new game is played within the same console session; is called by main()
void reset_config() {
	set_flag_rand_seed_1 = false; set_flag_rand_seed_2 = false; set_flag_rot_init_1 = false; set_flag_rot_init_2 = false; \
		set_flag_meta_scoring_func_1 = false; set_flag_meta_scoring_func_2 = false; set_flag_scoring_array_1 = false; \
		set_flag_scoring_array_2 = false; set_flag_user_name_1 = false; set_flag_user_name_2 = false;
		set_flag_print_rot_init = false; set_flag_print_rand_seed = false; set_flag_print_meta_scoring_func = false; \
		set_flag_print_score_array = false; set_flag_no_seed_1 = false; set_flag_no_seed_2 = false;

}

//// Main
int main() {

	std::cout << "==================================================\n";
	std::cout << "== Welcome to Rock-Paper-Scissors engine v.1.0! ==\n";
	std::cout << "==================================================\n\n\n";

	while (true) {

		Player* player1{}, * player2{};

		while (true) {
			std::cout << "\n\nFirst, you need to choose two players to play against each other from the following selection:\n\n";
			std::cout << "(0) Fixed Player: Always plays the same move (Rock, Paper or Scissors)\n";
			std::cout << "(1) Rotation Player: Rotates last opponent Move by set amount \n     (Rotation is clockwise for positive and counter clockwise for negative rotation values)\n";
			std::cout << "(2) Frequency Player: Plays winning move against most frequently played\n     opponent move; always beats fixed player\n";
			std::cout << "(3) Anti Rotation Player: Assuming opponent is a rotation player,\n     figures out opponent rotation and plays winning move against it\n";
			std::cout << "(4) Human Player: Plays whatever you tell it to; your chance to prove\n     our carbon-based superiority!\n";
			std::cout << "(5) Random Player: Plays uniformly distributed random moves;\n     unpredictable (except if you know the seed)!\n";
			std::cout << "(6) Meta Player: Smart player that chooses the best strategy\n     against its opponent; will win against Players 1-3 and 7\n";
			std::cout << "(7) Random Strategy Player: Chooses between strategies 0-3 and applies\n     a rotation between 0 and 2; frequently switches strategy and rotation\n";
			std::cout << "\nChoose Strategy (0-7): ";

			while (true) { // Choose Player 1
				try {
					p1 = input_exception_handler<int>();
					if (p1 < 0 or p1 > 7) throw std::domain_error{ "You have to choose between Strategy 0 and 7!" };
				}
				catch (std::exception& e) {
					std::cout << "\nError: " << e.what() << "\n\n";
					continue;
				}
				break;
			}

			// Depending on player type, different initialization variables are needed, so call configure_player(), then setup_player()
			configure_player(p1);
			player1 = setup_player(p1);

			while (true) { // Choose Player 2
				std::cout << "\n\nChoose Player 2 (0-7): ";
				try {
					p2 = input_exception_handler<int>();
					if (p2 > 7 or p2 < 0) throw std::range_error{ "You have to choose between Strategy 0 and 7!" };
				}
				catch (std::exception& e) {
					std::cout << "\nError: " << e.what() << "\n\n";
					continue;
				}
				break;
			}

			configure_player(p2);
			player2 = setup_player(p2);


			while (true) { // Set number of game rounds
				std::cout << "\n\nChoose number of rounds to play: ";
				try {
					rounds = input_exception_handler<int>();
					if (rounds < 0) throw std::domain_error{ "Number of rounds has to be larger than 0!" };
				}
				catch (std::exception& e) {
					std::cout << "\nError: " << e.what() << "\n\n";
					continue;
				}
				break;
			}

			// Print final setup
			std::cout << "\n\n----------------\n\n";
			std::cout << "Your game set up:\n\n\n";
			print_setup(p1, 1, player1);
			std::cout << "\n\n            vs            \n\n";
			print_setup(p2, 2, player2);
			std::cout << "\n\n\nPlaying " << rounds << " rounds";
			std::cout << "\n\n----------------\n\n";
			std::cout << "Do you want to continue with this setup or reconfigure your settings (0: reconfigure, 1: continue)? ";

			// Choice to reconfigure or play
			user_flag = input_exception_handler<bool>();

			if (!user_flag) {
				delete player1, player2;
				continue;
			}
			else break;
		}

		std::cout << "\n\nBefore starting the game, you can set a delay in milliseconds between \ngame rounds";
		std::cout << " (not recommended for large number of rounds; set to 0 for no extra delay): ";

		while (true) { // Set delay
			try {
				round_delay = input_exception_handler<int>();
				if (round_delay < 0) throw std::domain_error{ "Delay has to be larger than 0!" };
			}
			catch (std::exception& e) {
				std::cout << "Error: " << e.what() << std::endl;
				continue;
			}
			break;
		}

		Game this_game{ *player1, *player2, rounds , round_delay }; // Init Game


		std::cout << "\n\n----------------\n----------------\n\nGame starts!\n\n";

		this_game.play(); // Play

		delete player1, player2; // Delete dynamic player objects

		// choice to save game data
		std::cout << "\n\nDo you want to save the current win and move histories to a file (0=no, 1=yes)? ";
		user_flag = input_exception_handler<bool>();

		if (user_flag) { // Save game
			while (true) {
				try {
					std::cout << "\n\nSpecify a path to save game data: ";
					save_path = input_exception_handler<std::string>();
					std::cout << "\nFile name (file extension .csv is automatic): ";
					f_name = input_exception_handler<std::string>();

					std::cout << save_path;
					if (this_game.save(save_path, f_name)) break;
					else throw std::runtime_error{ "Something went wrong while saving your game data." };
				}
				catch (std::exception& e) {
					std::cout << "\nAn Error occured while saving game data: " << e.what();
					std::cout << "\nTry another path and make sure to only include alphanumeric characters or cancel (0: cancel, 1: try again): ";
					user_flag = input_exception_handler<bool>();
					if (user_flag) continue;
					else break;
				}
				break;
			}
			std::cout << "\n\nSuccessfully saved game data to " << save_path + "\\" + f_name + ".csv" << std::endl;

		}

		// Exit or continue with new game
		std::cout << "\n\n----------------\n\nDo you want to set up another game or exit (0: exit; 1: new game)? ";

		user_flag = input_exception_handler<bool>();

		if (!user_flag) {
			break;
		}
		else {
			reset_config(); // reset control flags for next game
			continue;
		}
	}

	return 0;

}