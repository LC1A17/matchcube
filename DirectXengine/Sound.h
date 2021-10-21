#pragma once

#include <Windows.h>
#include <xaudio2.h>
#include <wrl.h>

using namespace Microsoft::WRL;

//サウンド
class Sound
{
public://構造体
	//チャンクヘッダ
	struct ChunkHeader
	{
		char id[4];//チャンク毎のID
		int size;//チャンクサイズ
	};

	//RIFFヘッダチャンク
	struct RiffHeader
	{
		ChunkHeader	chunk;//"RIFF"
		char type[4];//"WAVE"
	};

	//FMTチャンク
	struct FormatChunk
	{
		ChunkHeader chunk;//"fmt "
		WAVEFORMATEX fmt;//波形フォーマット
	};

	//音声データ
	struct SoundData
	{
		WAVEFORMATEX wfex;//波形フォーマット
		BYTE* pBuffer;//バッファの先頭アドレス
		unsigned int bufferSize;//バッファのサイズ
	};

private://変数
	ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;
	//XAudio2VoiceCallback voiceCallback;

public://関数
	void Initialize();//初期化処理
	SoundData SoundLoadWave(const char* filename);//サウンドの読み込み
	void SoundUnload(SoundData* soundData);
	void SoundPlayWave(const SoundData& soundData);

};