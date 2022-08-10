/*

Copyright (c) 2016, 2018-2022, Arvid Norberg
Copyright (c) 2019, Steven Siloti
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#ifndef TORRENT_MMAP_HPP
#define TORRENT_MMAP_HPP

#include "libtorrent/config.hpp"

#if TORRENT_HAVE_MMAP || TORRENT_HAVE_MAP_VIEW_OF_FILE

#include "libtorrent/disk_interface.hpp" // for open_file_state
#include "libtorrent/aux_/open_mode.hpp"
#include "libtorrent/aux_/file.hpp" // for file_handle

#if TORRENT_HAVE_MAP_VIEW_OF_FILE

#include "libtorrent/aux_/windows.hpp"
#include <mutex>

#endif // TORRENT_HAVE_MAP_VIEW_OF_FILE

namespace libtorrent {

// for now
using byte = char;

namespace aux {

	using namespace flags;

	struct file_view;

#if TORRENT_HAVE_MAP_VIEW_OF_FILE
	struct TORRENT_EXTRA_EXPORT file_mapping_handle
	{
		file_mapping_handle(file_handle file, open_mode_t mode, std::int64_t size);
		~file_mapping_handle();
		file_mapping_handle(file_mapping_handle const&) = delete;
		file_mapping_handle& operator=(file_mapping_handle const&) = delete;

		file_mapping_handle(file_mapping_handle&& fm);
		file_mapping_handle& operator=(file_mapping_handle&& fm) &;

		HANDLE handle() const { return m_mapping; }
	private:
		void close();
		file_handle m_file;
		HANDLE m_mapping;
	};
#endif

	struct TORRENT_EXTRA_EXPORT file_mapping : std::enable_shared_from_this<file_mapping>
	{
		friend struct file_view;

		file_mapping(file_handle file, open_mode_t mode, std::int64_t file_size
#if TORRENT_HAVE_MAP_VIEW_OF_FILE
			, std::shared_ptr<std::mutex> open_unmap_lock
#endif
			);

#if TORRENT_HAVE_MAP_VIEW_OF_FILE
		void flush();
#endif

		// non-copyable
		file_mapping(file_mapping const&) = delete;
		file_mapping& operator=(file_mapping const&) = delete;

		file_mapping(file_mapping&& rhs);
		file_mapping& operator=(file_mapping&& rhs) &;
		~file_mapping();

		// ...
		file_view view();
	private:

		void close();

		// the memory range this file has been mapped into
		span<byte> memory()
		{
			TORRENT_ASSERT(m_mapping || m_size == 0);
			return { static_cast<byte*>(m_mapping), static_cast<std::ptrdiff_t>(m_size) };
		}

		// hint the kernel that we probably won't need this part of the file
		// anytime soon
		void dont_need(span<byte const> range);

		// hint the kernel that the given (dirty) range of pages should be
		// flushed to disk
		void page_out(span<byte const> range);

		std::int64_t m_size;
#if TORRENT_HAVE_MAP_VIEW_OF_FILE
		file_mapping_handle m_file;
		std::shared_ptr<std::mutex> m_open_unmap_lock;
#else
		file_handle m_file;
#endif
		void* m_mapping;
	};

	struct TORRENT_EXTRA_EXPORT file_view
	{
		friend struct file_mapping;
		file_view(file_view&&) = default;
		file_view& operator=(file_view&&) = default;

		span<byte const> range() const
		{
			TORRENT_ASSERT(m_mapping);
			return m_mapping->memory();
		}

		span<byte> range()
		{
			TORRENT_ASSERT(m_mapping);
			return m_mapping->memory();
		}

		void dont_need(span<byte const> range)
		{
			TORRENT_ASSERT(m_mapping);
			m_mapping->dont_need(range);
		}

		void page_out(span<byte const> range)
		{
			TORRENT_ASSERT(m_mapping);
			m_mapping->page_out(range);
		}


	private:
		explicit file_view(std::shared_ptr<file_mapping> m) : m_mapping(std::move(m)) {}
		std::shared_ptr<file_mapping> m_mapping;
	};

} // aux
} // libtorrent

#endif // HAVE_MMAP || HAVE_MAP_VIEW_OF_FILE

#endif

