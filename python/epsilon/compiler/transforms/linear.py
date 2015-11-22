"""Transforms for linear functions."""

import logging

from epsilon import dcp
from epsilon import error
from epsilon import expression
from epsilon import linear_map
from epsilon import tree_format
from epsilon.compiler import validate
from epsilon.expression_pb2 import Problem, Constant
from epsilon.compiler.transforms.transform_util import *

def transform_variable(expr):
    return expression.reshape(expr, dim(expr), 1)

def transform_constant(expr):
    return expression.reshape(expr, dim(expr), 1)

def promote_add(expr, dim_sum):
    if dim(expr) == dim_sum:
        return expr
    if dim(expr) != 1:
        raise TransformError("Can't promote non-scalar", expr)
    return expression.linear_map(linear_map.promote(dim_sum), expr)

def transform_add(expr):
    return expression.add(
        *[promote_add(transform_expr(e), dim(expr)) for e in expr.arg])

def transform_index(expr):
    return expression.linear_map(
        linear_map.kronecker_product(
            linear_map.index(expr.key[1], dim(only_arg(expr),1)),
            linear_map.index(expr.key[0], dim(only_arg(expr),0))),
        transform_expr(only_arg(expr)))

def multiply_constant(expr, n):
    # TODO(mwytock): Handle this case
    if expr.expression_type == Expression.CONSTANT:
        if expr.constant.constant_type == Constant.SCALAR:
            return linear_map.scalar(expr.constant.scalar, n)
        if expr.constant.constant_type == Constant.DENSE_MATRIX:
            return linear_map.dense_matrix(expr.constant)
        if expr.constant.constant_type == Constant.SPARSE_MATRIX:
            return linear_map.sparse_matrix(expr.constant)
    elif expr.expression_type == Expression.TRANSPOSE:
        return linear_map.transpose(multiply_constant(only_arg(expr), n))

    raise TransformError("unknown constant type", expr)

def transform_multiply(expr):
    if len(expr.arg) != 2:
        raise TransformError("wrong number of args", expr)

    m = dim(expr, 0)
    n = dim(expr, 1)
    if expr.arg[0].curvature.curvature_type == Curvature.CONSTANT:
        return expression.linear_map(
            linear_map.left_matrix_product(
                multiply_constant(expr.arg[0], m), n),
            transform_expr(expr.arg[1]))

    if expr.arg[1].curvature.curvature_type == Curvature.CONSTANT:
        return expression.linear_map(
            linear_map.right_matrix_product(
                multiply_constant(expr.arg[1], n), m),
            transform_expr(expr.arg[0]))

    raise TransformError("multiplying non constants", expr)

def transform_kron(expr):
    if len(expr.arg) != 2:
        raise TransformError("Wrong number of arguments", expr)

    if not dcp.is_constant(expr.arg[0]):
        raise TransformError("First arg is not constant", expr)

    return expression.linear_map(
        linear_map.kronecker_product_single_arg(
            multiply_constant(expr.arg[0], 1),
            dim(expr.arg[1], 0),
            dim(expr.arg[1], 1)),
        transform_expr(expr.arg[1]))

def multiply_elementwise_constant(expr):
    # TODO(mwytock): Handle this case
    if expr.expression_type != Expression.CONSTANT:
        raise TransformError("multiply constant is not leaf", expr)

    if expr.constant.constant_type == Constant.DENSE_MATRIX:
        return linear_map.diagonal_matrix(expr.constant)

    raise TransformError("unknown constant type", expr)

def transform_multiply_elementwise(expr):
    if len(expr.arg) != 2:
        raise TransformError("wrong number of args", expr)

    if expr.arg[0].curvature.curvature_type == Curvature.CONSTANT:
        c_expr = expr.arg[0]
        x_expr = expr.arg[1]
    elif expr.arg[1].curvature.curvature_type == Curvature.CONSTANT:
        c_expr = expr.arg[1]
        x_expr = expr.arg[0]
    else:
        raise TransformError("multiply non constants", expr)

    return expression.linear_map(
        multiply_elementwise_constant(c_expr),
        transform_expr(x_expr))

def transform_negate(expr):
    return expression.linear_map(
        linear_map.negate(dim(expr)),
        transform_expr(only_arg(expr)))

def transform_sum(expr):
    return expression.linear_map(
        linear_map.sum(dim(only_arg(expr))),
        transform_expr(only_arg(expr)))

def transform_hstack(expr):
    m = dim(expr, 0)
    n = dim(expr, 1)
    offset = 0
    add_args = []
    for arg in expr.arg:
        ni = dim(arg, 1)
        add_args.append(
            expression.linear_map(
                linear_map.right_matrix_product(
                    linear_map.index(slice(offset, offset+ni), n), m),
                transform_expr(arg)))
        offset += ni
    return expression.add(*add_args)

def transform_vstack(expr):
    m = dim(expr, 0)
    n = dim(expr, 1)
    offset = 0
    add_args = []
    for arg in expr.arg:
        mi = dim(arg, 0)

        add_args.append(
            expression.linear_map(
                linear_map.left_matrix_product(
                    linear_map.transpose(
                        linear_map.index(slice(offset, offset+mi), m)),
                    n),
                transform_expr(arg)))
        offset += mi
    return expression.add(*add_args)

def transform_reshape(expr):
    # drop reshape nodes as everything is a vector
    return transform_expr(only_arg(expr))

def transform_linear_map(expr):
    return expr

def transform_diag_mat(expr):
    return expression.linear_map(
        linear_map.diag_mat(dim(expr)),
        transform_expr(only_arg(expr)))

def transform_diag_vec(expr):
    return expression.linear_map(
        linear_map.diag_vec(dim(expr, 0)),
        transform_expr(only_arg(expr)))

def transform_upper_tri(expr):
    return expression.linear_map(
        linear_map.upper_tri(dim(expr, 0)),
        transform_expr(only_arg(expr)))

def transform_trace(expr):
    return expression.linear_map(
        linear_map.trace(dim(only_arg(expr), 0)),
        transform_expr(only_arg(expr)))

def transform_power(expr):
    p = expr.p
    if p == 1:
        return transform_expr(only_arg(expr))
    if p == 0:
        return expression.scalar_constant(1)

    raise TransformError("Unexpected power exponent", expr)

def transform_linear_expr(expr):
    logging.debug("transform_linear_expr:\n%s", tree_format.format_expr(expr))
    f_name = "transform_" + Expression.Type.Name(expr.expression_type).lower()
    return globals()[f_name](expr)

def transform_expr(expr):
    if expr.curvature.curvature_type in (Curvature.AFFINE, Curvature.CONSTANT):
        return transform_linear_expr(expr)
    else:
        for arg in expr.arg:
            arg.CopyFrom(transform_expr(arg))
        return expr

def transform_problem(problem):
    return Problem(
        objective=transform_expr(problem.objective),
        constraint=[transform_expr(e) for e in problem.constraint])
