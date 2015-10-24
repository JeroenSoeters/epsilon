#include "epsilon/linear/dense_matrix_impl.h"
#include "epsilon/util/string.h"
#include "epsilon/vector/vector_util.h"

namespace linear_map {

LinearMapImpl* DenseMatrixImpl::Inverse() const {
  // TODO(mwytock): LLT method may be faster?
  return new DenseMatrixImpl(A_.inverse());
}

std::string DenseMatrixImpl::DebugString() const {
  return StringPrintf(
      "dense matrix %d x %d\n%s", m(), n(), MatrixDebugString(A_).c_str());
}

bool DenseMatrixImpl::operator==(const LinearMapImpl& other) const {
  if (other.type() != DENSE_MATRIX ||
      other.m() != m() ||
      other.n() != n())
    return false;
  return static_cast<const DenseMatrixImpl&>(other).dense() == A_;
}


}  // namespace
