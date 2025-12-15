#pragma once

#include <filesystem>

#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>

namespace SP
{
	constexpr std::uint16_t NG_BITRATE = 128 * 1000 / 8;
	constexpr std::uint16_t GDCS_BITRATE = 320 * 1000 / 8;

	inline void playSong(const std::filesystem::path& path)
	{
		FMODAudioEngine::sharedEngine()->playMusic(path.string(), false, 0.f, 0);
	}

	inline std::uint64_t secondsToBytes(std::uint64_t seconds, bool is320kbps)
	{
		return seconds * (is320kbps ? GDCS_BITRATE : NG_BITRATE);
	}

	inline bool isSongDownloaded(std::uint32_t id)
	{
		auto* MDM = MusicDownloadManager::sharedState();

		// idk of any other way (MDM::isSongDownloaded always returns true for music library IDs)
		if (id <= 10000000)
			return MDM->isSongDownloaded(id);
		else
			return std::filesystem::exists(MDM->pathForSong(id).c_str());
	}
}
