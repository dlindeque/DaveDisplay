#pragma once

#include <vcruntime_exception.h>
#include <string>
#include <iostream>
#include "sqlite3.h"

#ifdef _DEBUG
    #include <crtdbg.h>
    #define ASSERT _ASSERTE
    #define VERIFY ASSERT
    #define VERIFY_(result, expression) ASSERT(result == expression)
#else
    #define ASSERT __noop
    #define VERIFY(expression) (expression)
    #define VERIFY_(result, expression) (expression)
#endif

namespace xerxes
{
    template<typename T> struct optional {
        bool is_null;
        T value;
    };

    enum class sqlite_type {
        _int = SQLITE_INTEGER,
        _float = SQLITE_FLOAT,
        _blob = SQLITE_BLOB,
        _null = SQLITE_NULL,
        _text = SQLITE_TEXT
    };

    inline auto operator << (std::ostream &os, const sqlite_type &t) -> std::ostream& {
        switch (t) {
        case sqlite_type::_int: return os << "integer";
        case sqlite_type::_float: return os << "float";
        case sqlite_type::_blob: return os << "blob";
        case sqlite_type::_null: return os << "null";
        case sqlite_type::_text: return os << "text";
        default: throw std::exception("Unsupported sqlite_type");
        }
    }

    template<typename T> struct sqlite_handle_traits {
        using Type = T;
        constexpr inline static auto invalid() noexcept -> Type { return nullptr; }
    };

    template<typename traits> class sqlite_handle {
        using Type = decltype(traits::invalid());

        Type _value;

        inline auto close() noexcept -> void {
            if (*this) {
                traits::close(_value);
            }
        }

    public:
        sqlite_handle(const sqlite_handle &) = delete;
        
        constexpr explicit sqlite_handle(const Type &value = traits::invalid()) noexcept
            : _value(value)
        {}

        constexpr sqlite_handle(sqlite_handle &&c) noexcept
            : _value(c.detach())
        {}

        ~sqlite_handle() noexcept {
            close();
        }

        auto operator=(const sqlite_handle &)->sqlite_handle& = delete;
        auto operator=(sqlite_handle &&c) noexcept -> sqlite_handle& {
            if (this != &c) {
                reset(c.detach());
            }
            return *this;
        }

        inline constexpr explicit operator bool() const noexcept {
            return _value != traits::invalid();
        }

        constexpr inline auto get() const noexcept -> Type {
            return _value;
        }
        inline auto set() noexcept -> Type* {
            return &_value;
        }
        
        inline auto detach() noexcept -> Type {
            Type value = _value;
            _value = traits::invalid();
            return value;
        }
        inline auto reset(Type value = traits::invalid()) noexcept -> bool {
            if (_value != value) {
                close();
                _value = value;
            }

            return static_cast<bool>(*this);
        }

        inline auto swap(sqlite_handle<traits> &other) noexcept -> void {
            Type tmp = _value;
            _value = other._value;
            other._value = tmp;
        }
    };

    template<typename traits> constexpr auto operator ==(const sqlite_handle<traits> &left, const sqlite_handle<traits> &right) noexcept -> bool {
        return left.get() == right.get();
    }
    template<typename traits> constexpr auto operator !=(const sqlite_handle<traits> &left, const sqlite_handle<traits> &right) noexcept -> bool {
        return !(left == right);
    }

    template<typename traits> auto swap(sqlite_handle<traits> &left, sqlite_handle<traits> &right) noexcept -> void {
        left.swap(right);
    }

    class sqlite_exception : public std::exception {
    private:
        int _result;
    public:
        explicit sqlite_exception(sqlite3* connection) throw()
            : std::exception(sqlite3_errmsg(connection)), _result(sqlite3_extended_errcode(connection))
        { }

        inline auto result() const noexcept { return _result; }
    };

    class sqlite_connection {
    private:
        struct sqlite_connection_handle_traits : public sqlite_handle_traits<sqlite3*> {
            static inline auto close(sqlite3* value) noexcept {
                VERIFY_(SQLITE_OK, sqlite3_close(value));
            }
        };

        using sqlite_connection_handle = sqlite_handle<sqlite_connection_handle_traits>;

        sqlite_connection_handle _handle;

        template<typename F, typename C> inline auto internal_open(const F &open, const C* const fn) -> void {
            sqlite_connection t;
            if (open(fn, t._handle.set()) != SQLITE_OK) {
                throw sqlite_exception(t._handle.get());
            }

            xerxes::swap(_handle, t._handle);
        }
    public:

        sqlite_connection() noexcept = default;
        template<typename T> sqlite_connection(T &&fn) {
            open(std::forward<T>(fn));
        }

        inline auto open(const char* const fn) -> void {
            internal_open(sqlite3_open, fn);
        }
        inline auto open(const wchar_t* const fn) -> void {
            internal_open(sqlite3_open16, fn);
        }
        inline auto open(const std::string &fn) -> void {
            open(fn.c_str());
        }
        inline auto open(const std::wstring &fn) -> void {
            open(fn.c_str());
        }

        static inline auto memory() -> sqlite_connection {
            return sqlite_connection(":memory:");
        }
        static inline auto wmemory() -> sqlite_connection {
            return sqlite_connection(L":memory:");
        }

        constexpr inline auto get_abi() const -> sqlite3* { return _handle.get(); }
        constexpr inline explicit operator bool() const noexcept { return static_cast<bool>(_handle); }

        static inline auto get_is_threadsafe() -> bool {
            return sqlite3_threadsafe() != 0;
        }

        inline auto get_last_inserted_rowid() const noexcept -> long long {
            return sqlite3_last_insert_rowid(get_abi());
        }

        friend constexpr auto operator ==(const sqlite_connection &left, const sqlite_connection &right) noexcept -> bool {
            return left._handle == right._handle;
        }
        friend constexpr auto operator !=(const sqlite_connection &left, const sqlite_connection &right) noexcept -> bool {
            return !(left == right);
        }

        inline auto swap(sqlite_connection &other) noexcept -> void {
            xerxes::swap(_handle, other._handle);
        }

        template<typename F> inline auto set_profile_handler(const F &callback, void * context = nullptr) -> void {
            sqlite3_profile(get_abi(), callback, context);
        }

        inline auto set_busy_timeout(int ms) noexcept -> void {
            sqlite3_busy_timeout(get_abi(), ms);
        }
    };

    inline auto swap(sqlite_connection &left, sqlite_connection &right) noexcept -> void {
        left.swap(right);
    }

    template<typename T> struct sqlite_reader {
        inline auto get_is_null(const int col = 0) const noexcept -> bool {
            return sqlite3_column_type(static_cast<const T *>(this)->get_abi(), col) == SQLITE_NULL;
        }
        inline auto get_int64(const int col = 0) const noexcept -> long long {
            return sqlite3_column_int64(static_cast<const T *>(this)->get_abi(), col);
        }
        inline auto get_int(const int col = 0) const noexcept -> int {
            return sqlite3_column_int(static_cast<const T *>(this)->get_abi(), col);
        }
        inline auto get_string(const int col = 0) const noexcept -> const char* {
            return reinterpret_cast<const char*>(sqlite3_column_text(static_cast<const T *>(this)->get_abi(), col));
        }
        inline auto get_wstring(const int col = 0) const noexcept -> const wchar_t* {
            return static_cast<const wchar_t*>(sqlite3_column_text16(static_cast<const T *>(this)->get_abi(), col));
        }

        inline auto get_column_type(const int col = 0) const noexcept -> sqlite_type {
            return static_cast<sqlite_type>(sqlite3_column_type(static_cast<const T *>(this)->get_abi(), col));
        }
    };

    class sqlite_statement : public sqlite_reader<sqlite_statement> {
    private:
        struct sqlite_statement_handle_traits : public sqlite_handle_traits<sqlite3_stmt*> {
            inline static auto close(sqlite3_stmt* value) noexcept {
                sqlite3_finalize(value);
            }
        };

        using sqlite_statement_handle = sqlite_handle<sqlite_statement_handle_traits>;

        sqlite_statement_handle _handle;

        template<typename F, typename C> inline auto internal_prepare(const sqlite_connection &cn, const F &prepare, const C * const text) -> void {
            ASSERT(static_cast<bool>(cn));

            if (prepare(cn.get_abi(), text, -1, _handle.set(), nullptr) != SQLITE_OK) {
                throw sqlite_exception(cn.get_abi());
            }
        }

        template<typename T> inline auto internal_bind_all(int index, T &&value) const -> void {
            bind(index, std::forward<T>(value));
        }

        template<typename Value, typename ... Rest> inline auto internal_bind_all(int index, Value && value, Rest && ... rest) const -> void {
            bind(index, std::forward<Value>(value));
            internal_bind_all(index + 1, std::forward<Rest>(rest) ...);
        }
    public:

        sqlite_statement() noexcept = default;
        template<typename T, typename ... Values> sqlite_statement(const sqlite_connection &cn, T &&text, Values && ... values) {
            prepare(cn, std::forward<T>(text), std::forward<Values>(values)...);
        }

        constexpr inline auto get_abi() const -> sqlite3_stmt* { return _handle.get(); }
        constexpr inline explicit operator bool() const noexcept { return static_cast<bool>(_handle); }

        inline auto prepare(const sqlite_connection &cn, const char * const text) -> void {
            internal_prepare(cn, sqlite3_prepare_v2, text);
        }

        inline auto prepare(const sqlite_connection &cn, const wchar_t * const text) -> void {
            internal_prepare(cn, sqlite3_prepare16_v2, text);
        }

        inline auto prepare(const sqlite_connection &cn, const std::string &text) -> void {
            prepare(cn, text.c_str());
        }

        inline auto prepare(const sqlite_connection &cn, const std::wstring &text) -> void {
            prepare(cn, text.c_str());
        }

        //template<typename T, typename ... Values> inline auto prepare(const sqlite_connection &cn, T && text, Values && ... values) -> void {
        //    prepare(cn, std::forward<T>(text));
        //    bind_all(std::forward<Values>(values)...);
        //}

        inline auto move_next() const -> bool {
            switch (sqlite3_step(get_abi())) {
            case SQLITE_ROW: return true;
            case SQLITE_DONE: return false;
            default:
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }
        }

        inline auto execute() const -> void {
            VERIFY(move_next() == false);
        }

        inline auto bind(const int index, const long long value) const -> const sqlite_statement&{
            if (sqlite3_bind_int64(get_abi(), index, value) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }

            return *this;
        }
        inline auto bind(const int index, const double value) const -> const sqlite_statement&{
            if (sqlite3_bind_double(get_abi(), index, value) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }

            return *this;
        }
        inline auto bind(const int index, const int value) const -> const sqlite_statement&{
            if (sqlite3_bind_int(get_abi(), index, value) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }

            return *this;
        }
        inline auto bind(const int index, const char* value, int size = -1) const -> const sqlite_statement&{
            if (sqlite3_bind_text(get_abi(), index, value, size, SQLITE_STATIC) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }

            return *this;
        }
        inline auto bind(const int index, const wchar_t* value, int size = -1) const -> const sqlite_statement&{
            if (sqlite3_bind_text16(get_abi(), index, value, size, SQLITE_STATIC) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }

            return *this;
        }
        inline auto bind(const int index, const std::string &value) const -> const sqlite_statement&{
            bind(index, value.c_str(), (int)value.size());

            return *this;
        }
        inline auto bind(const int index, std::string &&value) const -> const sqlite_statement&{
            if (sqlite3_bind_text(get_abi(), index, value.c_str(), (int)value.size(), SQLITE_TRANSIENT) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }

            return *this;
        }
        inline auto bind(const int index, const std::wstring &value) const -> const sqlite_statement&{
            bind(index, value.c_str(), (int)(value.size() * sizeof(wchar_t)));
            return *this;
        }
        inline auto bind(const int index, std::wstring &&value) const -> const sqlite_statement&{
            if (sqlite3_bind_text16(get_abi(), index, value.c_str(), (int)(value.size() * sizeof(wchar_t)), SQLITE_TRANSIENT) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }
            return *this;
        }
        inline auto bind(const int index, nullptr_t) const -> const sqlite_statement&{
            if (sqlite3_bind_null(get_abi(), index) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }
            return *this;
        }
        inline auto bind(const int index, bool value) const -> const sqlite_statement&{
            return bind(index, (value ? 1 : 0));
        }
        template<typename T> inline auto bind(const int index, const optional<T> &value) const -> const sqlite_statement&{
            if (value.is_null) {
                return bind(index, nullptr);
            }
            else {
                return bind(index, value.value);
            }
        }

        template<typename ... Values> inline auto bind_all(Values && ... values) const -> const sqlite_statement& {
            internal_bind_all(1, std::forward<Values>(values) ...);
            return *this;
        }

        inline auto reset() const -> void {
            if (sqlite3_reset(get_abi()) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }
        }

        inline auto clear_bindings() const -> void {
            if (sqlite3_clear_bindings(get_abi()) != SQLITE_OK) {
                throw sqlite_exception(sqlite3_db_handle(get_abi()));
            }
        }

        template<typename ... Values> inline auto rebind_all(Values && ... values) const -> const sqlite_statement& {
            reset();
            //clear_bindings();
            internal_bind_all(1, std::forward<Values>(values) ...);
            return *this;
        }

        friend constexpr auto operator ==(const sqlite_statement &left, const sqlite_statement &right) noexcept -> bool {
            return left._handle == right._handle;
        }
        friend constexpr auto operator !=(const sqlite_statement &left, const sqlite_statement &right) noexcept -> bool {
            return !(left == right);
        }

        inline auto swap(sqlite_statement &other) noexcept -> void {
            xerxes::swap(_handle, other._handle);
        }
    };

    inline auto swap(sqlite_statement &left, sqlite_statement &right) noexcept -> void {
        left.swap(right);
    }

    template<typename T, typename ... Values> inline auto sqlite_execute(const sqlite_connection &cn, T &&text, Values && ... values) -> void {
        sqlite_statement(cn, std::forward<T>(text), std::forward<Values>(values)...).execute();
    }

    template<typename T, typename ... Values> inline auto sqlite_execute_scalar_int(const sqlite_connection &cn, T &&text, Values && ... values) -> int {
        sqlite_statement stmt(cn, std::forward<T>(text), std::forward<Values>(values)...);
        VERIFY(stmt.move_next() == true);
        return stmt.get_int();
    }

    class sqlite_iterator;

    class sqlite_row : public sqlite_reader<sqlite_row> {
    private:
        sqlite3_stmt *_handle;
        sqlite_row(sqlite3_stmt *handle)
            : _handle(handle)
        {}
    public:
        sqlite_row() = delete;
        sqlite_row(const sqlite_row &) = default;
        sqlite_row(sqlite_row &&) noexcept = default;
        auto operator =(const sqlite_row&)->sqlite_row& = default;
        auto operator =(sqlite_row&&) noexcept->sqlite_row& = default;

        inline auto get_abi() const -> sqlite3_stmt* { return _handle; }

        friend sqlite_iterator;
    };

    class sqlite_iterator {
    private:
        const sqlite_statement *_statement = nullptr;
    public:
        sqlite_iterator() noexcept = default;
        sqlite_iterator(const sqlite_statement &statement) noexcept
        {
            if (statement.move_next()) {
                _statement = &statement;
            }
        }

        inline auto operator++() noexcept -> sqlite_iterator& {
            ASSERT(_statement != nullptr);
            if (!_statement->move_next()) {
                _statement = nullptr;
            }

            return *this;
        }

        inline auto operator ==(const sqlite_iterator &other) const noexcept -> bool {
            return _statement == other._statement;
        }
        inline auto operator !=(const sqlite_iterator &other) const noexcept -> bool {
            return !(*this == other);
        }

        inline auto operator *() const noexcept -> sqlite_row {
            return sqlite_row(_statement->get_abi());
        }
    };

    inline auto begin(const sqlite_statement &statement) noexcept -> sqlite_iterator {
        return sqlite_iterator(statement);
    }
    inline auto end(const sqlite_statement &statement) noexcept -> sqlite_iterator {
        return sqlite_iterator();
    }

    class sqlite_backup {
    private:
        struct sqlite_backup_handle_traits : public sqlite_handle_traits<sqlite3_backup*> {
            static inline auto close(sqlite3_backup* value) noexcept {
                VERIFY_(SQLITE_OK, sqlite3_backup_finish(value));
            }
        };

        using sqlite_backup_handle = sqlite_handle<sqlite_backup_handle_traits>;

        sqlite_backup_handle _handle;
        const sqlite_connection *_destination;
    public:
        sqlite_backup(const sqlite_connection &destination, const sqlite_connection &source, const char* destination_name = "main", const char *source_name = "main")
            : _handle(sqlite3_backup_init(destination.get_abi(), destination_name, source.get_abi(), source_name)), _destination(&destination)
        {
            if (!_handle) {
                throw sqlite_exception(destination.get_abi());
            }
        }
        
        constexpr inline auto get_abi() const -> sqlite3_backup* { return _handle.get(); }
        constexpr inline explicit operator bool() const noexcept { return static_cast<bool>(_handle); }

        friend constexpr auto operator ==(const sqlite_backup &left, const sqlite_backup &right) noexcept -> bool {
            return left._handle == right._handle;
        }
        friend constexpr auto operator !=(const sqlite_backup &left, const sqlite_backup &right) noexcept -> bool {
            return !(left == right);
        }

        inline auto swap(sqlite_backup &other) noexcept -> void {
            xerxes::swap(_handle, other._handle);
            const sqlite_connection *t = _destination;
            _destination = other._destination;
            other._destination = t;
        }

        inline auto move_next(const int pages = -1) -> bool {
            switch (sqlite3_backup_step(get_abi(), pages)) {
            case SQLITE_OK: return true;
            case SQLITE_DONE: return false;
            default:
                _handle.reset();
                throw sqlite_exception(_destination->get_abi());
            }
        }
    };

    inline auto swap(sqlite_backup &left, sqlite_backup &right) noexcept -> void {
        left.swap(right);
    }
}