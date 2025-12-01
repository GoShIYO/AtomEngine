#pragma once
#include <string>
#include <vector>
#include "json.hpp"

namespace AtomEngine
{
	using json = nlohmann::json;
	class Json
	{
	public:

		/// <summary>
		/// ジャンルにアイテムを追加
		/// </summary>
		/// <param name="genre">ジャンル</param>
		/// <param name="item">書き込めデータ</param>
		void AddItem(const std::string& genre, const json& item);
		/// <summary>
		/// ジャンルからアイテムを取得
		/// </summary>
		/// <param name="genre">ジャンルの名前</param>
		/// <returns></returns>
		json GetItem(const json& genre);
		//セーブ
		bool Save(const std::string& file);
		//ロード
		bool Load(const std::string& file);

	private:
		 std::map<std::string, json> mData;
	};
}


