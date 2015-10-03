"""Functional form of the expression operators."""

import struct

from epsilon.expression_pb2 import *

# Accessors

def dimension(expr):
    return expr.size.dim[0]*expr.size.dim[1]

def expr_vars(expr):
    retval = {}
    if expr.expression_type == Expression.VARIABLE:
        retval[expr.variable.variable_id] = expr
    else:
        for arg in expr.arg:
            retval.update(expr_vars(arg))
    return retval

def fp_expr(expr):
    return struct.pack("q", hash(expr.SerializeToString())).encode("hex")

# Constructors

def add(*args):
    assert len(args) > 0

    size = args[0].size
    for i in range(1, len(args)):
        if (args[i].size == size or
            dimension(args[i]) == 1):
            continue

        if size.dim[0]*size.dim[1] == 1:
            size = args[i].size
            continue

        assert False, "adding incompatible sizes"

    argc = [arg.curvature.curvature_type for arg in args]
    C = Curvature
    if all(c == C.CONSTANT for c in argc):
        curvature = Curvature.CONSTANT
    elif all(c == C.AFFINE or c == C.CONSTANT for c in argc):
        curvature = Curvature.AFFINE
    elif all(c == C.CONSTANT or c == C.AFFINE or c == C.CONVEX for c in argc):
        curvature = Curvature.CONVEX
    elif all(c == C.CONSTANT or c == C.AFFINE or c == C.CONCAVE for c in argc):
        curvature = Curvature.CONCAVE
    else:
        curvature = Curvature.UNKNOWN

    return Expression(
        expression_type=Expression.ADD,
        arg=args,
        size=size,
        curvature=Curvature(curvature_type=curvature))

def multiply(*args):
    assert len(args) == 2
    if dimension(args[0]) == 1:
        size = args[1].size
    elif dimension(args[1]) == 1:
        size = args[0].size
    else:
        assert args[0].size.dim[1] == args[1].size.dim[0]
        size = Size(dim=[args[0].size.dim[0], args[1].size.dim[1]])

    return Expression(
        expression_type=Expression.MULTIPLY,
        arg=args,
        size=size)

def hstack(*args):
    e = Expression(expression_type=Expression.HSTACK)

    for i, arg in enumerate(args):
        if i == 0:
            e.size.dim.extend(arg.size.dim)
            e.curvature.curvature_type = arg.curvature.curvature_type
            e.sign.sign_type = arg.sign.sign_type
        else:
            assert e.size.dim[0] == arg.size.dim[0]
            e.size.dim[1] += arg.size.dim[1]

            if arg.curvature.curvature_type != e.curvature.curvature_type:
                e.curvature.curvature_type = Curvature.UNKNOWN
            if arg.sign.sign_type != e.sign.sign_type:
                e.sign.sign_type = Sign.UNKNOWN

        e.arg.add().CopyFrom(arg)

    return e

def vstack(*args):
    e = Expression(expression_type=Expression.VSTACK)

    for i, arg in enumerate(args):
        if i == 0:
            e.size.dim.extend(arg.size.dim)
        else:
            assert e.size.dim[1] == arg.size.dim[1]
            e.size.dim[0] += arg.size.dim[0]

        e.arg.add().CopyFrom(arg)

    return e

def reshape(arg, m, n):
    assert m*n == dimension(arg)

    return Expression(
        expression_type=Expression.RESHAPE,
        arg=[arg],
        size=Size(dim=[m,n]),
        curvature=arg.curvature,
        sign=arg.sign)

def negate(arg):
    NEGATE_CURVATURE = {
        Curvature.UNKNOWN: Curvature.UNKNOWN,
        Curvature.AFFINE: Curvature.AFFINE,
        Curvature.CONVEX: Curvature.CONCAVE,
        Curvature.CONCAVE: Curvature.CONVEX,
        Curvature.CONSTANT: Curvature.CONSTANT,
    }
    return Expression(
        expression_type=Expression.NEGATE,
        arg=[arg],
        size=arg.size,
        curvature=Curvature(
            curvature_type=NEGATE_CURVATURE[arg.curvature.curvature_type],
            elementwise=arg.curvature.elementwise,
            scalar_multiple=arg.curvature.scalar_multiple))

def variable(m, n, variable_id):
    return Expression(
        expression_type=Expression.VARIABLE,
        size=Size(dim=[m, n]),
        variable=Variable(variable_id=variable_id),
        curvature=Curvature(
            curvature_type=Curvature.AFFINE,
            elementwise=True,
            scalar_multiple=True))

def constant(m, n, scalar=0):
    return Expression(
        expression_type=Expression.CONSTANT,
        size=Size(dim=[m, n]),
        constant=Constant(scalar=scalar),
        curvature=Curvature(curvature_type=Curvature.CONSTANT))

def indicator(cone_type, *args):
    return Expression(
        expression_type=Expression.INDICATOR,
        size=Size(dim=[1, 1]),
        cone=Cone(cone_type=cone_type),
        arg=args)

def norm_pq(x, p, q):
    return Expression(
        expression_type=Expression.NORM_PQ,
        size=Size(dim=[1, 1]),
        arg=[x], p=p, q=q)

def norm_p(x, p):
    return Expression(
        expression_type=Expression.NORM_P,
        size=Size(dim=[1, 1]),
        arg=[x], p=p)

def power(x, p):
    return Expression(
        expression_type=Expression.POWER,
        size=x.size,
        arg=[x], p=p)

def abs_val(x):
    return Expression(
        expression_type=Expression.ABS,
        size=x.size,
        arg=[x])

def sum_entries(x):
    return Expression(
        expression_type=Expression.SUM,
        size=Size(dim=[1, 1]),
        arg=[x])

def equality_constraint(a, b):
    return indicator(Cone.ZERO, add(a, negate(b)))

def leq_constraint(a, b):
    return indicator(Cone.NON_NEGATIVE, b, a)

def non_negative(x):
    return indicator(Cone.NON_NEGATIVE, x)
