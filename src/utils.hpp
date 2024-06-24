#pragma once

#include <filesystem>

#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>

namespace SP
{
	inline void playSong(const std::filesystem::path& path)
	{
		FMODAudioEngine::sharedEngine()->playMusic(path.string(), false, 0.f, 0);
	}

	inline std::uint64_t secondsToBytes(std::uint64_t seconds)
	{
		const int bitRate = 128 * 1024 / 8; // assume 128 kbps
		return seconds * bitRate;
	}

	inline bool isSongDownloaded(std::uint32_t id)
	{
		auto* MDM = MusicDownloadManager::sharedState();

		// idk of any other way (MDM::isSongDownloaded always returns true for music library IDs)
		if (id <= 10000000)
			return MDM->isSongDownloaded(id);
		else
			return std::filesystem::exists(MDM->pathForSong(id));
	}
}
