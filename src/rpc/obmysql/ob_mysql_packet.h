/**
 * Copyright (c) 2021 OceanBase
 * OceanBase Database Proxy(ODP) is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef _OB_MYSQL_PACKET_H_
#define _OB_MYSQL_PACKET_H_

#include "rpc/ob_packet.h"

namespace oceanbase
{
namespace obmysql
{

#define OBPROXY_NEW_MYSQL_CMD_START 161
#define OBPROXY_MYSQL_CMD_START 240

enum ObMySQLCmd
{
  OB_MYSQL_COM_SLEEP,
  OB_MYSQL_COM_QUIT,
  OB_MYSQL_COM_INIT_DB,
  OB_MYSQL_COM_QUERY,
  OB_MYSQL_COM_FIELD_LIST,

  OB_MYSQL_COM_CREATE_DB,
  OB_MYSQL_COM_DROP_DB,
  OB_MYSQL_COM_REFRESH,
  OB_MYSQL_COM_SHUTDOWN,
  OB_MYSQL_COM_STATISTICS,

  OB_MYSQL_COM_PROCESS_INFO,
  OB_MYSQL_COM_CONNECT,
  OB_MYSQL_COM_PROCESS_KILL,
  OB_MYSQL_COM_DEBUG,
  OB_MYSQL_COM_PING,

  OB_MYSQL_COM_TIME,
  OB_MYSQL_COM_DELAYED_INSERT,
  OB_MYSQL_COM_CHANGE_USER,
  OB_MYSQL_COM_BINLOG_DUMP,

  OB_MYSQL_COM_TABLE_DUMP,
  OB_MYSQL_COM_CONNECT_OUT,
  OB_MYSQL_COM_REGISTER_SLAVE,

  OB_MYSQL_COM_STMT_PREPARE,
  OB_MYSQL_COM_STMT_EXECUTE,
  OB_MYSQL_COM_STMT_SEND_LONG_DATA,
  OB_MYSQL_COM_STMT_CLOSE,

  OB_MYSQL_COM_STMT_RESET,
  OB_MYSQL_COM_SET_OPTION,
  OB_MYSQL_COM_STMT_FETCH,
  OB_MYSQL_COM_DAEMON,

  OB_MYSQL_COM_BINLOG_DUMP_GTID,

  // COM_RESET_CONNECTION,
  OB_MYSQL_COM_STMT_PREPARE_EXECUTE = OBPROXY_NEW_MYSQL_CMD_START,
  OB_MYSQL_COM_STMT_SEND_PIECE_DATA,
  OB_MYSQL_COM_STMT_GET_PIECE_DATA,
  OB_MYSQL_COM_END,



  // for obproxy
  // OB_MYSQL_COM_DELETE_SESSION is not a standard mysql package type. This is a package used to process delete session
  // When the connection is disconnected, the session needs to be deleted, but at this time it may not be obtained in the callback function disconnect
  // Session lock, at this time, an asynchronous task will be added to the obmysql queue
  OB_MYSQL_COM_DELETE_SESSION = OBPROXY_MYSQL_CMD_START,
  // OB_MYSQL_COM_HANDSHAKE and OB_MYSQL_COM_LOGIN are not standard mysql package types, they are used in ObProxy
  // OB_MYSQL_COM_HANDSHAKE represents client---->on_connect && observer--->hand shake or error
  // OB_MYSQL_COM_LOGIN represents client---->hand shake response && observer---> ok or error
  OB_MYSQL_COM_HANDSHAKE,
  OB_MYSQL_COM_LOGIN,
  OB_MYSQL_COM_MAX_NUM
};

union ObServerStatusFlags
{
  ObServerStatusFlags() : flags_(0) {}
  explicit ObServerStatusFlags(uint16_t flag) : flags_(flag) {}
  uint16_t flags_;
  //ref:http://dev.mysql.com/doc/internals/en/status-flags.html
  struct ServerStatusFlags
  {
    uint16_t OB_SERVER_STATUS_IN_TRANS:             1;  // a transaction is active
    uint16_t OB_SERVER_STATUS_AUTOCOMMIT:           1;  // auto-commit is enabled
    uint16_t OB_SERVER_STATUS_RESERVED:             1;  // reserved, not used now
    uint16_t OB_SERVER_MORE_RESULTS_EXISTS:         1;
    uint16_t OB_SERVER_STATUS_NO_GOOD_INDEX_USED:   1;
    uint16_t OB_SERVER_STATUS_NO_INDEX_USED:        1;
    // used by Binary Protocol Resultset to signal that
    // OB_MYSQL_COM_STMT_FETCH has to be used to fetch the row-data.
    uint16_t OB_SERVER_STATUS_CURSOR_EXISTS:        1;
    uint16_t OB_SERVER_STATUS_LAST_ROW_SENT:        1;
    uint16_t OB_SERVER_STATUS_DB_DROPPED:           1;
    uint16_t OB_SERVER_STATUS_NO_BACKSLASH_ESCAPES: 1;
    uint16_t OB_SERVER_STATUS_METADATA_CHANGED:     1;
    uint16_t OB_SERVER_QUERY_WAS_SLOW:              1;
    uint16_t OB_SERVER_PS_OUT_PARAMS:               1;
    uint16_t OB_SERVER_STATUS_IN_TRANS_READONLY:    1;  // in a read-only transaction
    uint16_t OB_SERVER_SESSION_STATE_CHANGED:       1;  // connection state information has changed
  } status_flags_;
};

union ObMySQLCapabilityFlags
{
  ObMySQLCapabilityFlags() : capability_(0) {}
  explicit ObMySQLCapabilityFlags(uint32_t cap) : capability_(cap) {}
  uint32_t capability_;
  //ref:http://dev.mysql.com/doc/internals/en/capability-flags.html
  struct CapabilityFlags
  {
    uint32_t OB_CLIENT_LONG_PASSWORD:                   1;
    uint32_t OB_CLIENT_FOUND_ROWS:                      1;
    uint32_t OB_CLIENT_LONG_FLAG:                       1;
    uint32_t OB_CLIENT_CONNECT_WITH_DB:                 1;
    uint32_t OB_CLIENT_NO_SCHEMA:                       1;
    uint32_t OB_CLIENT_COMPRESS:                        1;
    uint32_t OB_CLIENT_ODBC:                            1;
    uint32_t OB_CLIENT_LOCAL_FILES:                     1;
    uint32_t OB_CLIENT_IGNORE_SPACE:                    1;
    uint32_t OB_CLIENT_PROTOCOL_41:                     1;
    uint32_t OB_CLIENT_INTERACTIVE:                     1;
    uint32_t OB_CLIENT_SSL:                             1;
    uint32_t OB_CLIENT_IGNORE_SIGPIPE:                  1;
    uint32_t OB_CLIENT_TRANSACTIONS:                    1;
    uint32_t OB_CLIENT_RESERVED:                        1;
    uint32_t OB_CLIENT_SECURE_CONNECTION:               1;
    uint32_t OB_CLIENT_MULTI_STATEMENTS:                1;
    uint32_t OB_CLIENT_MULTI_RESULTS:                   1;
    uint32_t OB_CLIENT_PS_MULTI_RESULTS:                1;
    uint32_t OB_CLIENT_PLUGIN_AUTH:                     1;
    uint32_t OB_CLIENT_CONNECT_ATTRS:                   1;
    uint32_t OB_CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA:  1;
    uint32_t OB_CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS:    1;
    uint32_t OB_CLIENT_SESSION_TRACK:                   1;
    uint32_t OB_CLIENT_DEPRECATE_EOF:                   1;
    uint32_t OB_CLIENT_RESERVED_NOT_USE:                2;
    uint32_t OB_CLIENT_SUPPORT_ORACLE_MODE:             1;
    uint32_t OB_CLIENT_RESERVED_NOT_USE_V2:             4;
  } cap_flags_;
};

enum ObClientCapabilityPos
{
  OB_CLIENT_LONG_PASSWORD_POS = 0,
  OB_CLIENT_FOUND_ROWS_POS,
  OB_CLIENT_LONG_FLAG_POS,
  OB_CLIENT_CONNECT_WITH_DB_POS,
  OB_CLIENT_NO_SCHEMA_POS,
  OB_CLIENT_COMPRESS_POS,
  OB_CLIENT_ODBC_POS,
  OB_CLIENT_LOCAL_FILES_POS,
  OB_CLIENT_IGNORE_SPACE_POS,
  OB_CLIENT_PROTOCOL_41_POS,
  OB_CLIENT_INTERACTIVE_POS,
  OB_CLIENT_SSL_POS,
  OB_CLIENT_IGNORE_SIGPIPE_POS,
  OB_CLIENT_TRANSACTION_POS,
  OB_CLIENT_RESERVED_POS,
  OB_CLIENT_SECURE_CONNECTION_POS,
  OB_CLIENT_MULTI_STATEMENTS_POS,
  OB_CLIENT_MULTI_RESULTS_POS,
  OB_CLIENT_PS_MULTI_RESULTS_POS,
  OB_CLIENT_PLUGIN_AUTH_POS,
  OB_CLIENT_CONNECT_ATTRS_POS,
  OB_CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA_POS,
  OB_CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS_POS,
  OB_CLIENT_SESSION_TRACK_POS,
  OB_CLIENT_DEPRECATE_EOF_POS,
};

enum ObServerStatusFlagsPos
{
  OB_SERVER_STATUS_IN_TRANS_POS = 0,
  OB_SERVER_STATUS_AUTOCOMMIT_POS,
  OB_SERVER_STATUS_RESERVED_POS,
  OB_SERVER_MORE_RESULTS_EXISTS_POS,
  OB_SERVER_STATUS_NO_GOOD_INDEX_USED_POS,
  OB_SERVER_STATUS_NO_INDEX_USED_POS,
  OB_SERVER_STATUS_CURSOR_EXISTS_POS,
  OB_SERVER_STATUS_LAST_ROW_SENT_POS,
  OB_SERVER_STATUS_DB_DROPPED_POS,
  OB_SERVER_STATUS_NO_BACKSLASH_ESCAPES_POS,
  OB_SERVER_STATUS_METADATA_CHANGED_POS,
  OB_SERVER_QUERY_WAS_SLOW_POS,
  OB_SERVER_PS_OUT_PARAMS_POS,
  OB_SERVER_STATUS_IN_TRANS_READONLY_POS,
  OB_SERVER_SESSION_STATE_CHANGED_POS,
};

char const *get_mysql_cmd_str(ObMySQLCmd mysql_cmd);

//http://dev.mysql.com/doc/refman/5.7/en/information-functions.html#function_current-user
enum ObInformationFunctions
{
  BENCHMARK_FUNC = 0,     // Repeatedly execute an expression
  CHARSET_FUNC,           // Return the character set of the argument
  COERCIBILITY_FUNC,      // Return the collation coercibility value of the string argument
  COLLATION_FUNC,         // Return the collation of the string argument
  CONNECTION_ID_FUNC,     // Return the connection ID (thread ID) for the connection
  CURRENT_USER_FUNC,      // The authenticated user name and host name
  DATABASE_FUNC,          // Return the default (current) database name
  FOUND_ROWS_FUNC,        // For a SELECT with a LIMIT clause, the number of rows
                          // that would be returned were there no LIMIT clause
  LAST_INSERT_ID_FUNC,    // Value of the AUTOINCREMENT column for the last INSERT
  ROW_COUNT_FUNC,         // The number of rows updated
  SCHEAM_FUNC,            // Synonym for DATABASE()
  SESSION_USER_FUNC,      // Synonym for USER()
  SYSTEM_USER_FUNC,       // Synonym for USER()
  USER_FUNC,              // The user name and host name provided by the client
  VERSION_FUNC,           // Return a string that indicates the MySQL server version
  MAX_INFO_FUNC           // end
};

char const *get_info_func_name(const ObInformationFunctions func);

template<class K, class V>
class ObCommonKV
{
public:
  ObCommonKV() : key_(), value_() {}
  void reset()
  {
    key_.reset();
    value_.reset();
  }
  K key_;
  V value_;
  TO_STRING_KV(K_(key), K_(value));
};

typedef ObCommonKV<common::ObString, common::ObString> ObStringKV;

static const int64_t MAX_STORE_LENGTH = 9;

class ObMySQLPacketHeader
{
public:
  ObMySQLPacketHeader()
      : len_(0), seq_(0)
  { }

  TO_STRING_KV("length", len_, "sequence", seq_);

public:
  uint32_t len_;         // MySQL packet length not include packet header
  uint8_t  seq_;         // MySQL packet sequence
};

class ObMySQLPacket
    : public rpc::ObPacket
{
public:
  ObMySQLPacket()
      : hdr_(), cdata_(NULL)
  {}
  virtual ~ObMySQLPacket() {}

  static int encode_packet(char *buf, int64_t &len, int64_t &pos, const ObMySQLPacket &pkt);
  inline void set_seq(uint8_t seq);
  inline uint8_t get_seq(void) const;
  inline void set_content(const char *content, uint32_t len);

  virtual int64_t get_serialize_size() const;
  int encode(char *buffer, int64_t length, int64_t &pos) const;
  virtual int decode() { return common::OB_NOT_SUPPORTED; }

  VIRTUAL_TO_STRING_KV("header", hdr_);

protected:
  virtual int serialize(char*, const int64_t, int64_t&) const
  {
    return common::OB_NOT_SUPPORTED;
  }

public:
  static const int64_t HANDSHAKE_RESPONSE_RESERVED_SIZE = 23;
    // 4: capability flags
    // 4: max-packet size
    // 1: character set
    static const int64_t HANDSHAKE_RESPONSE_MIN_SIZE = 9 + HANDSHAKE_RESPONSE_RESERVED_SIZE;
    // 4: capability flags
    static const int64_t JDBC_SSL_MIN_SIZE = 4;
    // 2: capability flags
    static const int64_t MIN_CAPABILITY_SIZE = 2;

protected:
  ObMySQLPacketHeader hdr_;
  const char *cdata_;
};

class ObMySQLRawPacket
    : public ObMySQLPacket
{
public:
  ObMySQLRawPacket() : ObMySQLPacket(), cmd_(OB_MYSQL_COM_MAX_NUM) {}
  explicit ObMySQLRawPacket(obmysql::ObMySQLCmd cmd) : ObMySQLPacket(), cmd_(cmd) {}

  inline void set_cmd(ObMySQLCmd cmd);
  inline ObMySQLCmd get_cmd() const;

  inline const char *get_cdata() const;
  inline uint32_t get_clen() const;
  int encode_packet_meta(char *buf, int64_t &len, int64_t &pos) const;

  virtual int64_t get_serialize_size() const;
  TO_STRING_KV("header", hdr_);
protected:
  virtual int serialize(char*, const int64_t, int64_t&) const;

private:
  void set_len(uint32_t len);
private:
  ObMySQLCmd cmd_;
};

void ObMySQLPacket::set_seq(uint8_t seq)
{
  hdr_.seq_ = seq;
}

uint8_t ObMySQLPacket::get_seq(void) const
{
  return hdr_.seq_;
}

void ObMySQLPacket::set_content(const char *content, uint32_t len)
{
  cdata_ = content;
  hdr_.len_ = len;
}

void ObMySQLRawPacket::set_cmd(ObMySQLCmd cmd)
{
  cmd_ = cmd;
}

ObMySQLCmd ObMySQLRawPacket::get_cmd() const
{
  return cmd_;
}

inline const char *ObMySQLRawPacket::get_cdata() const
{
  return cdata_;
}

inline uint32_t ObMySQLRawPacket::get_clen() const
{
  return hdr_.len_;
}

} // end of namespace obmysql
} // end of namespace oceanbase

#endif /* _OB_MYSQL_PACKET_H_ */
