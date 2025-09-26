#pragma once
#include "../D3dUtility/d3dInclude.h"

namespace AtomEngine
{
	class RootParameter
	{
		friend class RootSignature;
	public:

		RootParameter()
		{
			//ルートパラメータのタイプを初期化
			mRootParameter.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xffffffff;
		}
		~RootParameter() {}

		// ルートパラメータをクリア
		void Clear()
		{
			//ルートパラメータのタイプをクリア
			if (mRootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				// デスクリプタテーブルの範囲を解放
				delete[] mRootParameter.DescriptorTable.pDescriptorRanges;

				mRootParameter.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xffffffff;
			}
		}

		void InitAsConstants(UINT Register, UINT NumDwords, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
		{
			mRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			mRootParameter.ShaderVisibility = Visibility;
			mRootParameter.Constants.Num32BitValues = NumDwords;
			mRootParameter.Constants.ShaderRegister = Register;
			mRootParameter.Constants.RegisterSpace = Space;
		}
		/// <summary>
		/// 定数バッファとしてルートパラメータを初期化します。
		/// </summary>
		/// <param name="Register">バインドするシェーダーレジスタ番号。</param>
		/// <param name="Visibility">この定数バッファを可視化するシェーダーステージ。デフォルトはすべてのシェーダーステージ（D3D12_SHADER_VISIBILITY_ALL）です。</param>
		/// <param name="Space">レジスタスペース番号。デフォルトは0です。</param>
		void InitAsConstantBuffer(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
		{
			mRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			mRootParameter.ShaderVisibility = Visibility;
			mRootParameter.Descriptor.ShaderRegister = Register;
			mRootParameter.Descriptor.RegisterSpace = Space;
		}
		/// <summary>
		/// バッファ型シェーダーリソースビュー (SRV) としてルートパラメータを初期化します。
		/// </summary>
		/// <param name="Register">SRV をバインドするシェーダーレジスタ番号。</param>
		/// <param name="Visibility">このパラメータが可視となるシェーダーステージ。デフォルトはすべてのシェーダーステージ (D3D12_SHADER_VISIBILITY_ALL)。</param>
		/// <param name="Space">レジスタスペース番号。デフォルトは 0。</param>
		void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
		{
			mRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			mRootParameter.ShaderVisibility = Visibility;
			mRootParameter.Descriptor.ShaderRegister = Register;
			mRootParameter.Descriptor.RegisterSpace = Space;
		}
		/// <summary>
		/// バッファーUAV（アンオーダードアクセスビュー）としてルートパラメータを初期化します。
		/// </summary>
		/// <param name="Register">シェーダーレジスタ番号を指定します。</param>
		/// <param name="Visibility">このルートパラメータが可視となるシェーダーステージを指定します。デフォルトはD3D12_SHADER_VISIBILITY_ALLです。</param>
		/// <param name="Space">レジスタスペースを指定します。デフォルトは0です。</param>
		void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
		{
			mRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			mRootParameter.ShaderVisibility = Visibility;
			mRootParameter.Descriptor.ShaderRegister = Register;
			mRootParameter.Descriptor.RegisterSpace = Space;
		}
		/// <summary>
		/// D3D12のディスクリプタレンジとして初期化します。
		/// </summary>
		/// <param name="Type">ディスクリプタレンジの種類（D3D12_DESCRIPTOR_RANGE_TYPE）。</param>
		/// <param name="Register">ディスクリプタレンジの開始レジスタ番号。</param>
		/// <param name="Count">ディスクリプタレンジ内のディスクリプタ数。</param>
		/// <param name="Visibility">このレンジが可視となるシェーダーステージ（デフォルトはD3D12_SHADER_VISIBILITY_ALL）。</param>
		/// <param name="Space">レジスタスペース番号（デフォルトは0）。</param>
		void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
		{
			InitAsDescriptorTable(1, Visibility);
			SetTableRange(0, Type, Register, Count, Space);
		}
		/// <summary>
		/// 指定された範囲数とシェーダーの可視性でディスクリプタテーブルとして初期化します。
		/// </summary>
		/// <param name="RangeCount">ディスクリプタレンジの数。</param>
		/// <param name="Visibility">シェーダーの可視性。デフォルトは D3D12_SHADER_VISIBILITY_ALL です。</param>
		void InitAsDescriptorTable(UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			mRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			mRootParameter.ShaderVisibility = Visibility;
			mRootParameter.DescriptorTable.NumDescriptorRanges = RangeCount;
			mRootParameter.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
		}
		/// <summary>
		/// 指定したインデックスのディスクリプタレンジを設定します。
		/// </summary>
		/// <param name="RangeIndex">設定するディスクリプタレンジのインデックス。</param>
		/// <param name="Type">ディスクリプタレンジの種類（D3D12_DESCRIPTOR_RANGE_TYPE 列挙型）。</param>
		/// <param name="Register">ベースとなるシェーダレジスタ番号。</param>
		/// <param name="Count">ディスクリプタの数。</param>
		/// <param name="Space">レジスタスペース番号（省略時は0）。</param>
		void SetTableRange(UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0)
		{
			// ディスクリプタレンジを設定する。
			D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(mRootParameter.DescriptorTable.pDescriptorRanges + RangeIndex);
			range->RangeType = Type;				// ディスクリプタの種類
			range->NumDescriptors = Count;			//  ディスクリプタの数
			range->BaseShaderRegister = Register;	//  ベースシェーダレジスタ
			range->RegisterSpace = Space;			//  レジスタスペース
			range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		}

		const D3D12_ROOT_PARAMETER& operator() () const { return mRootParameter; }

	protected:
		D3D12_ROOT_PARAMETER mRootParameter;
	};

    class RootSignature
    {
        friend class DynamicDescriptorHeap;

    public:

        RootSignature(UINT NumRootParams = 0, UINT NumStaticSamplers = 0) : mFinalized(FALSE), kNumParameters(NumRootParams)
        {
            Reset(NumRootParams, NumStaticSamplers);
        }

        ~RootSignature()
        {
        }

        static void DestroyAll(void);

        void Reset(UINT NumRootParams, UINT NumStaticSamplers = 0)
        {
            if (NumRootParams > 0)
                mParamArray.reset(new RootParameter[NumRootParams]);
            else
                mParamArray = nullptr;
            kNumParameters = NumRootParams;

            if (NumStaticSamplers > 0)
                mSamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
            else
                mSamplerArray = nullptr;
            kNumSamplers = NumStaticSamplers;
            kNumInitializedStaticSamplers = 0;
        }

        RootParameter& operator[] (size_t EntryIndex)
        {
			ASSERT(EntryIndex < kNumParameters);
            return mParamArray.get()[EntryIndex];
        }

        const RootParameter& operator[] (size_t EntryIndex) const
        {
			ASSERT(EntryIndex < kNumParameters);
            return mParamArray.get()[EntryIndex];
        }

        void InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
            D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);

        void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

        ID3D12RootSignature* GetSignature() const { return mSignature; }

    protected:

        BOOL mFinalized;
        UINT kNumParameters;
        UINT kNumSamplers;
        UINT kNumInitializedStaticSamplers;
        uint32_t mDescriptorTableBitMap;		                    // 非サンプラ記述子テーブルである
        uint32_t mSamplerTableBitMap;			                    // ルートパラメータには1ビットが設定されます
        uint32_t mDescriptorTableSize[16];		                    // 非サンプラ記述子テーブルは、記述子の数を知る必要があります
        std::unique_ptr<RootParameter[]> mParamArray;               //	ルートパラメータの数
        std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> mSamplerArray; //	ルートパラメータ配列
        ID3D12RootSignature* mSignature;                            //  静的サンプラー配列
    };                                                              //	ルートシグネチャ
}


