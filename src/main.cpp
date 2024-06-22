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

		EventListener<utils::web::WebTask> m_length_listener;
		EventListener<utils::web::WebTask> m_ng_length_listener;
		std::uint32_t m_song_length = -1;

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
		auto* MDM = MusicDownloadManager::sharedState();

		if (this->m_customSongID <= 10000000 && MDM->isSongDownloaded(this->m_customSongID))
			CustomSongWidget::onPlayback(sender);
		else
			onPlaybackNotDownloaded();

		this->m_playbackBtn->setVisible(true);
	}

	void onPlaybackNotDownloaded()
	{
		m_fields->m_is_playing = !m_fields->m_is_playing;

		if (m_fields->m_is_playing)
		{
			static_cast<CCSprite*>(this->m_playbackBtn->getNormalImage())->setDisplayFrame(
				CCSpriteFrameCache::get()->spriteFrameByName("GJ_stopMusicBtn_001.png")
			);

			fetchSongLength();


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

	void fetchSongLength()
	{
		m_fields->m_length_listener.bind([&](web::WebTask::Event* e) {
			if (web::WebResponse* res = e->getValue())
			{
				if (this->m_customSongID <= 10000000)
				{
					m_fields->m_ng_length_listener.bind([&](web::WebTask::Event* e) {
						if (web::WebResponse* res = e->getValue())
						{
							m_fields->m_song_length = std::stoi(res->header("content-length").value_or("0"));
							if (m_fields->m_song_length == 0) return;

							log::debug("normal song length is {}", m_fields->m_song_length);
						}
					});

					for (const auto& e : res->headers())
						log::debug("{}", e);

					if (res->header("location").has_value())
					{
						auto req = web::WebRequest()
							.send("HEAD", res->header("location").value());

						m_fields->m_ng_length_listener.setFilter(req);
					}
				}
				else
				{
					m_fields->m_song_length = std::stoi(res->header("content-length").value_or("0"));
					for (const auto& e : res->headers())
						log::debug("{}", e);
					if (m_fields->m_song_length == 0) return;

					log::debug("ng song length is {}", m_fields->m_song_length);
				}
			}
		});

		std::string_view url;
		if (this->m_customSongID <= 10000000)
			url = fmt::format("www.newgrounds.com/audio/download/{}", this->m_customSongID);
		else
			url = fmt::format("https://geometrydashfiles.b-cdn.net/music/{}.ogg", this->m_customSongID);

		auto req = web::WebRequest()
			.send("HEAD", url);

		m_fields->m_length_listener.setFilter(req);
	}
};
