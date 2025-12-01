#include "Json.h"
#include <map>
#include <fstream>
#include <filesystem>

namespace AtomEngine
{
	namespace fs = std::filesystem;

	void Json::AddItem(const std::string& genre, const json& item)
	{
		mData[genre] = item;
	}
	json Json::GetItem(const json& genre)
	{
		auto it = mData.find(genre);
		if (it != mData.end())
		{
			return it->second;
		}
		return json();
	}
	bool Json::Save(const std::string& file)
	{
		fs::path pathObj(file);
		fs::path dir = pathObj.parent_path();

		// ディレクトリが指定されている場合は作成
		if (!dir.empty() && !fs::exists(dir))
		{
			fs::create_directories(dir);
		}

		// JSON保存
		json j;
		for (const auto& [genre, content] : mData)
		{
			j[genre] = content;
		}
		std::ofstream f(file);
		f << j.dump(4);
		if (!f.good())
		{
			return false;
		}

		return true;
	}

	bool Json::Load(const std::string& file)
	{
		std::ifstream f(file);

		if (!f.is_open())return false;

		json j;
		f >> j;
		mData.clear();

		for (auto& [genre, items] : j.items())
		{
			mData[genre] = items;
		}

		return true;
	}
}