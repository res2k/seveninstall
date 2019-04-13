#include <algorithm>
#include <system_error>

#include "windows.h"

namespace std
{
  // Obtain string for Windows error code
  unsigned long _Winerror_message (unsigned long code, char* buf, unsigned long bufsize)
  {
    int msg_len = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  nullptr, code, 0, buf, bufsize, nullptr);
    if (msg_len == 0)
    {
      // Fallback to error code
      msg_len = snprintf (buf, bufsize, "error %d", static_cast<int> (code));
      msg_len = std::min (static_cast<unsigned long> (msg_len), bufsize);
    }
    if (msg_len < 0)
    {
      *buf = 0;
      return 0;
    }
    return msg_len;
  }

  // Maximum number of supported error codes
  static const int error_string_num = 256;
  // Storage for error strings
  static const char* error_strings[error_string_num];

  // Obtain string for errc value
  const char* _Syserror_map (int errc_value)
  {
    static const char unknown_error_str[] = "unknown error";
    if ((errc_value < 0) || (errc_value >= error_string_num)) return unknown_error_str;

    if (error_strings[errc_value] != nullptr) return error_strings[errc_value];

    int windows_error = -1;
    // Reverse-map errc values to Windows error codes
    auto error_code = static_cast<errc> (errc_value);
    switch (error_code)
    {
    case errc::address_family_not_supported:    windows_error = WSAEAFNOSUPPORT; break;
    case errc::address_in_use:                  windows_error = WSAEADDRINUSE; break;
    case errc::address_not_available:           windows_error = WSAEADDRNOTAVAIL; break;
    case errc::already_connected:               windows_error = WSAEISCONN; break;
    case errc::bad_address:                     windows_error = WSAEFAULT; break;
    case errc::bad_file_descriptor:             windows_error = WSAEBADF; break;
    case errc::connection_aborted:              windows_error = WSAECONNABORTED; break;
    case errc::connection_already_in_progress:  windows_error = WSAEALREADY; break;
    case errc::connection_refused:              windows_error = WSAECONNREFUSED; break;
    case errc::connection_reset:                windows_error = WSAECONNRESET; break;
    case errc::cross_device_link:               windows_error = ERROR_NOT_SAME_DEVICE; break;
    case errc::destination_address_required:    windows_error = WSAEDESTADDRREQ; break;
    case errc::device_or_resource_busy:         windows_error = ERROR_BUSY; break;
    case errc::directory_not_empty:             windows_error = ERROR_DIR_NOT_EMPTY; break;
    case errc::file_exists:                     windows_error = ERROR_FILE_EXISTS; break;
    case errc::filename_too_long:               windows_error = ERROR_BUFFER_OVERFLOW; break;
    case errc::function_not_supported:          windows_error = ERROR_INVALID_FUNCTION; break;
    case errc::host_unreachable:                windows_error = WSAEHOSTUNREACH; break;
    case errc::interrupted:                     windows_error = WSAEINTR; break;
    case errc::invalid_argument:                windows_error = ERROR_INVALID_NAME; break;
    case errc::io_error:                        windows_error = ERROR_READ_FAULT; break;
    case errc::message_size:                    windows_error = WSAEMSGSIZE; break;
    case errc::network_down:                    windows_error = WSAENETDOWN; break;
    case errc::network_reset:                   windows_error = WSAENETRESET; break;
    case errc::network_unreachable:             windows_error = WSAENETUNREACH; break;
    case errc::no_buffer_space:                 windows_error = WSAENOBUFS; break;
    case errc::no_lock_available:               windows_error = ERROR_LOCK_VIOLATION; break;
    case errc::no_protocol_option:              windows_error = WSAENOPROTOOPT; break;
    case errc::no_space_on_device:              windows_error = ERROR_DISK_FULL; break;
    case errc::no_such_device:                  windows_error = ERROR_INVALID_DRIVE; break;
    case errc::no_such_file_or_directory:       windows_error = ERROR_FILE_NOT_FOUND; break;
    case errc::not_a_socket:                    windows_error = WSAENOTSOCK; break;
    case errc::not_connected:                   windows_error = WSAENOTCONN; break;
    case errc::not_enough_memory:               windows_error = ERROR_NOT_ENOUGH_MEMORY; break;
    case errc::operation_canceled:              windows_error = ERROR_OPERATION_ABORTED; break;
    case errc::operation_in_progress:           windows_error = WSAEINPROGRESS; break;
    case errc::operation_not_supported:         windows_error = WSAEOPNOTSUPP; break;
    case errc::operation_would_block:           windows_error = WSAEWOULDBLOCK; break;
    case errc::permission_denied:               windows_error = ERROR_ACCESS_DENIED; break;
    case errc::protocol_not_supported:          windows_error = WSAEPROTONOSUPPORT; break;
    case errc::resource_unavailable_try_again:  windows_error = ERROR_NOT_READY; break;
    case errc::timed_out:                       windows_error = WSAETIMEDOUT; break;
    case errc::too_many_files_open_in_system:   windows_error = ERROR_TOO_MANY_OPEN_FILES; break;
    case errc::too_many_files_open:             windows_error = ERROR_TOO_MANY_OPEN_FILES; break;
    case errc::wrong_protocol_type:             windows_error = WSAEPROTOTYPE; break;

    // Below: errors without good map to Windows errors
    case errc::argument_list_too_long:
    case errc::argument_out_of_domain:
    case errc::bad_message:
    case errc::broken_pipe:
    case errc::executable_format_error:
    case errc::file_too_large:
    case errc::identifier_removed:
    case errc::illegal_byte_sequence:
    case errc::inappropriate_io_control_operation:
    case errc::invalid_seek:
    case errc::is_a_directory:
    case errc::no_child_process:
    case errc::no_link:
    case errc::no_message_available:
    case errc::no_message:
    case errc::no_stream_resources:
    case errc::no_such_device_or_address:
    case errc::no_such_process:
    case errc::not_a_directory:
    case errc::not_a_stream:
    case errc::not_supported:
    case errc::operation_not_permitted:
    case errc::owner_dead:
    case errc::protocol_error:
    case errc::read_only_file_system:
    case errc::resource_deadlock_would_occur:
    case errc::result_out_of_range:
    case errc::state_not_recoverable:
    case errc::stream_timeout:
    case errc::text_file_busy:
    case errc::too_many_links:
    case errc::too_many_symbolic_link_levels:
    case errc::value_too_large:
      break;
    }

    char message_buf[32*1024];
    if (windows_error >= 0)
    {
      _Winerror_message (static_cast<unsigned long> (windows_error), message_buf, sizeof(message_buf));
    }
    else
    {
      // Fall back to numeric value
      snprintf (message_buf, sizeof(message_buf), "error %d", static_cast<int> (errc_value));
    }

    auto new_str = _strdup (message_buf); // Yes, that's a leak, but we don't care
    error_strings[errc_value] = new_str;
    return new_str;
  }

  // Helper for _Winerror_map below
  static errc winerror_map_errc (int code)
  {
    // These mappings are based on Boost.System.
    switch (code)
    {
    case 0: return static_cast<errc> (0);

    case ERROR_ACCESS_DENIED:         return errc::permission_denied;
    case ERROR_ALREADY_EXISTS:        return errc::file_exists;
    case ERROR_BAD_UNIT:              return errc::no_such_device;
    case ERROR_BUFFER_OVERFLOW:       return errc::filename_too_long;
    case ERROR_BUSY:                  return errc::device_or_resource_busy;
    case ERROR_BUSY_DRIVE:            return errc::device_or_resource_busy;
    case ERROR_CANNOT_MAKE:           return errc::permission_denied;
    case ERROR_CANTOPEN:              return errc::io_error;
    case ERROR_CANTREAD:              return errc::io_error;
    case ERROR_CANTWRITE:             return errc::io_error;
    case ERROR_CURRENT_DIRECTORY:     return errc::permission_denied;
    case ERROR_DEV_NOT_EXIST:         return errc::no_such_device;
    case ERROR_DEVICE_IN_USE:         return errc::device_or_resource_busy;
    case ERROR_DIR_NOT_EMPTY:         return errc::directory_not_empty;
    case ERROR_DIRECTORY:             return errc::invalid_argument; // WinError.h: "The directory name is invalid"
    case ERROR_DISK_FULL:             return errc::no_space_on_device;
    case ERROR_FILE_EXISTS:           return errc::file_exists;
    case ERROR_FILE_NOT_FOUND:        return errc::no_such_file_or_directory;
    case ERROR_HANDLE_DISK_FULL:      return errc::no_space_on_device;
    case ERROR_INVALID_ACCESS:        return errc::permission_denied;
    case ERROR_INVALID_DRIVE:         return errc::no_such_device;
    case ERROR_INVALID_FUNCTION:      return errc::function_not_supported;
    case ERROR_INVALID_HANDLE:        return errc::invalid_argument;
    case ERROR_INVALID_NAME:          return errc::invalid_argument;
    case ERROR_LOCK_VIOLATION:        return errc::no_lock_available;
    case ERROR_LOCKED:                return errc::no_lock_available;
    case ERROR_NEGATIVE_SEEK:         return errc::invalid_argument;
    case ERROR_NOACCESS:              return errc::permission_denied;
    case ERROR_NOT_ENOUGH_MEMORY:     return errc::not_enough_memory;
    case ERROR_NOT_READY:             return errc::resource_unavailable_try_again;
    case ERROR_NOT_SAME_DEVICE:       return errc::cross_device_link;
    case ERROR_OPEN_FAILED:           return errc::io_error;
    case ERROR_OPEN_FILES:            return errc::device_or_resource_busy;
    case ERROR_OPERATION_ABORTED:     return errc::operation_canceled;
    case ERROR_OUTOFMEMORY:           return errc::not_enough_memory;
    case ERROR_PATH_NOT_FOUND:        return errc::no_such_file_or_directory;
    case ERROR_READ_FAULT:            return errc::io_error;
    case ERROR_RETRY:                 return errc::resource_unavailable_try_again;
    case ERROR_SEEK:                  return errc::io_error;
    case ERROR_SHARING_VIOLATION:     return errc::permission_denied;
    case ERROR_TOO_MANY_OPEN_FILES:   return errc::too_many_files_open;
    case ERROR_WRITE_FAULT:           return errc::io_error;
    case ERROR_WRITE_PROTECT:         return errc::permission_denied;
    case WSAEACCES:                   return errc::permission_denied;
    case WSAEADDRINUSE:               return errc::address_in_use;
    case WSAEADDRNOTAVAIL:            return errc::address_not_available;
    case WSAEAFNOSUPPORT:             return errc::address_family_not_supported;
    case WSAEALREADY:                 return errc::connection_already_in_progress;
    case WSAEBADF:                    return errc::bad_file_descriptor;
    case WSAECONNABORTED:             return errc::connection_aborted;
    case WSAECONNREFUSED:             return errc::connection_refused;
    case WSAECONNRESET:               return errc::connection_reset;
    case WSAEDESTADDRREQ:             return errc::destination_address_required;
    case WSAEFAULT:                   return errc::bad_address;
    case WSAEHOSTUNREACH:             return errc::host_unreachable;
    case WSAEINPROGRESS:              return errc::operation_in_progress;
    case WSAEINTR:                    return errc::interrupted;
    case WSAEINVAL:                   return errc::invalid_argument;
    case WSAEISCONN:                  return errc::already_connected;
    case WSAEMFILE:                   return errc::too_many_files_open;
    case WSAEMSGSIZE:                 return errc::message_size;
    case WSAENAMETOOLONG:             return errc::filename_too_long;
    case WSAENETDOWN:                 return errc::network_down;
    case WSAENETRESET:                return errc::network_reset;
    case WSAENETUNREACH:              return errc::network_unreachable;
    case WSAENOBUFS:                  return errc::no_buffer_space;
    case WSAENOPROTOOPT:              return errc::no_protocol_option;
    case WSAENOTCONN:                 return errc::not_connected;
    case WSAENOTSOCK:                 return errc::not_a_socket;
    case WSAEOPNOTSUPP:               return errc::operation_not_supported;
    case WSAEPROTONOSUPPORT:          return errc::protocol_not_supported;
    case WSAEPROTOTYPE:               return errc::wrong_protocol_type;
    case WSAETIMEDOUT:                return errc::timed_out;
    case WSAEWOULDBLOCK:              return errc::operation_would_block;
    }

     // Default error
     return errc::invalid_argument;
  }

  // Convert a Windows error code (ERROR_something) to it's errc equivalent
  int _Winerror_map (int code)
  {
    return static_cast<int> (winerror_map_errc (code));
  }
} // namespace std
