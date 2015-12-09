# Epsilon [![Circle CI](https://circleci.com/gh/mwytock/epsilon.svg?style=svg)](https://circleci.com/gh/mwytock/epsilon)

Epsilon is a general convex solver based on functions with efficient proximal
operators.

## Installation instructions

First, follow the instructions to install the numerical python environment
including CVXPY.

### Mac OS X

```
pip install epopt
```

### Linux

```
pip install http://epopt.s3.amazonaws.com/epopt-0.1.0-cp27-none-linux_x86_64.whl
```

## Usage

In order to use Epsilon, form an optimization problem using CVXPY in the usual
way but solve it using Epsilon.
```python
import numpy as np
import cvxpy as cp
import epopt as ep

# Form lasso problem with CVXPY
m = 5
n = 10
A = np.random.randn(m,n)
b = np.random.randn(m)
x = cp.Variable(n)
prob = cp.Problem(cp.Minimize(cp.sum_squares(A*x - b) + cp.norm1(x)))

# Solve with Epsilon
print "Objective:"
print ep.solve(prob)
print "Solution:"
print x.value
```

## Development Instructions

These instructions are for setting up the development environment with the
required C++ and numerical python envirnoment (cvxpy, numpy, scipy). For
end-users the package should be pip-installable, with binaries provided
for common environments.

### C++ dependencies on Mac OS X

Install dependencies using Homebrew (or MacPorts):

```
brew install glog gflags
brew install --devel protobuf
```

### C++ dependencies on Ubuntu

Install dependencies with the package manager
```
apt-get install libglog-dev libgflags-dev
```

The protocol buffer library must be >3.0.0 which is not yet included in
apt-get. It can be downloaed from https://github.com/google/protobuf.
```
wget https://github.com/google/protobuf/releases/download/v3.0.0-beta-1/protobuf-cpp-3.0.0-beta-1.tar.gz
tar zxvf protobuf-cpp-3.0.0-beta-1.tar.gz
cd protobuf-cpp-3.0.0-beta-1
./configure
make install
```

### NumPy, SciPy and CVXPY dependencies

Make sure to have the most recent version of numpy, scipy and cvxpy packages
```
pip install -U numpy scipy
pip install -U cvxpy
```

### Build Epsilon and run tests

First, get the sub modules
```
git submodule update --init
```
Compile the C++ code and run tests
```
make -j test
```

Now build the C++ Python extension and set up the local development environment
```
python setup.py build
python setup.py develop --user
```
Run python tests
```
pip install nose
nosetests epsilon
```

## Benchmark results

### Epsilon
```
python -m epsilon.problems.benchmark
```
Problem       |   Time | Objective
:------------- | ------:| ---------:
basis_pursuit  |   2.96s|   1.45e+02
covsel         |   0.77s|   3.63e+02
group_lasso    |   9.61s|   1.61e+02
hinge_l1       |   5.42s|   1.50e+03
huber          |   0.51s|   2.18e+03
lasso          |   3.88s|   1.64e+01
least_abs_dev  |   0.38s|   7.09e+03
logreg_l1      |   4.84s|   1.04e+03
lp             |   0.56s|   7.77e+02
mnist          |   1.44s|   1.53e+03
quantile       |  16.40s|   3.64e+03
tv_1d          |   0.50s|   2.13e+05
tv_denoise     |  24.17s|   1.15e+06

### SCS
```
python -m epsilon.problems.benchmark --scs
```

 Problem       |   Time | Objective
:------------- | ------:| ---------:
basis_pursuit  |  16.99s|   1.45e+02
covsel         |  23.50s|   3.62e+02
group_lasso    |  23.31s|   1.61e+02
hinge_l1       |  52.62s|   1.50e+03
huber          |   3.39s|   2.18e+03
lasso          |  22.02s|   1.63e+01
least_abs_dev  |   3.81s|   7.10e+03
logreg_l1      |  55.53s|   1.04e+03
lp             |   5.47s|   7.75e+02
mnist          | 227.65s|   1.60e+03
tv_1d          |  47.28s|   3.51e+05
tv_denoise     | 372.86s|   1.69e+06
quantile       |  88.60s|   4.99e+03
