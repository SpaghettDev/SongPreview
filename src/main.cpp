#include <fstream>
#include <filesystem>

#include <Geode/modify/CustomSongWidget.hpp>
#include <Geode/modify/MusicDownloadManager.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>

#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>

using namespace geode::prelude;

class $modify(CustomSongWidget)
{
	struct Fields
	{
		bool m_is_playing = false;
		std::string m_url;

		EventListener<utils::web::WebTask> m_song_downloader_listener;
	};

	bool init(SongInfoObject* songInfo, CustomSongDelegate* songDelegate, bool showSongSelect, bool showPlayMusic, bool showDownload, bool isRobtopSong, bool unkBool, bool isMusicLibrary, int isActiveSong)
	{
		if (!CustomSongWidget::init(songInfo, songDelegate, showSongSelect, showPlayMusic, showDownload, isRobtopSong, unkBool, isMusicLibrary, isActiveSong))
			return false;

		// updateWithMultiAssets is not called when a song widget is made for CustomSongCell/CustomMusicCell
		// TODO: fix this shit breaking in MusicBrowser
		if (isMusicLibrary && this->m_showDownloadBtn)
		{
			this->m_playbackBtn->setPosition(this->m_selectSongBtn->getPosition());
			this->m_playbackBtn->setVisible(true);
		}

		if (this->m_customSongID <= 10000000)
			m_fields->m_url = fmt::format("https://www.newgrounds.com/audio/download/{}", this->m_customSongID);
		else
			m_fields->m_url = fmt::format("https://geometrydashfiles.b-cdn.net/music/{}.ogg", this->m_customSongID);

		return true;
	}

	void updateWithMultiAssets(gd::string p0, gd::string p1, int p2)
	{
		CustomSongWidget::updateWithMultiAssets(p0, p1, p2);

		auto* MDM = MusicDownloadManager::sharedState();

		// this is getting a bit too crowded...
		if (this->m_songs.size() != 1 || this->m_sfx.size() != 0)
			this->m_infoBtn->setPositionX(-183.f);

		this->m_playbackBtn->setVisible(true);
	}

	void onPlayback(CCObject* sender)
	{
		m_fields->m_is_playing = !m_fields->m_is_playing;

		auto* MDM = MusicDownloadManager::sharedState();

		if (this->m_customSongID <= 10000000)
		{
			if (MDM->isSongDownloaded(this->m_customSongID))
			{
				log::debug("song isnt music lib and is downloaded");
				CustomSongWidget::onPlayback(sender);
			}
			else
			{
				log::debug("song isnt music lib and is not downloaded");
				onPlaybackNotDownloaded();
			}
		}
		else
		{
			// idk of any other way
			if (std::filesystem::exists(MDM->pathForSong(this->m_customSongID)))
			{
				log::debug("song is music lib and is downloaded");
				CustomSongWidget::onPlayback(sender);
			}
			else
			{
				log::debug("song is music lib and is not downloaded");
				onPlaybackNotDownloaded();
			}
		}

		this->m_playbackBtn->setVisible(true);
	}

	void updatePlaybackBtn()
	{
		CustomSongWidget::updatePlaybackBtn();

		log::debug("updated playback btn");
	}

	void onPlaybackNotDownloaded()
	{
		if (m_fields->m_is_playing)
		{
			static_cast<CCSprite*>(this->m_playbackBtn->getNormalImage())->setDisplayFrame(
				CCSpriteFrameCache::get()->spriteFrameByName("GJ_stopMusicBtn_001.png")
			);

			fetchSong();
		}
		else
		{
			static_cast<CCSprite*>(this->m_playbackBtn->getNormalImage())->setDisplayFrame(
				CCSpriteFrameCache::get()->spriteFrameByName("GJ_playMusicBtn_001.png")
			);

			if (auto* sharedEngine = FMODAudioEngine::sharedEngine(); sharedEngine->isMusicPlaying(0))
				sharedEngine->pauseMusic(0);
		}
	}

	void fetchSong()
	{
		m_fields->m_song_downloader_listener.bind([&](web::WebTask::Event* e) {
			if (web::WebResponse* res = e->getValue())
			{
				if (!res->ok()) return;

				auto songPath = Mod::get()->getSaveDir() / "songs" / fmt::format(
					"{}.{}", this->m_customSongID, this->m_customSongID <= 10000000 ? "mp3" : "ogg"
				);

				std::ofstream{ songPath };
				res->into(songPath);

				playSong(songPath);
			}
		});

		auto req = web::WebRequest()
			.downloadRange({ 0, getNumberOfBytes() - 1 })
			.get(m_fields->m_url);

		m_fields->m_song_downloader_listener.setFilter(req);
	}

	void playSong(const std::filesystem::path& path)
	{
		FMODAudioEngine::sharedEngine()->playMusic(path.string(), false, 0.f, 0);
	}

	std::uint64_t getNumberOfBytes()
	{
		const int bitRate = 128 * 1024 / 8; // assume 128 kbps
		return Mod::get()->getSavedValue<std::int64_t>("preview-time") * bitRate;
	}
};

$execute
{
	std::filesystem::create_directory(Mod::get()->getSaveDir() / "songs");

	for (const auto& entry : std::filesystem::directory_iterator(Mod::get()->getSaveDir() / "songs")) 
        std::filesystem::remove_all(entry.path());
}
