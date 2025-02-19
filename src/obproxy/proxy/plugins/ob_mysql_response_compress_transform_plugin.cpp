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

#include "ob_mysql_response_compress_transform_plugin.h"
#include "obproxy/obutils/ob_resource_pool_processor.h"

using namespace oceanbase::common;
using namespace oceanbase::obproxy::event;

namespace oceanbase
{
namespace obproxy
{
namespace proxy
{

ObMysqlResponseCompressTransformPlugin *ObMysqlResponseCompressTransformPlugin::alloc(ObApiTransaction &transaction)
{
  return op_reclaim_alloc_args(ObMysqlResponseCompressTransformPlugin, transaction);
}

ObMysqlResponseCompressTransformPlugin::ObMysqlResponseCompressTransformPlugin(ObApiTransaction &transaction)
  : ObTransformationPlugin(transaction, ObTransformationPlugin::RESPONSE_TRANSFORMATION),
    req_seq_(0), request_id_(0), server_sessid_(0),
    local_reader_(NULL), local_transfer_reader_(NULL), compress_analyzer_(),
    compress_ob20_analyzer_(), analyzer_(NULL)
{
  ObProxyProtocol ob_proxy_protocol = sm_->use_compression_protocol();
  if (ob_proxy_protocol == PROTOCOL_CHECKSUM) {
    analyzer_ = &compress_analyzer_;
  } else if (ob_proxy_protocol == PROTOCOL_OB20) {
    analyzer_ = &compress_ob20_analyzer_;
  }

  // get request seq
  req_seq_ = sm_->get_request_seq();
  request_id_ = sm_->get_server_session()->get_server_request_id();
  server_sessid_ = sm_->get_server_session()->get_server_sessid();
  PROXY_API_LOG(DEBUG, "ObMysqlResponseCompressTransformPlugin born", K(ob_proxy_protocol), K_(req_seq),
                K_(request_id), K_(server_sessid), K(this));
}

void ObMysqlResponseCompressTransformPlugin::destroy()
{
  PROXY_API_LOG(DEBUG, "ObMysqlResponseCompressTransformPlugin destroy", K(this));
  ObTransformationPlugin::destroy();
  analyzer_->reset();
  op_reclaim_free(this);
}

int ObMysqlResponseCompressTransformPlugin::consume(event::ObIOBufferReader *reader)
{
  PROXY_API_LOG(DEBUG, "ObMysqlResponseCompressTransformPlugin::consume happen");
  int ret = OB_SUCCESS;
  int64_t consume_size = 0;
  int64_t produce_size = 0;
  int64_t read_avail = 0;

  if (NULL == local_reader_) {
    local_reader_ = reader->clone();
  }

  if (!analyzer_->is_inited()) {
    const obmysql::ObMySQLCmd cmd = sm_->trans_state_.trans_info_.client_request_.get_packet_meta().cmd_;
    const bool extra_ok_packet_for_stats_enabled = sm_->is_extra_ok_packet_for_stats_enabled();
    // should decomrpess the data
    if (OB_FAIL(analyzer_->init(req_seq_, ObMysqlCompressAnalyzer::DECOMPRESS_MODE,
                cmd, extra_ok_packet_for_stats_enabled, req_seq_, request_id_, server_sessid_))) {
      PROXY_API_LOG(WARN, "fail to init compress analyzer", K_(req_seq), K_(request_id), K_(server_sessid),
                    K(cmd), K(extra_ok_packet_for_stats_enabled), K(ret));
    }
  }

  ObRespAnalyzeResult &analyze_result = sm_->trans_state_.trans_info_.server_response_.get_analyze_result();
  if (OB_SUCC(ret)) {
    if (NULL == local_transfer_reader_) {
      local_transfer_reader_ = analyzer_->get_transfer_miobuf()->alloc_reader();
    }

    ObMysqlResp &server_response = sm_->trans_state_.trans_info_.server_response_;
    // the ObMysqlResp will be use both by tunnel and this class, and if tunnel analyze finished,
    // is_resp_completed_ will set true, here we set back to false to ensure compress_analyzer
    // work happy.
    if (analyze_result.is_resp_completed()) {
      analyze_result.is_resp_completed_ = false;
    }

    int64_t plugin_decompress_response_begin = sm_->get_based_hrtime();

    read_avail = local_reader_->read_avail();
    if (OB_FAIL(analyzer_->analyze_response(*local_reader_, &server_response))) {
      PROXY_API_LOG(ERROR, "fail to analyze mysql compress", K_(local_reader), K(ret));
    } else if (OB_FAIL(local_reader_->consume(read_avail))) {// consume all input data anyway
      PROXY_API_LOG(WARN, "fail to consume", K(consume_size), K(ret));
    } else {
      consume_size = local_transfer_reader_->read_avail();

      int64_t plugin_decompress_response_end = sm_->get_based_hrtime();
      sm_->cmd_time_stats_.plugin_decompress_response_time_ +=
        milestone_diff(plugin_decompress_response_begin, plugin_decompress_response_end);
    }
  }

  if (OB_SUCC(ret) && consume_size > 0) {
    // all data has recevied complete, and decompress totally
    if (!analyze_result.is_last_ok_handled()
        && analyze_result.get_last_ok_pkt_len() > 0) {
      // only resultset will use transform to decompress, so we just trim last ok packet
      if (OB_FAIL(sm_->trim_ok_packet(*local_transfer_reader_))) {
        PROXY_API_LOG(WARN, "fail to trim last ok packet", K(ret));
      } else {
        analyze_result.is_last_ok_handled_ = true;
      }
      sm_->print_mysql_complete_log(NULL);
    }

    if (OB_SUCC(ret)) {
      // the current state may be has already set to CMD_COMPLETE in tunnel by thre
      // producer of server session, when received complete. but it not decompressed,
      // so we should set TRANSACTION_COMPLETE here.
      if (analyze_result.is_trans_completed()) {
        sm_->trans_state_.current_.state_ = ObMysqlTransact::TRANSACTION_COMPLETE;
      } else if (analyze_result.is_resp_completed()) {
        sm_->trans_state_.current_.state_ = ObMysqlTransact::CMD_COMPLETE;
      }

      // get consume size again, for trim the last packet
      consume_size = local_transfer_reader_->read_avail() - analyze_result.get_reserved_len_for_ob20_ok();

      // just send all data in local_transfer_reader_
      if (consume_size != (produce_size = produce(local_transfer_reader_, consume_size))) {
        ret = OB_ERR_UNEXPECTED;
        PROXY_API_LOG(WARN, "fail to produce", "expected size", consume_size,
                      "actual size", produce_size, K(ret));
      } else if (OB_FAIL(local_transfer_reader_->consume(consume_size))) {
        PROXY_API_LOG(WARN, "fail to consume local transfer reader", K(consume_size), K(ret));
      }
    }
  }

  if (OB_FAIL(ret)) {
    sm_->trans_state_.inner_errcode_ = ret;
    // if failed, set state to INTERNAL_ERROR
    sm_->trans_state_.current_.state_ = ObMysqlTransact::INTERNAL_ERROR;
  }

  return ret;
}

void ObMysqlResponseCompressTransformPlugin::handle_input_complete()
{
  PROXY_API_LOG(DEBUG, "ObMysqlResponseCompressTransformPlugin::handle_input_complete happen");
  if (NULL != local_reader_) {
    local_reader_->dealloc();
    local_reader_ = NULL;
  }
  if (NULL != local_transfer_reader_) {
    local_transfer_reader_->dealloc();
    local_transfer_reader_ = NULL;
  }
  set_output_complete();
}

} // end of namespace proxy
} // end of namespace obproxy
} // end of namespace oceanbase
