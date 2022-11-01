/**
 * Framework for Threes! and its variants (C++ 11)
 * weight.h: Lookup table template for n-tuple network
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <iostream>
#include <vector>
#include <utility>

class weight {
public:
	typedef float type;

public:
	weight() {}
	// weight(size_t len) : value(len) {}
	// weight(weight&& f) : value(std::move(f.value)) {}
	// weight(const weight& f) = default;
	weight(const std::vector<int> &patn) : pattern(patn)
	{
		// 1*2^(size*2^2) = 2^(size*4) = 16^size
		size_t value_size = 1 << (patn.size() << 2);
		value = std::vector<type>(value_size, 0);
		for(int iso_type = 0; iso_type <= 7 ; iso_type++)
		{
			std::vector<int> iso_patn;
			for(int position : pattern)
			{
				iso_patn.emplace_back(gen_isomorphism(position)[iso_type]);
			}
			isomorphism.emplace_back(iso_patn);
		}
	}

	weight& operator =(const weight& f) = default;
	type& operator[] (size_t i) { return value[i]; }
	const type& operator[] (size_t i) const { return value[i]; }
	size_t size() const { return value.size(); }

public:

	float estimate_value(const board&b) const
	{
		float sum = 0;
		for(auto &iso_patn : isomorphism)
		{
			int value_index = 0;
			for (size_t i = 0; i < iso_patn.size(); i++)
			{
				int board_value = b(iso_patn[i]);
				value_index |= board_value << (i << 2);
			}
			sum += value[value_index];
		}
		return sum;
	}

	float update(const board &b, float delta)
	{
		// std::cout<<"iso_size "<<isomorphism.size()<<" ";
		float sum = 0;
		for(auto &iso_patn : isomorphism )
		{
			int value_index = 0;
			for(size_t i = 0; i < iso_patn.size(); i++)
			{
				int board_value = b(iso_patn[i]);
				value_index |= board_value << (i << 2); 
			}
			value[value_index] += delta;
			sum += value[value_index];
		}
		return sum;
	}

	friend std::ostream& operator <<(std::ostream& out, const weight& w) {
		auto& value = w.value;
		uint64_t size = value.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
		out.write(reinterpret_cast<const char*>(value.data()), sizeof(type) * size);
		return out;
	}
	friend std::istream& operator >>(std::istream& in, weight& w) {
		auto& value = w.value;
		uint64_t size = 0;
		in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
		value.resize(size);
		in.read(reinterpret_cast<char*>(value.data()), sizeof(type) * size);
		return in;
	}

	std::vector<int> gen_isomorphism(int position)
	{
		switch(position){
			case 0:
				return std::vector<int>{0, 3, 15, 12, 3, 15, 12, 0};
			case 1:
				return std::vector<int>{1, 7, 14, 8, 2, 11, 13, 4};
			case 2:
				return std::vector<int>{2, 11, 13, 4, 1, 7, 14, 8};
			case 3:
				return std::vector<int>{3, 15, 12, 0, 0, 3, 15, 12};
			case 4:
				return std::vector<int>{4, 2, 11, 13, 7, 14, 8, 1};
			case 5:
				return std::vector<int>{5, 6, 10, 9, 6, 10, 9, 5};
			case 6:
				return std::vector<int>{6, 10, 9, 5, 5, 6, 10, 9};
			case 7:
				return std::vector<int>{7, 14, 8, 1, 4, 2, 11, 13};
			case 8:
				return std::vector<int>{8, 1, 7, 14, 11, 13, 4, 2};
			case 9:
				return std::vector<int>{9, 5, 6, 10, 10, 9, 5, 6};
			case 10:
				return std::vector<int>{10, 9, 5, 6, 9, 5, 6, 10};
			case 11:
				return std::vector<int>{11, 13, 4, 2, 8, 1, 7, 14};
			case 12:
				return std::vector<int>{12, 0, 3, 15, 15, 12, 0, 3};
			case 13:
				return std::vector<int>{13, 4, 2, 11, 14, 8, 1, 7};
			case 14:
				return std::vector<int>{14, 8, 1, 7, 13, 4, 2, 11};
			case 15:
				return std::vector<int>{15, 12, 0, 3, 12, 0, 3, 15};
		}
		return std::vector<int>();
	}

protected:
	std::vector<type> value;
	std::vector<std::vector<int>> isomorphism;
	std::vector<int> pattern;
};
