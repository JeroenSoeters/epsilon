#!/usr/bin/env python

from collections import namedtuple
import time

from epsilon.problems import covsel
from epsilon.problems import lasso
from epsilon.problems import tv_smooth
import epsilon

Problem = namedtuple("Problem", ["name", "create", "kwargs"])

class Column(namedtuple("Column", ["name", "width", "fmt", "right"])):
    """Columns for a Markdown appropriate text table."""

    @property
    def header(self):
        header_fmt = " %" + str(self.width-2) + "s "
        return header_fmt % self.name

    @property
    def sub_header(self):
        val = "-" * (self.width-2)
        if self.right:
            val = " " + val + ":"
        else:
            val = ":" + val + " "
        return val

Column.__new__.__defaults__ = (None, None, None, False)

PROBLEMS = [
    Problem("covsel", covsel.create, dict(m=100, n=200, lam=0.1)),
    Problem("lasso", lasso.create, dict(m=1500, n=5000)),
    Problem("tv_smooth", tv_smooth.create, dict(n=400, lam=1)),
]

COLUMNS = [
    Column("Problem",   10, "%-10s"),
    Column("Time",      8,  "%7.2fs", right=True),
    Column("Objective", 11, "%11.2e", right=True),
]

def print_header():
    print "|".join(c.header for c in COLUMNS)
    print "|".join(c.sub_header for c in COLUMNS)

def print_problem(*args):
    print "|".join(c.fmt % args[i] for i, c in enumerate(COLUMNS))

if __name__ == "__main__":
    print_header()
    for problem in PROBLEMS:
        cvxpy_prob = problem.create(**problem.kwargs)

        t0 = time.time()
        epsilon.solve(cvxpy_prob)
        t1 = time.time()

        print_problem(problem.name, t1-t0, cvxpy_prob.objective.value)
