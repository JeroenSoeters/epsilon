
#include <unordered_set>

#include <glog/logging.h>

#include "epsilon/util/string.h"
#include "epsilon/vector/block_matrix.h"

bool InvertBlockDiagonal(const BlockMatrix& A, BlockMatrix* A_inv) {
  std::set<std::string> seen_row_keys;
  for (const auto& col_iter : A.data()) {
    if (col_iter.second.size() != 1)
      return false;
    const std::string& row_key = col_iter.second.begin()->first;
    if (seen_row_keys.find(row_key) != seen_row_keys.end())
      return false;
    seen_row_keys.insert(row_key);
  }

  for (const auto& col_iter : A.data()) {
    const std::string& col_key = col_iter.first;
    const std::string& row_key = col_iter.second.begin()->first;
    const linear_map::LinearMap& Ai = col_iter.second.begin()->second;
    A_inv->InsertOrAdd(col_key, row_key, Ai.Inverse());
  }
  return true;
}

linear_map::LinearMap& BlockMatrix::operator()(
    const std::string& row_key, const std::string& col_key) {
  return data_[col_key][row_key];
}

const linear_map::LinearMap& BlockMatrix::operator()(
    const std::string& row_key, const std::string& col_key) const {
  auto col_iter = data_.find(col_key);
  CHECK(col_iter != data_.end()) << "column: " << col_key << " not found";
  auto block_iter = col_iter->second.find(row_key);
  CHECK(block_iter != col_iter->second.end())
      << "row: " << row_key << " not found";
  return block_iter->second;
}

bool BlockMatrix::has_key(
    const std::string& row_key, const std::string& col_key) const {
  auto col_iter = data_.find(col_key);
  if (col_iter == data_.end())
    return false;
  auto block_iter = col_iter->second.find(row_key);
  if (block_iter == col_iter->second.end())
    return false;
  return true;
}

BlockMatrix BlockMatrix::Transpose() const {
  BlockMatrix transpose;
  for (const auto& col_iter : data_) {
    for (const auto& item_iter : col_iter.second) {
      transpose.InsertOrAdd(
          col_iter.first, item_iter.first, item_iter.second.Transpose());
    }
  }
  return transpose;
}

BlockMatrix BlockMatrix::Inverse() const {
  CHECK_EQ(m(), n()) << "Inverting non square matrix";
  BlockMatrix A_inv;
  if (InvertBlockDiagonal(*this, &A_inv)) {
    return A_inv;
  }

  LOG(FATAL) << "Unable to invert matrix\n" << DebugString();
}

BlockMatrix BlockMatrix::LeftIdentity() const {
  BlockMatrix C;
  for (const auto& col_iter : data_) {
    for (const auto& block_iter : col_iter.second) {
      const std::string& key = block_iter.first;
      if (C.data_.find(key) == C.data_.end()) {
        C.InsertOrAdd(
            key, key, linear_map::Identity(block_iter.second.impl().m()));
      }
    }
  }
  return C;
}

BlockMatrix BlockMatrix::RightIdentity() const {
  BlockMatrix C;
  for (const auto& col_iter : data_) {
    CHECK(col_iter.second.size() > 0);
    const auto& block_iter = *col_iter.second.begin();
    const std::string& key = col_iter.first;
    C.InsertOrAdd(
        key, key, linear_map::Identity(block_iter.second.impl().n()));
  }
  return C;
}

BlockMatrix operator*(const BlockMatrix& A, const BlockMatrix& B) {
  BlockMatrix C;

  for (const auto& B_col_iter : B.data_) {
    for (const auto& B_iter : B_col_iter.second) {
      auto A_col_iter_ptr = A.data_.find(B_iter.first);
      if (A_col_iter_ptr == A.data_.end())
        continue;
      const auto& A_col_iter = *A_col_iter_ptr;

      for (const auto& A_iter : A_col_iter.second) {
        VLOG(3) << "C(" << A_iter.first << "," << B_col_iter.first << ") += "
                << "A(" << A_iter.first << "," << A_col_iter.first << ")*"
                << "B(" << B_iter.first << "," << B_col_iter.first << ")";
        VLOG(3) << A_iter.second.impl().m() << " x " << A_iter.second.impl().n();
        VLOG(3) << B_iter.second.impl().m() << " x " << B_iter.second.impl().n();
        C.InsertOrAdd(
            A_iter.first, B_col_iter.first, A_iter.second*B_iter.second);
      }
    }
  }

  return C;
}

BlockMatrix operator+(const BlockMatrix& A, const BlockMatrix& B) {
  BlockMatrix C = A;
  for (const auto& col_iter : B.data_) {
    for (const auto& block_iter : col_iter.second) {
      C.InsertOrAdd(block_iter.first, col_iter.first, block_iter.second);
    }
  }
  return C;
}

BlockMatrix operator-(const BlockMatrix& A, const BlockMatrix& B) {
  return A + (-1)*B;
}

BlockMatrix operator*(double alpha, const BlockMatrix& A) {
  BlockMatrix C;
  for (const auto& col_iter : A.data_) {
    for (const auto& block_iter : col_iter.second) {
      C.InsertOrAdd(block_iter.first, col_iter.first, alpha*block_iter.second);
    }
  }
  return C;
}

BlockMatrix operator*(const BlockMatrix& A, double alpha) {
  return alpha*A;
}

BlockVector operator*(const BlockMatrix& A, const BlockVector& x) {
  VLOG(3) << "block matrix-vector product";
  BlockVector y;
  for (const auto& x_iter : x.data_) {
    auto col_iter = A.data_.find(x_iter.first);
    if (col_iter == A.data_.end())
      continue;

    for (const auto& block_iter : col_iter->second)
      y.InsertOrAdd(block_iter.first, block_iter.second*x_iter.second);
  }
  VLOG(3) << "block matrix-vector product done";
  return y;
}

void BlockMatrix::InsertOrAdd(
    const std::string& row_key,
    const std::string& col_key,
    linear_map::LinearMap value) {
  auto res = data_[col_key].insert(std::make_pair(row_key, value));
  if (!res.second) (res.first)->second += value;
}

int BlockMatrix::m() const {
  std::unordered_set<std::string> seen;
  int m = 0;
  for (const auto& col_iter : data_) {
    for (const auto& block_iter : col_iter.second) {
      auto seen_iter = seen.find(block_iter.first);
      if (seen_iter != seen.end())
        continue;
      m += block_iter.second.impl().m();
      seen.insert(block_iter.first);
    }
  }
  return m;
}

int BlockMatrix::n() const {
  int n = 0;
  for (auto col_iter : data_) {
    n += col_iter.second.begin()->second.impl().n();
  }
  return n;
}

std::set<std::string> BlockMatrix::row_keys() const {
  std::set<std::string> retval;
  for (const auto& col_iter : data_) {
    for (const auto& block_iter : col_iter.second) {
      retval.insert(block_iter.first);
    }
  }
  return retval;
}

std::set<std::string> BlockMatrix::col_keys() const {
  std::set<std::string> retval;
  for (const auto& col_iter : data_) {
    retval.insert(col_iter.first);
  }
  return retval;
}

const std::map<std::string, linear_map::LinearMap>& BlockMatrix::col(
    const std::string& col_key) const {
  auto iter = data_.find(col_key);
  CHECK(iter != data_.end());
  return iter->second;
}

std::string BlockMatrix::DebugString() const {
  std::string retval = StringPrintf("block matrix %d x %d", m(), n());
  for (const auto& col_iter : data_) {
    for (const auto& block_iter : col_iter.second) {
      if (retval != "") retval += "\n";
      retval += "(" + block_iter.first + ", " + col_iter.first + ")\n";
      retval += block_iter.second.impl().DebugString();
    }
  }
  return retval;
}

void BlockMatrix::Remove(
    const std::string& row_key, const std::string& col_key) {
  auto iter = data_.find(col_key);
  CHECK(iter != data_.end());
  auto iter2 = iter->second.find(row_key);
  CHECK(iter2 != iter->second.end());
  iter->second.erase(iter2);
  if (iter->second.empty())
    data_.erase(iter);
}
