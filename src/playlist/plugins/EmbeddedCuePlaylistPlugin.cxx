/*
 * Copyright 2003-2017 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file
 *
 * Playlist plugin that reads embedded cue sheets from the "CUESHEET"
 * tag of a music file.
 */

#include "config.h"
#include "EmbeddedCuePlaylistPlugin.hxx"
#include "../PlaylistPlugin.hxx"
#include "../SongEnumerator.hxx"
#include "../cue/CueParser.hxx"
#include "tag/Handler.hxx"
#include "tag/Generic.hxx"
#include "DetachedSong.hxx"
#include "TagFile.hxx"
#include "fs/Traits.hxx"
#include "fs/AllocatedPath.hxx"
#include "util/ASCII.hxx"

#include <string.h>

class EmbeddedCuePlaylist final : public SongEnumerator {
public:
	/**
	 * This is an override for the CUE's "FILE".  An embedded CUE
	 * sheet must always point to the song file it is contained
	 * in.
	 */
	std::string filename;

	/**
	 * The value of the file's "CUESHEET" tag.
	 */
	std::string cuesheet;

	/**
	 * The offset of the next line within "cuesheet".
	 */
	char *next;

	CueParser *parser;

public:
	EmbeddedCuePlaylist()
		:parser(nullptr) {
	}

	virtual ~EmbeddedCuePlaylist() {
		delete parser;
	}

	virtual std::unique_ptr<DetachedSong> NextSong() override;
};

class ExtractCuesheetTagHandler final : public NullTagHandler {
public:
	std::string cuesheet;

	ExtractCuesheetTagHandler() noexcept:NullTagHandler(WANT_PAIR) {}

	void OnPair(const char *key, const char *value) noexcept override;
};

void
ExtractCuesheetTagHandler::OnPair(const char *name, const char *value) noexcept
{
	if (cuesheet.empty() &&
	    StringEqualsCaseASCII(name, "cuesheet"))
		cuesheet = value;
}

static std::unique_ptr<SongEnumerator>
embcue_playlist_open_uri(const char *uri,
			 gcc_unused Mutex &mutex)
{
	if (!PathTraitsUTF8::IsAbsolute(uri))
		/* only local files supported */
		return nullptr;

	const auto path_fs = AllocatedPath::FromUTF8Throw(uri);

	ExtractCuesheetTagHandler extract_cuesheet;
	ScanFileTagsNoGeneric(path_fs, extract_cuesheet);
	if (extract_cuesheet.cuesheet.empty())
		ScanGenericTags(path_fs, extract_cuesheet);

	if (extract_cuesheet.cuesheet.empty())
		/* no "CUESHEET" tag found */
		return nullptr;

	auto playlist = std::make_unique<EmbeddedCuePlaylist>();

	playlist->filename = PathTraitsUTF8::GetBase(uri);

	playlist->cuesheet = std::move(extract_cuesheet.cuesheet);

	playlist->next = &playlist->cuesheet[0];
	playlist->parser = new CueParser();

	return playlist;
}

std::unique_ptr<DetachedSong>
EmbeddedCuePlaylist::NextSong()
{
	auto song = parser->Get();
	if (song != nullptr) {
		song->SetURI(filename);
		return song;
	}

	while (*next != 0) {
		const char *line = next;
		char *eol = strpbrk(next, "\r\n");
		if (eol != nullptr) {
			/* null-terminate the line */
			*eol = 0;
			next = eol + 1;
		} else
			/* last line; put the "next" pointer to the
			   end of the buffer */
			next += strlen(line);

		parser->Feed(line);
		song = parser->Get();
		if (song != nullptr) {
			song->SetURI(filename);
			return song;
		}
	}

	parser->Finish();
	song = parser->Get();
	if (song != nullptr)
		song->SetURI(filename);
	return song;
}

static const char *const embcue_playlist_suffixes[] = {
	/* a few codecs that are known to be supported; there are
	   probably many more */
	"flac",
	"mp3", "mp2",
	"mp4", "mp4a", "m4b",
	"ape",
	"wv",
	"ogg", "oga",
	nullptr
};

const struct playlist_plugin embcue_playlist_plugin = {
	"embcue",

	nullptr,
	nullptr,
	embcue_playlist_open_uri,
	nullptr,

	nullptr,
	embcue_playlist_suffixes,
	nullptr,
};
