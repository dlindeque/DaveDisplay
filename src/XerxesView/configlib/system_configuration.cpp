#include "stdafx.h"
#include "system_configuration.h"

namespace xerxes
{
    system_configuration::system_configuration(const std::string & fn)
        : _connection(fn)
    {
        // Create the database
        sqlite_statement(_connection, "CREATE TABLE IF NOT EXISTS [window]([id] INTEGER PRIMARY KEY, [key] TEXT NOT NULL, [show_on_primary] INTEGER NOT NULL, [show_maximized] INTEGER NOT NULL, [show_fullscreen] INTEGER NOT NULL, [monitor_name] TEXT)").execute();

        _window_select.prepare(_connection, "SELECT [key], [show_on_primary], [show_maximized], [show_fullscreen], [monitor_name] FROM [window]");
        _window_find.prepare(_connection, "SELECT [id] FROM [window] WHERE [key]=?");
        _window_insert.prepare(_connection, "INSERT INTO [window]([key], [show_on_primary], [show_maximized], [show_fullscreen], [monitor_name]) VALUES (?, ?, ?, ?, ?)");
        _window_update.prepare(_connection, "UPDATE [window] SET [key]=?, [show_on_primary]=?, [show_maximized]=?, [show_fullscreen]=?, [monitor_name]=? WHERE [id] = ?");
    }

    auto system_configuration::write_window_configuration(const std::wstring & key, const window_config & cfg) -> void
    {
        if (_window_find.rebind_all(key).move_next()) {
            // Found the record - update it
            _window_update.rebind_all(key, cfg.show_on_primary, cfg.show_maximized, cfg.show_fullscreen, optional<std::wstring>{ cfg.monitor_name.empty(), cfg.monitor_name }, _window_find.get_int64(0)).execute();
        }
        else {
            // Not found - insert it
            _window_insert.rebind_all(key, cfg.show_on_primary, cfg.show_maximized, cfg.show_fullscreen, optional<std::wstring>{ cfg.monitor_name.empty(), cfg.monitor_name }).execute();
        }
    }

}
