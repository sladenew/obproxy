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

#ifndef OBPROXY_OB_PROXY_OPERATOR_AGG_H
#define OBPROXY_OB_PROXY_OPERATOR_AGG_H

#include "ob_proxy_operator.h"
#include "ob_proxy_operator_sort.h"
#include "common/ob_row_store.h"
#include "lib/timezone/ob_timezone_info.h"
#include "common/expression/ob_i_sql_expression.h" /* common::ObExprCtx */


namespace oceanbase {
namespace obproxy {
namespace engine {
typedef common::ObSEArray<common::ObColumnInfo, 4, common::ObIAllocator&> ObColInfoArray;

class ObAggregateFunction;
class HashTable;
class ObHashCols;
class ObGbyHashCols;

class ObColumnInfo
{
public:
  int64_t index_;
  ObProxyExpr *group_expr_;
  common::ObCollationType cs_type_; //NOT TO USE
  ObColumnInfo()
    : index_(common::OB_INVALID_INDEX),
      group_expr_(NULL),
      cs_type_(common::CS_TYPE_INVALID) {}

  virtual ~ObColumnInfo() {}

  TO_STRING_KV2(N_INDEX_ID, index_,
      N_COLLATION_TYPE, common::ObCharset::collation_name(cs_type_));
};

class HashTable
{
public:
  HashTable(common::ObIAllocator &allocator)
        : allocator_(allocator),
        buckets_(ObModIds::OB_SE_ARRAY_ENGINE, array_new_alloc_size),
        nbuckets_(0),
        buf_cnt_(0),
        cur_(0)
  {}

  void reuse()
  {
    cur_ = 0;
  }
  void reset()
  {
    //buckets_.reset();
    nbuckets_ = 0;
    cur_ = 0;
  }

//protected:
public:
  common::ObIAllocator &allocator_;
  common::ObSEArray<ObGbyHashCols*, 1> buckets_;
  int64_t nbuckets_;
  int64_t buf_cnt_;
  int64_t cur_;
};


class ObAggregateFunction
{
public:
  struct GroupRow
  {
    GroupRow() : row_(NULL) {}
    common::ObRowStore::StoredRow *row_;
    TO_STRING_KV(K(row_));
  };
  //ObAggregateFunction();
  ObAggregateFunction(common::ObIAllocator &allocator,
        common::ObSEArray<ObProxyExpr*, 4> &select_exprs);
  ~ObAggregateFunction();

  //int init();

  void set_int_div_as_double(bool did);
  bool get_int_div_as_double() const;
  void set_sort_based_gby() { is_sort_based_gby_ = true; }

  int clone_cell(const common::ObObj &src_cell, common::ObObj &target_cell);

  int clone_number_cell(const common::ObObj &src_cell, common::ObObj &target_cell);

  int max_calc(common::ObObj &base,
         const common::ObObj &other,
         common::ObCollationType cs_type);

  int min_calc(common::ObObj &base,
         const common::ObObj &other,
         common::ObCollationType cs_type);

  int add_calc(common::ObObj &res,
         const common::ObObj &left,
         const common::ObObj &right,
         const common::ObTimeZoneInfo *tz_info);

  int calc_aggr_cell(const ObProxyExprType aggr_fun,  const ResultRow &oprands, 
             common::ObObj &res, const common::ObTimeZoneInfo *tz_info,
             common::ObCollationType cs_type);

  int calc_aggr_cell(const ObProxyExprType aggr_fun, common::ObObj &res,
             common::ObObj &src);

  int cal_row_agg(ResultRow &obj_row, ResultRow &src_row, bool &has_inited_normal_cell);

  bool is_same_group(const ResultRow &row1, const ResultRow &row2);

  int is_same_group(const ObRowStore::StoredRow &row1, const ObNewRow &row2,
      bool &result, int64_t &first_diff_pos);

  int is_same_group(const ResultRow &row1, const ResultRow &row2,
      bool &result, int64_t &first_diff_pos);

  const static int64_t STORED_ROW_MAGIC_NUM  = 0xaaaabbbbccccdddd;

  int init(ObColInfoArray &group_col_idxs_);

  virtual int add_row(ResultRow *row);
  virtual int handle_all_result(ResultRow *&row);
  virtual int handle_all_hash_result(ResultRows *rows);
  inline static bool is_int_int_out_of_range(int64_t val1, uint64_t val2, uint64_t res)
  {
    // top digit:
    // 0 + 0     : safe.
    // 0 + 1 = 0 : overflow.
    // 0 + 1 = 1 : safe.
    // 1 + 0 = 0 : safe.
    // 1 + 0 = 1 : underflow.
    // 1 + 1     : safe.
    return (val1 >> SHIFT_OFFSET) != (res >> SHIFT_OFFSET) &&
       (val2 >> SHIFT_OFFSET) != (res >> SHIFT_OFFSET);
  }
  inline static bool is_uint_uint_out_of_range(uint64_t val1, uint64_t val2, uint64_t res)
  {
    // top digit:
    // 0 + 0     : safe.
    // 0 + 1 = 0 : overflow.
    // 0 + 1 = 1 : safe.
    // 1 + 0 = 0 : overflow.
    // 1 + 0 = 1 : safe.
    // 1 + 1     : overflow.
    return (val1 >> SHIFT_OFFSET) + (val2 >> SHIFT_OFFSET) > (res >> SHIFT_OFFSET);
  }

protected:
  common::ObExprCtx *expr_ctx_;
  bool is_sort_based_gby_;
  common::ObSEArray<ObProxyExpr*, 4> &select_exprs_;
  common::ObIAllocator &allocator_;
  ObColInfoArray *group_col_idxs_;
  SortColumnArray *sort_columns_;
  ResultRows *agg_rows_;
  HashTable *result_rows_;
  static const int64_t SHIFT_OFFSET = 63;
};

//Used for calc hash for columns
class ObHashCols
{
public:
  ObHashCols()
      : row_(NULL),
        stored_row_(NULL),
        hash_col_idx_(NULL),
        next_(NULL),
        hash_val_(0) {}

  ObHashCols(ResultRow *row,
    const common::ObIArray<common::ObColumnInfo> *hash_col_idx)
      : row_(row),
        stored_row_(NULL),
        hash_col_idx_(hash_col_idx),
        next_(NULL),
        hash_val_(0) {}

  ~ObHashCols() {}

  int init(ResultRow *row,
       const common::ObIArray<common::ObColumnInfo> *hash_col_idx,
       const uint64_t hash_val = 0)
  {
    row_ = row;
    stored_row_ = NULL;
    hash_col_idx_ = hash_col_idx;
    hash_val_ = hash_val;
    return common::OB_SUCCESS;
  }

  uint64_t hash() const
  {
    if (hash_val_ == 0) {
      hash_val_ = inner_hash();
    }
    return hash_val_;
  }

  uint64_t inner_hash() const;

  bool operator ==(const ObHashCols &other) const;
  void set_stored_row(const common::ObRowStore::StoredRow *stored_row);
  ObHashCols *&next() { return *reinterpret_cast<ObHashCols **>(&next_); };

  TO_STRING_KV(K_(row), K_(hash_val));

public:
  ResultRow *row_;
  const common::ObRowStore::StoredRow *stored_row_;
  const common::ObIArray<common::ObColumnInfo> *hash_col_idx_;
  void *next_;
  mutable uint64_t hash_val_;
};

class ObGbyHashCols : public ObHashCols
{
public:
  ObGbyHashCols *&next() { return *reinterpret_cast<ObGbyHashCols **>(&next_); };
public:
  int64_t group_id_;
};

const int16_t BUCKET_BUF_SIZE = 1024;
const int16_t BUCKET_SHIFT = 10;
const int16_t BUCKET_MASK = 1023;

class ObProxyAggOp : public ObProxyOperator
{
public:
  ObProxyAggOp(ObProxyOpInput *input, common::ObIAllocator &allocator)
            : ObProxyOperator(input, allocator),
              ob_agg_func_(NULL), hash_col_idxs_(NULL),
              sort_columns_(NULL), has_done_agg_(false) {
    set_op_type(PHY_AGG);
  }

  ~ObProxyAggOp();
  virtual int init();
  virtual int get_next_row();
  virtual void set_hash_col_idx(ObColInfoArray &hash_col_idx) {
    hash_col_idxs_ = &hash_col_idx;
  }

  virtual int init_group_by_columns();
  virtual int handle_response_result(void *src, bool is_final, ObProxyResultResp *&result);
  virtual int process_exprs_in_agg(ResultRows *src_rows, ResultRows *obj_rows);

protected:
  ObAggregateFunction *ob_agg_func_;
  ObColInfoArray *hash_col_idxs_;
  SortColumnArray *sort_columns_;
  bool has_done_agg_;
};

class ObProxyAggInput : public ObProxyOpInput
{
public:
  ObProxyAggInput()
     : ObProxyOpInput(),
       group_by_exprs_(ObModIds::OB_SE_ARRAY_ENGINE, array_new_alloc_size),
       having_exprs_(NULL) {}
  ObProxyAggInput(
      const common::ObSEArray<ObProxyExpr*, 4> &select_exprs,
      const common::ObSEArray<ObProxyExpr*, 4> &group_by_exprs,
      ObProxyExpr *&having_exprs)
     : ObProxyOpInput(select_exprs),
       group_by_exprs_(group_by_exprs),
       having_exprs_(having_exprs) {}

  ~ObProxyAggInput() {}

  void init(const common::ObSEArray<ObProxyExpr*, 4> &select_exprs,
     const common::ObSEArray<ObProxyExpr*, 4> &group_by_exprs,
     ObProxyExpr *&having_exprs) {
    select_exprs_ = select_exprs;
    group_by_exprs_ = group_by_exprs;
    having_exprs_ = having_exprs;
  }

  void set_group_by_exprs(const common::ObSEArray<ObProxyExpr*, 4> &group_by_exprs) {
    group_by_exprs_ = group_by_exprs;
  }

  void set_having_exprs(ObProxyExpr* having_exprs) {
    having_exprs_ = having_exprs;
  }

  common::ObSEArray<ObProxyExpr*, 4>& get_group_by_exprs() {
    return group_by_exprs_;
  }

  ObProxyExpr* get_having_exprs() {
    return having_exprs_;
  }

protected:
  common::ObSEArray<ObProxyExpr*, 4> group_by_exprs_;
  ObProxyExpr* having_exprs_;
};

class ObProxyHashAggOp : public ObProxyAggOp
{
public:
  ObProxyHashAggOp(ObProxyOpInput *input, common::ObIAllocator &allocator)
    : ObProxyAggOp(input, allocator), result_rows_(NULL) {
    set_op_type(PHY_HASH_AGG);
  }

  ~ObProxyHashAggOp() {};

  virtual int init();
  virtual int handle_response_result(void *src, bool is_final, ObProxyResultResp *&result);

private:
  HashTable *result_rows_;
};

class ObProxyMergeAggOp : public ObProxyAggOp
{
public:
  ObProxyMergeAggOp(ObProxyOpInput *input, common::ObIAllocator &allocator)
    : ObProxyAggOp(input, allocator), regions_(0), result_rows_array_(NULL),
    result_rows_flag_array_(NULL), regions_results_(NULL) {
    set_op_type(PHY_MERGE_AGG);
  }

  ~ObProxyMergeAggOp() {};

  virtual int init();
  virtual int init_result_rows_array(int64_t regions);
  virtual int handle_response_result(void *src, bool is_final, ObProxyResultResp *&result);
  int fetch_all_result(ResultRows *rows);

protected:
  int64_t regions_;
  ResultRows *result_rows_array_;
  typedef common::ObSEArray<bool, 4, common::ObIAllocator&> ResultFlagArray;
  ResultFlagArray *result_rows_flag_array_;
  typedef common::ObSEArray<ObProxyResultResp*, 4, common::ObIAllocator&> ResultRespArray;
  ResultRespArray *regions_results_;
};

}
}
}

#endif //OBPROXY_OB_PROXY_OPERATOR_AGG_H
