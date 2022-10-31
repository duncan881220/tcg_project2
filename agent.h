/**
 * Framework for Threes! and its variants (C++ 11)
 * agent.h: Define the behavior of variants of agents including players and environments
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include "board.h"
#include "action.h"
#include "weight.h"

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

/**
 * base agent for agents with randomness
 */
class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * base agent for agents with weight tables and a learning rate
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), alpha(0) {
		// if (meta.find("init") != meta.end())
		// 	init_weights(meta["init"]);
		if (meta.find("load") != meta.end())
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end())
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
		std::string res = info; // comma-separated sizes, e.g., "65536,65536"
		for (char& ch : res)
			if (!std::isdigit(ch)) ch = ' ';
		std::stringstream in(res);
		for (size_t size; in >> size; net.emplace_back(size));
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}

protected:
	std::vector<weight> net;
	float alpha;
};

class TD_slider : public weight_agent
{
public:
	TD_slider(const std::string& args = "") : weight_agent("name=TD role=slider" + args),
		opcode({0, 1, 2, 3})
		{
			state_record.reserve(20000000);
			if (meta.find("init") != meta.end())
				init_weights(meta["init"]);

		}
	virtual void init_weights(const std::string& info) 
	{
        net.emplace_back(weight({0, 1, 2, 3, 4, 5}));
        net.emplace_back(weight({4, 5, 6, 7, 8, 9}));
        net.emplace_back(weight({0, 1, 2, 4, 5, 6}));
        net.emplace_back(weight({4, 5, 6, 8, 9, 10}));
        std::cout<<"initial weights"<<std::endl;
    }

	virtual action take_action(const board& before)
	{
		std::vector<board> after_boards;
		std::vector<board::reward> rewards;

		int max_op = -1;
		board::reward best_reward = INT_MIN;
		board::reward best_slide_reward;
		for(int op : opcode)
		{
			board::reward slide_reward;
			board::reward estimate_value;
			after_boards.emplace_back(board(before));
			slide_reward =after_boards[op].slide(op);
			if(slide_reward != -1)
			{
				estimate_value = estimate_board(after_boards[op]);
				if(estimate_value + slide_reward > best_reward)
				{
					best_reward = estimate_value + slide_reward;
					max_op = op;
					best_slide_reward = slide_reward;
				}
			}
		}
		if(max_op != -1)
		{
			state board_state;
			board_state.before = before;
			board_state.after = after_boards[max_op];
			board_state.op = max_op;
			board_state.reward = best_slide_reward;
			board_state.value = best_reward;
			return action::slide(max_op);
		}
		return action();
	}

	void episode_update()
	{
		float reward_afterstate_sum = 0;
		for(state_record.pop_back(); state_record.size(); state_record.pop_back())
		{
			state& board_act = state_record.back();
			float diff = reward_afterstate_sum - (board_act.value - board_act.reward); // after is B, value - reward = V(B)
			reward_afterstate_sum = board_act.reward + weight_update(board_act.after, alpha * diff);
		}
	}

	float estimate_board(const board& b) const 
	{
		float value = 0;
		for (auto& w : net)
		{
            value += w.estimate_value(b);
        }
        return value;
	}

	float weight_update(const board& b, float diff)
	{
		float delta = diff / net.size();
		float value = 0;
		for (auto& patn : net)
		{
			value += patn.update(b, delta);
		}
		return value;
	}

private:
	struct state
	{
		board before, after;
		int op;
		board::reward reward, value;
	};
	std::vector<state> state_record;
	std::array<int, 4> opcode;
};

/**
 * default random environment, i.e., placer
 * place the hint tile and decide a new hint tile
 */
class random_placer : public random_agent {
public:
	random_placer(const std::string& args = "") : random_agent("name=place role=placer " + args) {
		spaces[0] = { 12, 13, 14, 15 };
		spaces[1] = { 0, 4, 8, 12 };
		spaces[2] = { 0, 1, 2, 3};
		spaces[3] = { 3, 7, 11, 15 };
		spaces[4] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	}

	virtual action take_action(const board& after) {
		std::vector<int> space = spaces[after.last()];
		std::shuffle(space.begin(), space.end(), engine);
		for (int pos : space) {
			if (after(pos) != 0) continue;

			int bag[3], num = 0;
			for (board::cell t = 1; t <= 3; t++)
				for (size_t i = 0; i < after.bag(t); i++)
					bag[num++] = t;
			std::shuffle(bag, bag + num, engine);

			board::cell tile = after.hint() ?: bag[--num];
			board::cell hint = bag[--num];

			return action::place(pos, tile, hint);
		}
		return action();
	}

private:
	std::vector<int> spaces[5];
};

/**
 * random player, i.e., slider
 * select a legal action randomly
 */
class random_slider : public random_agent {
public:
	random_slider(const std::string& args = "") : random_agent("name=slide role=slider " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};

class greedy_slider : public random_agent {
public:
	greedy_slider(const std::string& args = "") : random_agent("name=slide role=slider " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {

		board::reward max_reward = -1;
		int max_op = -1;
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if(reward > max_reward)
			{
				max_op = op;
				max_reward = reward;
			}
		}
		if(max_op != -1)
		{
			return action::slide(max_op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};

class bottom_slider : public random_agent {
public:
	bottom_slider(const std::string& args = "") : random_agent("name=slide role=slider " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {

		board::reward max_reward = -1;
		int max_op = -1;
		

		for(int i = 0; i <= 2; i++)
		{
			int op = i+1;
			board::reward reward = board(before).slide(op);
			if(reward > max_reward)
			{
				max_op = op;
				max_reward = reward;
			}

		}
		if(max_op != -1)
		{
			
			return action::slide(max_op);
		}

		board::reward reward = board(before).slide(0);
		if(reward != -1)
		{
			return action::slide(0);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};