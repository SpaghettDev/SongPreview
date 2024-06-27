#include <fstream>

#include <Geode/modify/CustomSongWidget.hpp>
#include <Geode/modify/CustomSongCell.hpp>
#include <Geode/modify/CustomMusicCell.hpp>

#include <Geode/binding/MusicDownloadManager.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <Geode/binding/GameManager.hpp>

#include "types/SongState.hpp"

#include "nodes/SPLoadingCircle.hpp"

#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>

#include "utils.hpp"

using namespace geode::prelude;

const std::filesystem::path SnippetsDir = Mod::get()->getSaveDir() / "snippets";
std::vector<int> g_downloadedSnippets;
std::uint32_t g_playingSong = -1;

struct CustomSongWidgetPlus : Modify<CustomSongWidgetPlus, CustomSongWidget>
{
	struct Fields
	{
		bool m_is_playing = false;
		SongState m_song_download_state = SongState::NONE;
		std::string m_url;

		SPLoadingCircle* m_loading_circle;

		EventListener<utils::web::WebTask> m_song_downloader_listener;
	};

	bool init(SongInfoObject* songInfo, CustomSongDelegate* songDelegate, bool showSongSelect, bool showPlayMusic, bool showDownload, bool isRobtopSong, bool unkBool, bool isMusicLibrary, int isActiveSong)
	{
		if (!CustomSongWidget::init(songInfo, songDelegate, showSongSelect, showPlayMusic, showDownload, isRobtopSong, unkBool, isMusicLibrary, isActiveSong))
			return false;

		// updateWithMultiAssets is not called when a song widget is made for CustomSongCell/CustomMusicCell
		if (isMusicLibrary && this->m_showDownloadBtn)
		{
			this->m_playbackBtn->setPosition(this->m_selectSongBtn->getPosition());
			this->m_playbackBtn->setVisible(true);
		}

		if (this->m_customSongID <= 10000000)
			m_fields->m_url = fmt::format("https://www.newgrounds.com/audio/download/{}", this->m_customSongID);
		else
			m_fields->m_url = fmt::format("https://geometrydashfiles.b-cdn.net/music/{}.ogg", this->m_customSongID);

		m_fields->m_loading_circle = SPLoadingCircle::create();
		m_fields->m_loading_circle->setScale(.5f);
		m_fields->m_loading_circle->setPosition(this->m_playbackBtn->getPosition());
		m_fields->m_loading_circle->setID("preview-loading-circle"_spr);
		this->m_buttonMenu->addChild(m_fields->m_loading_circle);

		return true;
	}

	void updateSongInfo()
	{
		CustomSongWidget::updateSongInfo();

		// the game is so adamant on hiding this button its unreal
		this->m_playbackBtn->setVisible(true);
	}

	void updateWithMultiAssets(gd::string p0, gd::string p1, int p2)
	{
		CustomSongWidget::updateWithMultiAssets(p0, p1, p2);

		// this is getting a bit too crowded...
		if (this->m_songs.size() != 1 || this->m_sfx.size() != 0)
			this->m_infoBtn->setPositionX(this->m_playbackBtn->getPositionX() - 26.f);

		this->m_playbackBtn->setVisible(true);
	}

	void onPlayback(CCObject* sender)
	{
		m_fields->m_is_playing = !m_fields->m_is_playing;

		g_playingSong = m_fields->m_is_playing ? this->m_customSongID : -1;

		// TODO: why are rob songs breaking ffs
		if (this->m_isRobtopSong)
		{
			CustomSongWidget::onPlayback(sender);
			this->m_playbackBtn->setVisible(true);
			return;
		}

		if (SP::isSongDownloaded(this->m_customSongID))
			CustomSongWidget::onPlayback(sender);
		else
			onPlaybackNotDownloaded();
	}

	// this gets called on all CSWs on the screen
	void updatePlaybackBtn()
	{
		CustomSongWidget::updatePlaybackBtn();

		this->m_playbackBtn->setVisible(true);

		if (this->m_isRobtopSong) return;

		auto* MDM = MusicDownloadManager::sharedState();
		auto* FMOD = FMODAudioEngine::sharedEngine();

		const auto& songPath = getSongFilePath();

		if (m_fields->m_is_playing && this->m_customSongID == g_playingSong && FMOD->isMusicPlaying(songPath, 0))
			static_cast<CCSprite*>(this->m_playbackBtn->getNormalImage())->setDisplayFrame(
				CCSpriteFrameCache::get()->spriteFrameByName("GJ_stopMusicBtn_001.png")
			);
		else
			static_cast<CCSprite*>(this->m_playbackBtn->getNormalImage())->setDisplayFrame(
				CCSpriteFrameCache::get()->spriteFrameByName("GJ_playMusicBtn_001.png")
			);

		if (this->m_customSongID != g_playingSong)
			m_fields->m_is_playing = false;

		// DOWNLOAD_FAILED usually means newgrounds sent back 400, TODO: fetch getGJSongInfo.php in that case
		if (
			m_fields->m_song_download_state == SongState::DOWNLOAD_FAILED ||
			m_fields->m_song_download_state == SongState::DOWNLOADED && FMOD->m_lastResult != FMOD_RESULT::FMOD_OK
		) {
			m_fields->m_is_playing = false;
			this->m_playbackBtn->setColor({ 125, 125, 125 });
			this->m_playbackBtn->m_bEnabled = false;
		}
	}

	void onPlaybackNotDownloaded()
	{
		FMODAudioEngine::sharedEngine()->stopAllMusic();

		if (m_fields->m_is_playing)
		{
			static_cast<CCSprite*>(this->m_playbackBtn->getNormalImage())->setDisplayFrame(
				CCSpriteFrameCache::get()->spriteFrameByName("GJ_stopMusicBtn_001.png")
			);

			if (isSnippetDownloaded())
				playDownloadedSnippet();
			else
				fetchAndPlaySnippet();
		}
		else
		{
			static_cast<CCSprite*>(this->m_playbackBtn->getNormalImage())->setDisplayFrame(
				CCSpriteFrameCache::get()->spriteFrameByName("GJ_playMusicBtn_001.png")
			);

			GameManager::sharedState()->fadeInMenuMusic();
		}
	}

	void playDownloadedSnippet()
	{
		SP::playSong(getSongFilePath());

		this->updatePlaybackBtn();
	}

	void fetchAndPlaySnippet()
	{
		this->m_playbackBtn->setVisible(false);

		m_fields->m_loading_circle->show();

		m_fields->m_song_downloader_listener.bind([&](web::WebTask::Event* e) {
			if (web::WebResponse* res = e->getValue())
			{
				if (!res->ok())
				{
					m_fields->m_song_download_state = SongState::DOWNLOAD_FAILED;
					m_fields->m_loading_circle->hide();
					this->updatePlaybackBtn();

					return;
				}

				const auto& songPath = getSongFilePath();

				std::ofstream{ songPath };
				auto resp = res->into(songPath);

				m_fields->m_song_download_state = SongState::DOWNLOADED;

				SP::playSong(songPath);

				this->updatePlaybackBtn();
				m_fields->m_loading_circle->hide();

				g_downloadedSnippets.emplace_back(this->m_customSongID);
			}
		});

		auto req = web::WebRequest()
			.downloadRange({ 0, SP::secondsToBytes(Mod::get()->getSettingValue<std::int64_t>("preview-time")) - 1 })
			.get(m_fields->m_url);

		m_fields->m_song_downloader_listener.setFilter(req);
	}

	/**
	 * Checks if the CSW's song ID has already been fetched and installed.
	 *
	 * @return bool
	 */
	bool isSnippetDownloaded()
	{
		return std::find(
			g_downloadedSnippets.cbegin(),
			g_downloadedSnippets.cend(),
			this->m_customSongID
		) != g_downloadedSnippets.cend();
	}

	/**
	 * Gets the CSW's song path. It is either the song's path if downloaded, or the snippet's path if not.
	 *
	 * @return std::string 
	 */
	std::string getSongFilePath()
	{
		if (SP::isSongDownloaded(this->m_customSongID))
			return MusicDownloadManager::sharedState()->pathForSong(this->m_customSongID);
		else
			return (
				SnippetsDir / fmt::format(
					"{}.{}",
					this->m_customSongID,
					this->m_customSongID <= 10000000 ? "mp3" : "ogg"
				)
			).string();
	}
};

struct CustomSongCellPlus : Modify<CustomSongCellPlus, CustomSongCell>
{
	void loadFromObject(SongInfoObject* obj)
	{
		CustomSongCell::loadFromObject(obj);

		auto* songWidget = getChildOfType<CustomSongWidget>(this->m_mainLayer, 0);

		// reset preview button position in CustomSongCell if we finish downloading the song
		if (SP::isSongDownloaded(songWidget->m_customSongID))
		{
			songWidget->m_playbackBtn->setPosition(songWidget->m_downloadBtn->getPosition());
			songWidget->m_playbackBtn->setVisible(true);
		}
	}
};

struct CustomMusicCellPlus : Modify<CustomMusicCellPlus, CustomMusicCell>
{
	void loadFromObject(SongInfoObject* obj)
	{
		CustomMusicCell::loadFromObject(obj);

		auto* songWidget = getChildOfType<CustomSongWidget>(this->m_mainLayer, 0);

		// reset preview button position in CustomMusicCell if we finish downloading the song
		if (SP::isSongDownloaded(songWidget->m_customSongID))
		{
			songWidget->m_playbackBtn->setPosition(songWidget->m_downloadBtn->getPosition());
			songWidget->m_playbackBtn->setVisible(true);
		}
	}
};

$execute
{
	std::filesystem::create_directory(SnippetsDir);

	for (const auto& entry : std::filesystem::directory_iterator(SnippetsDir))
		std::filesystem::remove_all(entry.path());
}
