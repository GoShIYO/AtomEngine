#pragma once
#include <random>
#include <vector>
#include <numeric>

/*
example:
	// 1 から 100 の範囲で整数を生成
	int random_int = Random::uniform(1, 10);

	// -10.5 から 10.5 の範囲で浮動小数を生成
	float random_float = Random::uniform(-10.5f, 10.5f);

	// 25%確率で true または false を返します。
	bool result = Random::bernoulli(0.25f);

	// 50.0 から 10 の範囲で正規乱数を生成
	float normal_val = Random::normal(50.f, 10.f);

	// 0.0 から 1.0 の範囲で 5 個の double をコンテナに格納
	std::vector<double> random_doubles(5);
	Random::fill<std::uniform_real_distribution<double>>(random_doubles, 0.0, 1.0);
*/

namespace AtomEngine
{
	class Random
	{
	private:

		static std::mt19937 randomEngine;

	public:
		static void Initialize(unsigned int seed = std::random_device()())
		{
			randomEngine.seed(seed);
		}

		/// <summary>
		/// 範囲を指定して乱数を生成します
		/// </summary>
		/// <typeparam name="T">型</typeparam>
		/// <param name="min">最小値</param>
		/// <param name="max">最大値</param>
		/// <returns>乱数</returns>
		template<typename T>
		static T uniform(T min, T max)
		{
			using Distribution = std::conditional_t<std::is_integral_v<T>,
				std::uniform_int_distribution<T>,
				std::uniform_real_distribution<T>>;

			return Distribution(min, max)(randomEngine);
		}

		/// <summary>
		/// [0, 1] の範囲で浮動小数乱数を生成します
		/// </summary>
		/// <returns></returns>
		static float uniform_unit()
		{
			return uniform(0.f, std::nextafter(1.f, FLT_MAX));
		}

		/// <summary>
		/// [-1, 1] の範囲で符号を含む浮動小数乱数を生成します
		/// </summary>
		/// <returns></returns>
		static float uniform_symmetry()
		{
			return uniform(-1.f, std::nextafter(1.f, FLT_MAX));
		}

		static int32_t uniform_int(int32_t max)
		{
            return uniform(0, max);
		}

		/// <summary>
		/// 指定された確率で true または false を返します。
		/// </summary>
		/// <param name="probability">確率</param>
		/// <returns></returns>
		static bool bernoulli(float probability)
		{
			return std::bernoulli_distribution(probability)(randomEngine);
		}

		/// <summary>
		/// 平均値と標準偏差に基づいて、正規分布（Gaussian）に従う乱数を生成します。
		/// </summary>
		/// <param name="mean">正規分布の平均値（μ）</param>
		/// <param name="stddev">正規分布の標準偏差（σ）</param>
		/// <returns>生成された乱数（float）</returns>
		static float normal(float mean, float stddev)
		{
			return std::normal_distribution<float>(mean, stddev)(randomEngine);
		}

		/// <summary>
		/// 任意の標準分布を使用して、指定されたコンテナに乱数を充填します。
		/// </summary>
		/// <typeparam name="Distribution">使用する乱数分布の型（例：std::uniform_real_distribution）</typeparam>
		/// <typeparam name="Range">乱数を格納するコンテナの型</typeparam>
		/// <typeparam name="Params">分布の初期化に使用するパラメータ</typeparam>
		/// <param name="range">乱数を格納する対象のコンテナ</param>
		/// <param name="params">分布の初期化パラメータ</param>
		template<typename Distribution, typename Range, typename... Params>
		static void fill(Range& range, Params&&... params)
		{
			Distribution dist(std::forward<Params>(params)...);
			std::generate(std::begin(range), std::end(range), [&]
				{
					return dist(randomEngine);
				});
		}
	};
}
