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

#ifndef OBPROXY_CLIENT_UTILS_H
#define OBPROXY_CLIENT_UTILS_H

#include "iocore/net/ob_net.h"
#include "proxy/mysqllib/ob_mysql_transaction_analyzer.h"
#include "proxy/mysqllib/ob_resultset_fetcher.h"

namespace oceanbase
{
namespace obproxy
{
namespace proxy
{
class ObMysqlRequestParam
{
public:
  ObMysqlRequestParam() : sql_(), is_deep_copy_(false), is_user_idc_name_set_(false),
                          need_print_trace_stat_(false), current_idc_name_() {};
  explicit ObMysqlRequestParam(const char *sql)
    : sql_(sql), is_deep_copy_(false), is_user_idc_name_set_(false),
      need_print_trace_stat_(false), current_idc_name_() {};
  ObMysqlRequestParam(const char *sql, const ObString &idc_name)
    : sql_(sql), is_deep_copy_(false), is_user_idc_name_set_(true),
      need_print_trace_stat_(true), current_idc_name_(idc_name) {};
  void reset();
  void reset_sql();
  bool is_valid() const { return !sql_.empty(); }
  int deep_copy(const ObMysqlRequestParam &other);
  int deep_copy_sql(const common::ObString &sql);
  TO_STRING_KV(K_(sql), K_(is_deep_copy), K_(current_idc_name), K_(is_user_idc_name_set), K_(need_print_trace_stat));

  common::ObString sql_;
  bool is_deep_copy_;
  bool is_user_idc_name_set_;
  bool need_print_trace_stat_;
  common::ObString current_idc_name_;
  char current_idc_name_buf_[OB_PROXY_MAX_IDC_NAME_LENGTH];
};

class ObClientMysqlResp
{
public:
  ObClientMysqlResp()
    : is_inited_(false), analyzer_(), mysql_resp_(), rs_fetcher_(NULL),
      response_buf_(NULL), response_reader_(NULL) {}
  ~ObClientMysqlResp() { destroy(); }

  int init();
  void reset();
  void destroy();

  int analyze_resp(const obmysql::ObMySQLCmd cmd);
  bool is_error_resp() const { return mysql_resp_.get_analyze_result().is_error_resp(); }
  bool is_ok_resp() const { return mysql_resp_.get_analyze_result().is_ok_resp(); }
  bool is_resultset_resp() const { return mysql_resp_.get_analyze_result().is_eof_resp(); }
  bool is_resp_completed() const { return analyzer_.is_resp_completed(); }
  uint16_t get_err_code() const { return mysql_resp_.get_analyze_result().get_error_code(); }
  common::ObString get_err_msg() const {return mysql_resp_.get_analyze_result().get_error_pkt().get_message();}
  int get_affected_rows(int64_t &affected_row);

  int get_resultset_fetcher(ObResultSetFetcher *&result);
  event::ObMIOBuffer *get_resp_miobuf() { return response_buf_; }
  event::ObIOBufferReader *get_response_reader() { return response_reader_; }

  void consume_resp_buf();

private:
  bool is_inited_;
  ObMysqlTransactionAnalyzer analyzer_;
  ObMysqlResp mysql_resp_;
  ObResultSetFetcher *rs_fetcher_;

  event::ObMIOBuffer *response_buf_;
  event::ObIOBufferReader *response_reader_;

  DISALLOW_COPY_AND_ASSIGN(ObClientMysqlResp);
};

inline void ObClientMysqlResp::consume_resp_buf()
{
  if (NULL != response_reader_) {
    int ret = common::OB_SUCCESS;
    if (OB_FAIL(response_reader_->consume(response_reader_->read_avail()))) {
      PROXY_LOG(WARN, "fail to consume ", K(ret));
    }
  }
}

class ObMysqlResultHandler
{
public:
  ObMysqlResultHandler() : resp_(NULL), rs_fetcher_(NULL) {}
  ~ObMysqlResultHandler() { destroy(); }

  void set_resp(ObClientMysqlResp *resp) { resp_ = resp; }
  int next();
  int get_int(const char *col_name, int64_t &int_val) const;
  int get_uint(const char *col_name, uint64_t &int_val) const;
  int get_bool(const char *col_name, bool &bool_val) const;
  int get_varchar(const char *col_name, common::ObString &varchar_val) const;
  int get_double(const char *col_name, double &double_val) const;

  bool is_valid() { return (NULL != resp_); }
  int64_t to_string(char *buf, const int64_t buf_len) const;

private:
  int get_resultset_fetcher(ObResultSetFetcher *&rs_fetcher);
  void destroy();

private:
  ObClientMysqlResp *resp_;
  ObResultSetFetcher *rs_fetcher_;
  DISALLOW_COPY_AND_ASSIGN(ObMysqlResultHandler);
};

inline int ObMysqlResultHandler::get_int(const char *col_name, int64_t &int_val) const
{
  return (NULL == rs_fetcher_) ? (common::OB_INNER_STAT_ERROR) : (rs_fetcher_->get_int(col_name, int_val));
}

inline int ObMysqlResultHandler::get_uint(const char *col_name, uint64_t &int_val) const
{
  return (NULL == rs_fetcher_) ? (common::OB_INNER_STAT_ERROR) : (rs_fetcher_->get_uint(col_name, int_val));
}

inline int ObMysqlResultHandler::get_bool(const char *col_name, bool &bool_val) const
{
  return (NULL == rs_fetcher_) ? (common::OB_INNER_STAT_ERROR) : (rs_fetcher_->get_bool(col_name, bool_val));
}

inline int ObMysqlResultHandler::get_varchar(const char *col_name, common::ObString &varchar_val) const
{
  return (NULL == rs_fetcher_) ? (common::OB_INNER_STAT_ERROR) : (rs_fetcher_->get_varchar(col_name, varchar_val));
}

inline int ObMysqlResultHandler::get_double(const char *col_name, double &double_val) const
{
  return (NULL == rs_fetcher_) ? (common::OB_INNER_STAT_ERROR) : (rs_fetcher_->get_double(col_name, double_val));
}

class ObClientReuqestInfo
{
public:
   ObClientReuqestInfo()
    : user_name_(), database_name_(), password_(),
      request_param_(), name_(NULL), name_len_(0),
      need_skip_stage2_(false) {}
  ~ObClientReuqestInfo() { reset(); }

  void reset();
  void reset_names();
  void reset_sql();
  void set_need_skip_stage2(bool is_need) {
    need_skip_stage2_ = is_need;
  }
  bool is_need_skip_stage2() { return need_skip_stage2_;}
  int set_names(const common::ObString &user_name,
                const common::ObString &password,
                const common::ObString &database_name);
  int set_request_param(const ObMysqlRequestParam &request_param);

  const common::ObString &get_user_name() const { return user_name_; }
  const common::ObString &get_database_name() const { return database_name_; }
  const common::ObString &get_password() const { return password_; }
  const common::ObString &get_request_sql() const { return request_param_.sql_; }
  ObMysqlRequestParam &get_request_param() { return request_param_; }

  int64_t to_string(char *buf, const int64_t buf_len) const;

private:
  common::ObString user_name_;
  common::ObString database_name_;
  common::ObString password_;
  ObMysqlRequestParam request_param_;
  char *name_;
  int64_t name_len_;
  bool need_skip_stage2_;

  DISALLOW_COPY_AND_ASSIGN(ObClientReuqestInfo);
};

class ObClientUtils
{
public:
  static const uint32_t CAPABILITY_FLAGS = 0x007FA685;
  static int get_auth_password(const common::ObString &raw_pwd, const common::ObString &scramble,
                               char *pwd_buf, int64_t buf_len, int64_t &copy_len);
  static int get_auth_password_from_stage1(const ObString &stage1,
                                           const common::ObString &scramble,
                                           char *pwd_buf, int64_t buf_len, int64_t &copy_len);
  static int get_scramble(event::ObIOBufferReader *response_reader, char *buf,
                          const int64_t buf_len, int64_t &copy_len);

  static int build_handshake_response_packet(ObClientMysqlResp *handshake,
                                             ObClientReuqestInfo *info,
                                             event::ObMIOBuffer *handshake_resp_buf);
};

} // end of namespace proxy
} // end of namespace obproxy
} // end of namespace oceanbase

#endif // OBPROXY_CLIENT_UTILS_H
